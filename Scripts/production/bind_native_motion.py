#!/usr/bin/env python3
"""Bind genuine, already imported UE source motions to the ONE packaged global
NoWorld action catalog. No per-Version motion libraries or pre-phone approval gates.

python Scripts/production/bind_native_motion.py path/to/original_source_bindings.json

Input {"schema":"noworld.native_motion_bindings.v1","motions":[...]} uses the
same fields as Content/NoWorld/Animation/SharedMotionCatalog.json, plus
optional source evidence. Only genuine existing native source and authored
combat timing/scene-role constraints may affect binding; user phone rejection
is a separate explicit owner decision.
The authoritative native asset must already exist as a .uasset under Content.
This utility changes ONLY the shared catalog, never source or gameplay rules.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import tempfile

SOURCE_ROOT = Path(__file__).resolve().parents[2]
ROOT = SOURCE_ROOT
CATALOG = ROOT / "Content/NoWorld/Animation/SharedMotionCatalog.json"
SCHEMA = "noworld.shared_motion_catalog.v1"
INPUT_SCHEMA = "noworld.native_motion_bindings.v1"
SHA256 = re.compile(r"^[a-fA-F0-9]{64}$")
ID = re.compile(r"^[A-Za-z0-9_.:\-/]+$")


def checked_id(value: object, field: str) -> str:
    if not isinstance(value, str) or not ID.fullmatch(value):
        raise ValueError(f"{field}: require a nonempty canonical name")
    return value


def inside_repo(path: str) -> Path:
    if not isinstance(path, str) or not path or Path(path).is_absolute():
        raise ValueError("Evidence paths must be repository relative")
    # GitHub checkout and the authorized Unreal workstation can be distinct.
    # Source-authentic proof is read from the committed checkout, or its exact
    # path in the native project; never from unrelated user directories.
    for parent in (SOURCE_ROOT, ROOT):
        target = (parent / path).resolve()
        if target.is_relative_to(parent) and target.is_file():
            return target
    raise ValueError(f"Evidence is not an existing approved project file: {path}")


def native_package(name: str) -> Path:
    if not isinstance(name, str) or not name.startswith("/Game/"):
        raise ValueError("native_sequence must be an Unreal /Game/ object path")
    if name.count(".") != 1:
        raise ValueError("native_sequence requires Package.Asset")
    package, asset = name[6:].split(".")
    if not package or not asset or package.rsplit("/", 1)[-1] != asset:
        raise ValueError("native_sequence package and object names must match")
    path = (ROOT / "Content" / (package + ".uasset")).resolve()
    if not path.is_relative_to((ROOT / "Content").resolve()) or not path.is_file():
        raise ValueError(f"Original imported Unreal asset is missing: {name}")
    return path


def key(m: dict) -> tuple:
    return tuple(str(m.get(k, "")).casefold() for k in (
        "action", "context", "rig", "anatomy", "outfit",
        "participants", "scene_role", "priority"))


def validate_motion(m: dict) -> dict:
    if not isinstance(m, dict):
        raise ValueError("Every motion binding must be a JSON object")
    fields = ("motion_id", "source_clip_id", "action", "context", "rig",
              "anatomy", "native_sequence", "source_hash")
    for field in fields:
        if field not in m:
            raise ValueError(f"Motion is missing {field}")
    for field in fields[:6]:
        checked_id(m[field], field)
    if not SHA256.fullmatch(str(m["source_hash"])):
        raise ValueError("source_hash must be the authentic original SHA-256")
    # An older authoring flag is NOT the owner's explicit phone-test decision.
    # Never manufacture acceptance or exclude an authentic native source from
    # the common catalog just because legacy QA marked it false.
    if m.get("approval_evidence"):
        inside_repo(m["approval_evidence"])
    if m.get("owner_phone_review") == "REJECTED_BY_OWNER_AFTER_PHONE_TEST":
        raise ValueError("Explicit owner phone-test rejection requires corrective source, not automatic re-entry")
    native_package(m["native_sequence"])
    if m.get("contact_seconds") is not None and (
        isinstance(m["contact_seconds"], bool) or
        not isinstance(m["contact_seconds"], (float, int)) or
        not (-1 <= m["contact_seconds"] <= 10000)):
        raise ValueError("contact_seconds must be an authentic nonnegative time, or -1")
    if "natural_completion_contact" in m and not isinstance(m["natural_completion_contact"], bool):
        raise ValueError("natural_completion_contact requires a source boolean")
    authored_seconds = isinstance(m.get("contact_seconds"), (float, int)) and m["contact_seconds"] >= 0
    authored_notify = bool(m.get("contact_notify"))
    natural_completion = m.get("natural_completion_contact") is True
    if natural_completion and (authored_seconds or authored_notify or m.get("loop", False)):
        raise ValueError("Natural completion cannot replace an authored marker or looping clip")
    if str(m["action"]).startswith("Combat.") and not (
        authored_seconds or authored_notify or natural_completion):
        raise ValueError("Combat requires authored contact or explicit natural completion")
    if isinstance(m.get("participants", 1), bool) or not isinstance(m.get("participants", 1), int) or m.get("participants", 1) < 1:
        raise ValueError("participants must be a positive integer")
    if m.get("capabilities") is not None and (
        not isinstance(m["capabilities"], list) or
        not all(isinstance(x, str) and ID.fullmatch(x) for x in m["capabilities"])):
        raise ValueError("capabilities must be canonical names")
    if m.get("participants", 1) > 1 and (
        not m.get("scene_id") or not m.get("scene_role")):
        raise ValueError("Synchronized participants require original scene identity and role")
    # This runtime catalog is character-independent: rig/action/role select
    # derivatives. Character identity/Version/Manifestation never fork the bank.
    forbidden = ("character_version", "manifestation_id", "character_id")
    if any(k in m for k in forbidden):
        raise ValueError("Bind the canonical character on the native controller, not in this motion bank")
    result = dict(m)
    result.pop("approved_for_gameplay", None)
    result.pop("approval_evidence", None)
    result.pop("owner_phone_review", None)
    result["source_hash"] = str(result["source_hash"]).lower()
    return result


def bind(recipe_file: Path) -> dict:
    recipe = json.loads(recipe_file.read_text(encoding="utf-8"))
    if recipe.get("schema") != INPUT_SCHEMA or not isinstance(recipe.get("motions"), list) or not recipe["motions"]:
        raise ValueError(f"Recipe requires {INPUT_SCHEMA} and nonempty motions")
    if CATALOG.stat().st_size:
        existing = json.loads(CATALOG.read_text(encoding="utf-8"))
    else:
        existing = {"schema": SCHEMA, "motions": []}
    if existing.get("schema") != SCHEMA or not isinstance(existing.get("motions"), list):
        raise ValueError("Existing shared motion catalog is invalid; refusing to overwrite")
    motions = existing["motions"]
    staged_ids = {}
    for source in recipe["motions"]:
        m = validate_motion(source)
        ident = m["motion_id"].casefold()
        if ident in staged_ids and staged_ids[ident] != m:
            raise ValueError(f"Conflicting definitions for {m['motion_id']}")
        staged_ids[ident] = m
    merged = {m["motion_id"].casefold(): m for m in motions}
    if len(merged) != len(motions):
        raise ValueError("Existing catalog has duplicate IDs")
    for ident, m in staged_ids.items():
        prior = merged.get(ident)
        if prior and prior.get("source_clip_id") != m["source_clip_id"]:
            raise ValueError(f"Cannot silently change authentic source of {m['motion_id']}")
        merged[ident] = m
    occupied = {}
    for m in merged.values():
        k = key(m)
        if k in occupied and occupied[k] != m["motion_id"]:
            raise ValueError(f"Ambiguous shared rig/action binding: {occupied[k]} and {m['motion_id']}")
        occupied[k] = m["motion_id"]
    existing["motions"] = sorted(merged.values(), key=lambda m: m["motion_id"].casefold())
    # Preserve unknown top-level fields and write atomically; no rewriting
    # imported .uasset data, original receipts, user saves or canonical contracts.
    fd, tmp = tempfile.mkstemp(prefix=".shared_motions_", suffix=".json", dir=CATALOG.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as out:
            json.dump(existing, out, ensure_ascii=False, indent=2)
            out.write("\n")
            out.flush()
            os.fsync(out.fileno())
        os.replace(tmp, CATALOG)
    finally:
        if os.path.exists(tmp):
            os.unlink(tmp)
    return {"bound": len(staged_ids), "total": len(merged), "catalog": str(CATALOG.relative_to(ROOT))}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("recipe", type=Path, help="Real original source + exact imported native asset bindings")
    parser.add_argument("--project-root", type=Path,
                        help="Existing authorized native Unreal game project; default is this source checkout")
    args = parser.parse_args()
    global ROOT, CATALOG
    if args.project_root:
        ROOT = args.project_root.resolve(strict=True)
        if not (ROOT / "OfflineGame.uproject").is_file():
            raise ValueError("Selected native Unreal project root has no real OfflineGame.uproject")
        CATALOG = ROOT / "Content/NoWorld/Animation/SharedMotionCatalog.json"
    print(json.dumps(bind(args.recipe.resolve()), indent=2))


if __name__ == "__main__":
    main()
