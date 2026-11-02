"""Read-only design-reference intake for corrected Gate10 2026-10-17 ZIP.

NEVER use frozen screenshots or crops as production art. Every depicted
character/region or example number is noncanonical. The opening is animated
and has NO approved final static background screenshot.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile
import zipfile

CORRECTED_SHA256 = "565e66b75d2e3968af6f4894a59305d00e948d74499eb201ae7373a368fd6ad5"
PREFIX = "01_APPROVED_VISUAL_REFERENCES/"
ICON = PREFIX + "08_Icon_FINAL_USER_SELECTED_EXACT_818455.png"
REQUIRED = (
    "00_START_HERE_READ_FIRST.md",
    "00_CRITICAL_GLOBAL_INTERPRETATION_RULES.md",
    PREFIX + "01_World_HUD_LANDSCAPE_APPROVED.png",
    PREFIX + "02_Ruler_Home_PORTRAIT_APPROVED.png",
    PREFIX + "03_Records_PORTRAIT_APPROVED_PLACEHOLDER_TEXT.png",
    PREFIX + "04_Gacha_PORTRAIT_APPROVED.png",
    PREFIX + "05_Territory_MAP_A_APPROVED.png",
    PREFIX + "06_Territory_MAP_B_APPROVED.png",
    PREFIX + "07_Territory_MAP_C_APPROVED.png",
    ICON,
    "05_FROZEN_SPECIFICATIONS/OPENING_APPROVED_SPEC.md",
    "05_FROZEN_SPECIFICATIONS/FINAL_SCREEN_AND_BRANDING_FREEZE.md",
    "05_FROZEN_SPECIFICATIONS/RARITY_APPROVED_SPEC.md",
)

def checksum(content):
    return hashlib.sha256(content).hexdigest()

def intake(archive, destination, verify_only=False):
    archive = Path(archive)
    if not archive.is_file():
        raise FileNotFoundError("Corrected Gate10 2026-10-17 ZIP not found")
    if checksum(archive.read_bytes()) != CORRECTED_SHA256:
        raise ValueError("Wrong/withdrawn Gate10 archive: SHA256 mismatch")
    selected = {}
    with zipfile.ZipFile(archive, "r") as master:
        manifest = json.loads(master.read("SHA256_MANIFEST.json"))
        if manifest.get("algorithm") != "sha256":
            raise ValueError("Unexpected Gate10 checksum algorithm")
        hashes = manifest["files"]
        for name, expected in hashes.items():
            content = master.read(name)
            if checksum(content) != expected:
                raise ValueError("Incorrect Gate10 member checksum: " + name)
        for path in REQUIRED:
            if path not in hashes:
                raise ValueError("Missing corrected frozen source: " + path)
        for path in sorted(hashes):
            selected[path] = master.read(path)
    report = {
        "schema": "GATE10_CORRECTED_LAYOUT_REFERENCE_ONLY_V2",
        "master_sha256": CORRECTED_SHA256,
        "master_bytes": archive.stat().st_size,
        "file_count": len(selected),
        "frozen_screens_are_layout_only": True,
        "mockup_character_scene_and_artwork_discarded": True,
        "no_art_style_or_game_data_inferred": True,
        "no_static_opening_asset_approved": True,
        "app_icon_only_exception": ICON,
        "production_actors_or_UMG_installed": False,
        "files": {p: {"sha256": checksum(v), "bytes": len(v)}
                  for p, v in sorted(selected.items())},
    }
    if verify_only:
        return report
    destination = Path(destination)
    if destination.exists():
        raise FileExistsError(
            "Reference destination already exists; source files cannot be overwritten")
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="gate10-corrected-", dir=destination.parent) as tmp:
        stage = Path(tmp)
        for path, content in selected.items():
            relative = Path(path)
            if relative.is_absolute() or ".." in relative.parts:
                raise ValueError("Unsafe corrected Gate10 path")
            target = stage / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(content)
        (stage / "INTAKE_SHA256.json").write_text(
            json.dumps(report, indent=2) + "\n", encoding="utf-8")
        os.replace(stage, destination)
    return report

if __name__ == "__main__":
    cli = argparse.ArgumentParser()
    cli.add_argument("--archive", required=True)
    cli.add_argument("--out", default=str(Path(__file__).resolve().parents[2] /
        "Saved" / "Gate12Readiness" / "Gate10CorrectedReferenceOnly_20261017"))
    cli.add_argument("--verify-only", action="store_true")
    args = cli.parse_args()
    result = intake(args.archive, args.out, args.verify_only)
    print(json.dumps({
        "status": "CORRECTED_GATE10_EXACT_LAYOUT_REFERENCE_VERIFIED",
        "files": result["file_count"],
        "icon_sha256": result["files"][ICON]["sha256"],
        "staged": not args.verify_only,
    }))
