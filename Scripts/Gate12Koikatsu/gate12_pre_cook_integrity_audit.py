"""Read-only Gate12 pre-cook integrity audit (never cooks, modifies saves, or imports).
Usage: python Scripts/Gate12Koikatsu/gate12_pre_cook_integrity_audit.py
All results describe observable source/assets only, NOT player-visible acceptance.
"""
from __future__ import annotations

import json
import pathlib
import subprocess
from datetime import datetime, timezone

ROOT = pathlib.Path(__file__).resolve().parents[2]
BASE = ROOT / "Source" / "OfflineGame"
OUT = ROOT / "Saved" / "Gate12Readiness" / "integrity_audit.json"
FROZEN = "3f54f07cacbf193e160d0be15a7be5a7bc730c3b"


def git(*args: str, root: pathlib.Path = ROOT) -> str:
    result = subprocess.run(
        ["git", "-C", str(root), *args],
        check=True, capture_output=True, text=True, encoding="utf-8", errors="replace",
    )
    return result.stdout.strip()


def check_file(rel: str) -> dict:
    path = ROOT / rel
    return {"path": rel, "exists": path.is_file(), "size": path.stat().st_size if path.is_file() else 0}


def main() -> None:
    report: dict = {
        "schema": "GATE12_NONCOOK_SOURCE_AUDIT_V1",
        "utc": datetime.now(timezone.utc).isoformat(),
        "project_root": str(ROOT),
        "build_or_cook_invoked": False,
        "native_tests_invoked": False,
        "branch": git("branch", "--show-current"),
        "head": git("rev-parse", "HEAD"),
        "scope_note": "Gate09 authored animations excluded; artwork may arrive later. No fake Registry IDs.",
        "checks": {},
        "dependencies": {},
    }

    frozen_worktree = ROOT.parent / "Offline-Game"
    report["checks"]["frozen_foundation_is_unchanged"] = (
        git("rev-parse", "HEAD", root=frozen_worktree) == FROZEN
        if frozen_worktree.is_dir() else None
    )
    report["checks"]["frozen_foundation_clean"] = (
        git("status", "--porcelain", root=frozen_worktree) == ""
        if frozen_worktree.is_dir() else None
    )
    report["checks"]["production_branch"] = (
        report["branch"] == "production/gate12-koikatsu-ue583-fixture-20261010"
    )
    protected = git("status", "--porcelain=v1", "--", "Source/OfflineGame/Private/Koikatsu/OGKoikatsuPlayableCharacter.cpp",
                    "Source/OfflineGame/Public/Koikatsu/OGKoikatsuPlayableCharacter.h")
    report["checks"]["original_koikatsu_dirty_files_preserved"] = (
        "OGKoikatsuPlayableCharacter.cpp" in protected
        and "OGKoikatsuPlayableCharacter.h" in protected
    )

    native_sources = [
        "Source/OfflineGame/Private/UI/OGGate12NativeRulerWidget.cpp",
        "Source/OfflineGame/Public/UI/OGGate12CharacterVisualCatalog.h",
        "Source/OfflineGame/Private/UI/OGGate12CharacterVisualCatalog.cpp",
        "Source/OfflineGame/Public/UI/OGGate12CharacterPreviewStage.h",
        "Source/OfflineGame/Private/UI/OGGate12CharacterPreviewStage.cpp",
        "Source/OfflineGame/Private/Runtime/OGGate12RulerSessionSubsystem.cpp",
        "Source/OfflineGame/Private/UI/OGUiViewModelService.cpp",
    ]
    report["checks"]["source_files"] = [check_file(x) for x in native_sources]
    source_text = (BASE / "Private/UI/OGUiViewModelService.cpp").read_text(
        encoding="utf-8-sig", errors="replace"
    )
    view_text = (BASE / "Public/UI/OGUiViewModels.h").read_text(
        encoding="utf-8-sig", errors="replace"
    )
    report["checks"]["canonical_version_flows_into_roster"] = (
        "ManifestationView.ActiveVersionId =" in source_text
        and "Manifestation.ActiveVersionId;" in source_text
        and "FOGContentId ActiveVersionId;" in view_text
    )

    native_content = ROOT / "Content" / "Experimental" / "LuminousValentineV3"
    report["checks"]["protected_native_wardrobe_assets"] = {
        state: len(list((native_content / state).rglob("*.uasset")))
        for state in ("CLOTHED", "UNDERWEAR", "NUDE")
    }
    report["checks"]["approved_native_visual_catalog_assets"] = [
        str(x.relative_to(ROOT))
        for x in (ROOT / "Content").rglob("*CharacterVisualCatalog*.uasset")
    ]
    report["dependencies"]["gate11_accepted_identity_version_and_owned_packages"] = "NOT_ESTABLISHED"
    report["dependencies"]["gate09_authored_motion_import"] = "PENDING_GATE09"
    report["dependencies"]["gate02_first_authored_world"] = "NOT_ESTABLISHED"
    report["dependencies"]["gate10_final_nine_native_UMG_assets"] = "NOT_ESTABLISHED"
    report["dependencies"]["optional_2d_artwork"] = "MAY_BE_SUPPLIED_LATER"
    report["dependencies"]["approved_3d_preview_blueprints"] = (
        "PENDING_CATALOG_CONFIGURATION"
        if not report["checks"]["approved_native_visual_catalog_assets"]
        else "CATALOG_FILE_PRESENT_NOT_RUNTIME_VERIFIED"
    )

    # Source presence and an old APK do not prove runtime or Android readiness.
    report["is_ready_to_cook"] = False
    report["device_tested_by_this_audit"] = False
    report["status"] = "READ_ONLY_SOURCE_FACTS_ONLY"
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(str(OUT))
    print(json.dumps({
        "head": report["head"][:12],
        "foundation_unchanged": report["checks"]["frozen_foundation_is_unchanged"],
        "version_mapping": report["checks"]["canonical_version_flows_into_roster"],
        "wardrobe_native_asset_counts": report["checks"]["protected_native_wardrobe_assets"],
        "ready_to_cook": False,
    }, indent=2))


if __name__ == "__main__":
    main()
