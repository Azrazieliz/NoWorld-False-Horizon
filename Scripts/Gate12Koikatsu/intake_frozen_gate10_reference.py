"""Verify and stage immutable Gate 10 reference art. Never imports runtime screenshots.

Input is the exact user-approved 2026-10-13 archive; stage only design
references under Saved/Gate12Readiness, NEVER inside Content, Registry or saves.
"""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import tempfile
import zipfile

EXPECTED_MASTER = "b92074ca762d4db3228ef237747e664ea3abc9c2da48941a20716ae84c2e7f49"
REFERENCES = {
    "Gate10_Opening_10_Concepts_2026-10-12.zip": [
        "variants/04_vacant_throne.png",
        "IMPLEMENTATION_AND_REVIEW.md",
    ],
    "Gate10_Continuation_Validated_2026-10-12.zip": [
        "01_FROZEN/WorldHUD/world_hud_freeze.png",
        "01_FROZEN/RulerHome/approved_visual.png",
        "01_FROZEN/Records/approved_visual_with_placeholder_text.png",
        "01_FROZEN/Gacha/user_approved_reference.png",
        "02_TERRITORY_FREEZE_REFERENCE_CANDIDATES/noctis_basin_astral_territory_map.png",
        "02_TERRITORY_FREEZE_REFERENCE_CANDIDATES/gothic_territory_map_of_astral_prime.png",
        "02_TERRITORY_FREEZE_REFERENCE_CANDIDATES/gothic_celestial_territory_map.png",
    ],
    "Gate10_Roster_Approved_Freeze_2026-10-12.zip": [
        "APPROVAL_AND_FREEZE.md", "design_tokens.json",
        "SOURCE_RULES.md",
    ],
}

def sha256(data):
    return hashlib.sha256(data).hexdigest()

def intake(archive, destination, verify_only):
    archive = Path(archive)
    if not archive.is_file():
        raise ValueError("Original Gate10 handoff ZIP not found on this machine")
    if sha256(archive.read_bytes()) != EXPECTED_MASTER:
        raise ValueError("Gate10 original archive SHA256 MISMATCH; refusing source intake")
    files = {}
    with zipfile.ZipFile(archive, "r") as master:
        manifest = json.loads(master.read("SHA256_MANIFEST.json"))
        for name, expected in manifest.items():
            if sha256(master.read(name)) != expected:
                raise ValueError("Nested Gate10 member checksum mismatch: " + name)
        for nested, selected in REFERENCES.items():
            with zipfile.ZipFile(io.BytesIO(master.read(nested))) as child:
                for member in selected:
                    content = child.read(member)
                    key = nested.replace(".zip", "") + "/" + member
                    files[key] = content
        files["818455.png"] = master.read("818455.png")
    report = {
        "schema": "GATE10_ORIGINAL_REFERENCE_ONLY_V1",
        "master_sha256": EXPECTED_MASTER,
        "master_bytes": archive.stat().st_size,
        "not_production_umg": True,
        "no_actual_asset_approval": True,
        "selected_opening_direction": "04_vacant_throne",
        "no_character_chips": True,
        "files": {k: {"sha256": sha256(v), "bytes": len(v)}
                  for k, v in sorted(files.items())},
    }
    if verify_only:
        return report
    destination = Path(destination)
    if destination.exists():
        raise ValueError("Immutable reference folder already exists; refusing overwrite")
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="gate10-stage-", dir=destination.parent) as tmp:
        stage = Path(tmp)
        for name, content in files.items():
            relative = Path(name)
            if relative.is_absolute() or ".." in relative.parts:
                raise ValueError("Unsafe reference path in original archive")
            target = stage / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(content)
        (stage / "INTAKE_SHA256.json").write_text(
            json.dumps(report, indent=2) + "\n", encoding="utf-8")
        os.replace(stage, destination)
    return report

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--archive", required=True)
    parser.add_argument("--out", default=str(Path(__file__).resolve().parents[2] /
        "Saved" / "Gate12Readiness" / "Gate10FrozenReferenceOnly"))
    parser.add_argument("--verify-only", action="store_true")
    args = parser.parse_args()
    result = intake(args.archive, args.out, args.verify_only)
    print(json.dumps({"status": "GATE10_REFERENCE_HASH_VERIFIED",
        "references": len(result["files"]), "extracted": not args.verify_only,
        "target": args.out}))
