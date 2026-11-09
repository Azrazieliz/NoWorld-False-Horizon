#!/usr/bin/env python3
"""5-star NoWorld native source preservation: audit every indexed animation's
actual original native package and its inclusion in Android/Windows cooking.
Read-only. Never creates, retargets, approves, or guesses a motion/contact.

  python Scripts/production/verify_shared_motion_cook_coverage.py \
      --source-project D:/UnrealProjects/Gate12_WholeCharacter_Build_20261031 \
      --config-project D:/UnrealProjects/NoWorld_P0_NativeCompile_20261102
"""
from __future__ import annotations
import argparse
from collections import Counter
import json
from pathlib import Path
import re
import sys

MANIFEST = "NoWorld/Animation/SharedMotionCatalog.json"
CONFIG = "Config/DefaultGame.ini"
SCHEMA = "noworld.shared_motion_catalog.v1"
UE4_ASSET_MAGIC = bytes.fromhex("c1832a9e")


def audit(source_project: Path, config_project: Path) -> dict:
    source = source_project / "Content"
    catalog = json.loads((source / MANIFEST).read_text(encoding="utf-8-sig"))
    if catalog.get("schema") != SCHEMA or not isinstance(catalog.get("motions"), list):
        raise ValueError("The original shared motion catalog is missing or invalid")
    config = (config_project / CONFIG).read_text(encoding="utf-8-sig")
    directories = {
        value.rstrip("/")
        for value in re.findall(
            r'^\s*\+DirectoriesToAlwaysCook=\(Path="([^"]+)"\)',
            config, re.MULTILINE
        )
    }
    ufs = set(re.findall(
        r'^\s*\+DirectoriesToAlwaysStageAsUFS=\(Path="([^"]+)"\)',
        config, re.MULTILINE
    ))
    ids: set[str] = set()
    missing: list[str] = []
    bad_assets: list[str] = []
    uncooked: list[str] = []
    categories: Counter[str] = Counter()
    folders: Counter[str] = Counter()
    for motion in catalog["motions"]:
        motion_id = motion.get("motion_id")
        native = motion.get("native_sequence", "")
        if not isinstance(motion_id, str) or not motion_id:
            raise ValueError("Original motion is missing its source identity")
        if motion_id.casefold() in ids:
            raise ValueError("Duplicate original motion identity: " + motion_id)
        ids.add(motion_id.casefold())
        if not isinstance(native, str) or not native.startswith("/Game/") or native.count(".") != 1:
            raise ValueError("Non-native/invalid motion reference: " + motion_id)
        package, asset = native[6:].split(".", 1)
        if not package or package.split("/")[-1] != asset or ".." in package:
            raise ValueError("Native package and asset disagree: " + motion_id)
        categories[motion.get("category", "unknown")] += 1
        folder = "/Game/" + package.rsplit("/", 1)[0]
        folders[folder] += 1
        disk = source / (package + ".uasset")
        if not disk.is_file():
            missing.append(motion_id)
        elif disk.stat().st_size <= 128 or disk.open("rb").read(4) != UE4_ASSET_MAGIC:
            bad_assets.append(motion_id)
        if not any(folder == directory or folder.startswith(directory + "/") for directory in directories):
            uncooked.append(motion_id)

    status = {
        "schema": "noworld.motion_cook_coverage.v1",
        "catalog_entries": len(ids),
        "categories": dict(sorted(categories.items())),
        "source_folders": dict(sorted(folders.items())),
        "native_missing_count": len(missing),
        "native_invalid_count": len(bad_assets),
        "not_in_always_cook_count": len(uncooked),
        "shared_manifest_staged_as_ufs": "NoWorld/Animation" in ufs,
        "missing_sample": missing[:12],
        "invalid_sample": bad_assets[:12],
        "uncooked_sample": uncooked[:12],
        "project_unmodified": True,
    }
    status["pass"] = not (missing or bad_assets or uncooked) and status["shared_manifest_staged_as_ufs"]
    return status


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-project", type=Path, required=True)
    parser.add_argument("--config-project", type=Path, required=True)
    args = parser.parse_args()
    try:
        result = audit(args.source_project, args.config_project)
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as err:
        print("MOTION_COOK_AUDIT_BLOCKED:", str(err), file=sys.stderr)
        return 2
    print(json.dumps(result, indent=2))
    return 0 if result["pass"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
