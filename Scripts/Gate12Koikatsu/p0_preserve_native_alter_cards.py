"""P0 read-only source verification + immutable copy of user's Alter Party cards.
Never opens/modifies/resaves the source character card. Identity/wardrobe
assignment remains unverified until Gate01 inspects embedded KK coordinates.
"""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile
from datetime import datetime, timezone

NAMES = [
    "Koikatu_F_20261013224249541_Valentine Luminous.png",
    "Koikatu_F_20261013224655353_Valentine Luminous.png",
    "Koikatu_F_20261013225235666_Valentine Luminous.png",
]
SOURCE = Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party\UserData\chara\female")
ZIP = Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\LUMINOUS_VALENTINE_BASE_STATES_SAFE_20261011.zip")
ROOT = Path(__file__).resolve().parents[2] / "Saved" / "P0CharacterQuality" / "source_preservation"


def hash_file(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def frozen_copy(src: Path, dst: Path, expected: str):
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists():
        if hash_file(dst) != expected:
            raise RuntimeError("Existing immutable backup disagrees: " + str(dst))
        return "ALREADY_PRESERVED"
    fd, temporary = tempfile.mkstemp(prefix=".preserve_", dir=dst.parent)
    try:
        with os.fdopen(fd, "wb") as out, src.open("rb") as original:
            shutil.copyfileobj(original, out, 1024 * 1024)
            out.flush()
            os.fsync(out.fileno())
        if hash_file(Path(temporary)) != expected:
            raise RuntimeError("Original changed mid-copy or backup write incomplete")
        os.replace(temporary, dst)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
    return "SNAPSHOT_CREATED"


def run():
    ROOT.mkdir(parents=True, exist_ok=True)
    result = {
        "status": "PENDING",
        "date_utc": datetime.now(timezone.utc).isoformat(),
        "originals_not_modified": True,
        "alter_state_assignments": "UNVERIFIED_GATE01_EMBEDDED_COORDINATES",
        "alter_identity": "PENDING_GATE01_USER_AUTHORED_APPEARANCE_VERIFICATION",
        "cards": [],
        "baseline": None,
        "errors": [],
    }
    for name in NAMES:
        try:
            src = SOURCE / name
            if not src.is_file():
                raise FileNotFoundError(str(src))
            before = hash_file(src)
            stats = src.stat()
            dst = ROOT / "alter_cards" / before / name
            copy_status = frozen_copy(src, dst, before)
            after = hash_file(src)
            if before != after:
                raise RuntimeError("Original changed during preservation; investigate immediately")
            result["cards"].append({
                "source_path": str(src),
                "source_sha256": before,
                "source_bytes": stats.st_size,
                "source_last_modified_utc": datetime.fromtimestamp(
                    stats.st_mtime, timezone.utc).isoformat(),
                "immutable_backup": str(dst),
                "backup_sha256": hash_file(dst),
                "copy_status": copy_status,
                "appearance_state": "UNVERIFIED",
            })
        except Exception as exc:
            result["errors"].append(f"{name}: {exc}")
    try:
        if not ZIP.is_file():
            raise FileNotFoundError(str(ZIP))
        h = hash_file(ZIP)
        dst = ROOT / "gate00_base_archive" / h / ZIP.name
        state = frozen_copy(ZIP, dst, h)
        result["baseline"] = {
            "source_path": str(ZIP),
            "source_sha256": h,
            "source_bytes": ZIP.stat().st_size,
            "immutable_backup": str(dst),
            "backup_sha256": hash_file(dst),
            "copy_status": state,
        }
    except Exception as exc:
        result["errors"].append(f"V3 baseline ZIP: {exc}")
    result["status"] = "HASH_LOCKED_APPEARANCE_UNVERIFIED" if not result["errors"] else "INCOMPLETE"
    report = ROOT / "preservation_receipt.json"
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))
    return 0 if not result["errors"] else 2


if __name__ == "__main__":
    raise SystemExit(run())
