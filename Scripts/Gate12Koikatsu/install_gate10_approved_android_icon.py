"""Install the EXACT frozen Gate10 launcher resources as source-only Android overlay.

Never invokes UAT, cook, package, sign or adb.
Requires approved ZIP on the workstation; refuses substituted/recompressed images.
Default is dry-run. Source app icons are not accepted merely because this runs.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import tempfile
import zipfile

APPROVED_ARCHIVE_SHA256 = (
    "fb6f5eaac4583ae9ceab8a6f54847d75bf8b2172d2d4d3e9071fd78981c4381d"
)
APPROVED_ORIGINAL_IMAGE_SHA256 = (
    "61b0ca2d5ddaecc8acf7139ed00867851b2a5edb9b07b58954732c417eadef46"
)


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--archive", required=True, type=Path)
    parser.add_argument("--project", type=Path,
                        default=Path(__file__).resolve().parents[2])
    parser.add_argument("--apply", action="store_true",
                        help="Copy exact resources into Build/Android/res without cooking")
    args = parser.parse_args()
    root = args.project.resolve()
    overlay = root / "Build" / "Android" / "res"
    receipt = root / "Saved" / "Gate12Readiness" / "gate10_android_icon_receipt.json"
    report = {
        "schema": "GATE12_GATE10_EXACT_ICON_IMPORT_V1",
        "source": str(args.archive),
        "source_sha256": None,
        "mode": "source_overlay_only" if args.apply else "dry_run",
        "cooked": False,
        "signed": False,
        "deployed": False,
        "verified_files": [],
        "status": "NOT_INSTALLED",
    }
    try:
        raw = args.archive.read_bytes()
        report["source_sha256"] = sha(raw)
        if sha(raw) != APPROVED_ARCHIVE_SHA256:
            raise ValueError("Unapproved or modified icon archive. Refusing installation.")
        with zipfile.ZipFile(args.archive) as archive:
            manifest = json.loads(archive.read("manifest.json"))
            if manifest.get("source_sha256") != APPROVED_ORIGINAL_IMAGE_SHA256:
                raise ValueError("The original Gate10 icon SHA-256 does not match.")
            for rel, expected in manifest["files"].items():
                if sha(archive.read(rel)) != expected:
                    raise ValueError("An approved icon resource has been altered: " + rel)
            resources = [(name, archive.read(name)) for name in manifest["files"]
                         if name.startswith("res/")]
            for name, data in resources:
                rel = PurePosixPath(name)
                if any(part in ("..", ".", "") for part in rel.parts):
                    raise ValueError("Unsafe Android overlay path: " + name)
                target = overlay.joinpath(*rel.parts[1:])
                if target.is_file() and sha(target.read_bytes()) != sha(data):
                    raise ValueError(
                        "Existing launcher resource conflicts: " + str(target)
                        + ". Preserve existing files and reconcile explicitly.")
                report["verified_files"].append({
                    "source": name, "overlay": str(target), "sha256": sha(data)
                })
            if args.apply:
                for name, data in resources:
                    target = overlay.joinpath(*PurePosixPath(name).parts[1:])
                    target.parent.mkdir(parents=True, exist_ok=True)
                    if target.is_file():
                        continue
                    handle, temporary = tempfile.mkstemp(
                        dir=target.parent, prefix=".approved_gate10_")
                    try:
                        with os.fdopen(handle, "wb") as stream:
                            stream.write(data)
                            stream.flush()
                            os.fsync(stream.fileno())
                        os.replace(temporary, target)
                    finally:
                        if os.path.exists(temporary):
                            os.unlink(temporary)
                report["status"] = "SOURCE_OVERLAY_INSTALLED_NOT_COOKED"
            else:
                report["status"] = "VERIFIED_DRY_RUN_NOT_INSTALLED"
    except Exception as exc:
        report["error"] = str(exc)
        report["status"] = "BLOCKED"
    receipt.parent.mkdir(parents=True, exist_ok=True)
    receipt.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))
    return 0 if report["status"] not in ("BLOCKED",) else 2


if __name__ == "__main__":
    raise SystemExit(main())
