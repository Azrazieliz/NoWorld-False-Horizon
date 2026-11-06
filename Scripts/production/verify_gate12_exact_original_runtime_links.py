#!/usr/bin/env python3
"""Strict source acceptance: actual 18 native UE assets, roster/World classes and no proxy.
Checks imported original-card packages and no fake Grand avatar / fifth ordinary lineage.
This is a source contract check, not native compilation, visual test or Android acceptance.
"""
import json,pathlib,sys
R=pathlib.Path(__file__).resolve().parents[2]
proof=json.loads((R/"docs/production/gate12/evidence/verified-original-koikatsu-native/G12_Awakened_Berserker_ExactOriginal_NativeUEImportAccepted.json").read_text("utf8"))
registry=(R/"Source/OfflineGame/Private/Koikatsu/OGExactOriginalLuminousParts.cpp").read_text("utf8")
world=(R/"Source/OfflineGame/Private/Koikatsu/OGLuminousV3PlayableCharacter.cpp").read_text("utf8")
header=(R/"Source/OfflineGame/Public/Koikatsu/OGLuminousV3PlayableCharacter.h").read_text("utf8")
preview=(R/"Source/OfflineGame/Private/Koikatsu/OGLuminousExactOriginalPreviewActor.cpp").read_text("utf8")
catalog=(R/"Source/OfflineGame/Private/UI/OGGate12CharacterVisualCatalog.cpp").read_text("utf8")
assert proof["status"]=="PASS_AWAKENED_AND_BERSERKER_TRUE_ORIGINAL_COMPLETE_NATIVE_UE_SKINNED_SOURCES_NOT_DEVICE_ACCEPTED"
for form,bones in (("Awakened",1073),("Berserker",882)):
    data=proof["forms"][form]
    assert data["complete_native_3d_character_source"] and data["rig_bones_source"]==bones
    assert data["native_skeletal_mesh_count"]==9
    assert data["complete_source_body_coverage"]
    assert len(data["installed_native_parts"])==9
    for part in data["installed_native_parts"]:
        assert part["native_skeletal_mesh_saved_and_built"]
        for field in ("skeletal_mesh","skeleton"):
            asset=part[field]
            assert asset and asset.startswith(f"/Game/NoWorld/Characters/Luminous/{form}/ExactOriginalClothed/")
            binary=R/"Content"/(asset.removeprefix("/Game/")+".uasset")
            assert binary.is_file() and binary.stat().st_size>500
            assert asset+"."+asset.rsplit("/",1)[-1] in registry if field=="skeletal_mesh" else True
    assert f"AOGLuminous{form}OriginalWorldCharacter::AOGLuminous{form}OriginalWorldCharacter()" in world
    assert f"class OFFLINEGAME_API AOGLuminous{form}OriginalWorldCharacter" in header
    assert f"OGLuminous{form}OriginalWorldCharacter" in catalog
assert registry.count("TrueOriginalPart")>=10
assert "character:luminous_valentine.version_grand_v4" not in registry
assert "OGLuminousExactOriginalPreviewActor" in catalog
assert "TryReadCharacterManifestation" in preview and "OwningRulerId != RulerId" in preview
assert "RegisterNativeParts(Components)" in preview and "Motion->BindCanonicalCharacter(IdentityId)" in preview
assert "bGroupPartsByReferencePose = true" in preview
assert "bGroupPartsByReferencePose = true" in world
assert "bVerifiedExactClothed" in world
for form in ("Awakened","Berserker"):
    ini=(R/"Config/DefaultGame.ini").read_text("utf8")
    assert f'/Game/NoWorld/Characters/Luminous/{form}/ExactOriginalClothed' in ini
assert not proof["android_visual_acceptance"] and not proof["native_unreal_game_build_acceptance"]
print("GATE12_EXACT_ORIGINAL_NATIVE_WORLD_ROSTER_SOURCE_LINKS_PASS: 18 real UE imported meshes, correct ownership, whole-character motion, no Grand proxy")
