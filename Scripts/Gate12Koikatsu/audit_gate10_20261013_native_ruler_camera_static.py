#!/usr/bin/env python3
"""Read-only Gate12 source-contract audit; NOT a UE UHT/UBT/device test."""
from __future__ import annotations
import argparse
import json
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument("--root", type=Path, required=True)
args = p.parse_args()
root = args.root.resolve()
files = {
    "native_header": "Source/OfflineGame/Public/UI/OGGate12NativeRulerWidget.h",
    "native_cpp": "Source/OfflineGame/Private/UI/OGGate12NativeRulerWidget.cpp",
    "session": "Source/OfflineGame/Private/Runtime/OGGate12RulerSessionSubsystem.cpp",
    "hud_header": "Source/OfflineGame/Public/World/OGStartingRegionPresentation.h",
    "hud_cpp": "Source/OfflineGame/Private/World/OGStartingRegionPresentation.cpp",
    "privacy": "Source/OfflineGame/Private/Koikatsu/OGLuminousV3PlayableCharacter.cpp",
    "privacy_test": "Source/OfflineGame/Private/Tests/OGGate12LuminousAppearancePrivacyTests.cpp",
}
source = {name: (root / path).read_text(encoding="utf-8")
          for name, path in files.items()}
checks = {
    "native_umg_widget_not_fullscreen_image":
        "UWidgetTree" not in source["native_header"]
        and "UOGGate12NativeRulerWidget" in source["native_cpp"]
        and "WidgetTree->ConstructWidget" in source["native_cpp"]
        and "UUniformGridPanel" in source["native_cpp"],
    "canonical_three_equal_roster_columns":
        "GetApprovedRosterColumns() { return 3; }" in source["native_header"]
        and "return 0.445f;" in source["native_header"]
        and "Grid->AddChildToUniformGrid(Fixed, Index / 3, Index % 3)" in source["native_cpp"]
        and "Fixed->SetWidthOverride(CardWidth)" in source["native_cpp"]
        and "Fixed->SetHeightOverride(CardHeight)" in source["native_cpp"],
    "five_native_navigation_destinations":
        all(needle in source["native_cpp"] for needle in
            ("TEXT(\"Home\")", "TEXT(\"Characters\")", "TEXT(\"Gacha\")",
             "TEXT(\"Territory\")", "TEXT(\"Records\")")),
    "no_fake_manifestations_or_character_chips":
        "UpsertCharacterManifestation(" not in source["native_cpp"]
        and "UpsertCharacterManifestation(" not in source["session"]
        and "SetChipBalance(" not in source["native_cpp"]
        and "SetChipBalance(" not in source["session"]
        and "OwnedRoster" in source["native_cpp"],
    "canonical_read_model_and_owned_selection":
        all(needle in source["session"] for needle in
            ("TryReadEntity(", "BuildRoster(", "BuildTerritory(",
             "BuildRecordsHub(", "UpsertManifestationContextSelection(")),
    "ruler_umg_safe_fallback_and_world_delegate":
        all(needle in source["hud_cpp"] for needle in
            ("SynchronizeGate12NativeRulerUMG", "OnGate12NativeWorldRequested",
             "Gate12NativeRulerWidget->AddToViewport(50)",
             "Gate12NativeRulerWidget->RemoveFromParent()")),
    "camera_close_zoom_pinch_and_reserved_controls":
        all(needle in source["hud_cpp"] for needle in
            ("CameraBoom->TargetArmLength = 395.0f;",
             "bCameraPinchActive", "AdjustThirdPersonCameraZoom(",
             "bMovementZone", "bActionZone", "bTopUi")),
    "privacy_render_not_actual_outfit_mutation":
        all(needle in source["privacy"] for needle in
            ("FOGLuminousV3AppearancePolicy::Resolve",
             "EffectiveAppearance.EffectiveRenderedWardrobe",
             "CurrentWardrobe = Desired;",
             "RefreshPrivacyRenderedAppearance()"))
        and "PrivacyDoesNotMutateCanonicalOutfitOrOwnership" in source["privacy_test"],
}
payload = {
    "status": "PASS_STATIC_SOURCE_CONTRACT" if all(checks.values())
              else "FAIL_STATIC_SOURCE_CONTRACT",
    "important": "Static text presence only. UHT, C++ compile, native behavior, "
                 "portrait layout, Android rendering and Gate11 acceptance NOT_TESTED.",
    "checks": checks,
    "source_root": str(root),
}
print(json.dumps(payload, indent=2))
raise SystemExit(0 if all(checks.values()) else 1)
