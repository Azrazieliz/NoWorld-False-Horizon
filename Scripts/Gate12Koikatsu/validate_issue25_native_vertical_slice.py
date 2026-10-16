"""Issue #25 — actual Unreal 5.8.3 native asset inspection without cooking.
No canonical ownership is invented, no gameplay/world/save data is modified.
The C++ editor automation suite separately checks exact QA class inheritance.
"""
import json
from pathlib import Path
import unreal

PROJECT = Path(unreal.Paths.project_saved_dir())
OUT = PROJECT / "Gate12Readiness" / "issue25_native_vertical_slice.json"
QA_CLASS = ("/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/"
            "BP_LuminousValentineV3_CLOTHED_RuntimeQA."
            "BP_LuminousValentineV3_CLOTHED_RuntimeQA_C")
UMG_CLASS = "/Game/NoWorld/UI/WBP_NoWorld_RulerShell.WBP_NoWorld_RulerShell_C"
CATALOG_PATH = "/Game/NoWorld/UI/DA_NoWorld_CharacterVisuals"
WORLD_PATH = "/Game/Maps/StartingWorld"
REPORT = {
    "schema": "GATE12_ISSUE25_NATIVE_VERTICAL_SLICE_PREFLIGHT_V2",
    "no_cook": True, "no_install": True, "no_package": True,
    "no_save_or_registry_mutation": True,
    "qa_actor_spawned": False, "canonical_owner_created": False,
    "evidence": {}, "failures": [],
    "not_proven": [
        "Actual local-controller UMG navigation/World input in PIE or Android",
        "Actual source part draw/pose, lighting, camera and 3D frame visual parity",
        "13 simultaneous visible/animated adult participants",
        "Any accepted Gate11 playable identity/version or owned Manifestation",
        "Android device performance, cook, installation",
    ],
}


def verify(key, good):
    REPORT["evidence"][key] = bool(good)
    if not good:
        REPORT["failures"].append(key)


try:
    native = unreal.load_class(None, "/Script/OfflineGame.OGGate12NativeRulerWidget")
    compiled_umg = unreal.load_class(None, UMG_CLASS)
    ruler = unreal.load_class(None,
        "/Script/OfflineGame.OGGate12RulerSessionSubsystem")
    stage = unreal.load_class(None,
        "/Script/OfflineGame.OGGate12CharacterPreviewStage")
    interaction = unreal.load_class(None,
        "/Script/OfflineGame.OGFoundationInteractionHost")
    verify("native_umg_class", bool(native))
    verify("compiled_widget_blueprint_class", bool(compiled_umg))
    verify("authoritative_ruler_session_class", bool(ruler))
    verify("passive_preview_stage_native_class", bool(stage))
    verify("existing_systemic_interaction_host_class", bool(interaction))

    qa = unreal.load_class(None, QA_CLASS)
    verify("exact_real_imported_qa_blueprint", bool(qa and
        qa.get_path_name() == QA_CLASS))
    REPORT["evidence"]["qa_blueprint_source_path"] = QA_CLASS
    REPORT["evidence"]["qa_asset_is_not_canonical"] = True

    # Check every genuine imported CLOTHED part; no actor/Pawn spawn.
    full_parts = []
    for mesh, max_part in ((0,2),(1,0),(2,0),(3,11),(4,11),(5,15)):
        for prim in range(max_part + 1):
            n = "Mesh_%02d_Prim_%02d" % (mesh, prim)
            folder = "Mesh%03dPrim%03d" % (mesh, prim)
            path = ("/Game/Experimental/LuminousValentineV3/CLOTHED/Parts/"
                    + folder + "/" + n + "/SkeletalMeshes/" + n)
            obj = unreal.load_asset(path)
            full_parts.append({
                "path": path, "loaded": bool(obj),
                "type": obj.get_class().get_name() if obj else None,
            })
    REPORT["evidence"]["loaded_part_count"] = sum(int(p["loaded"]) for p in full_parts)
    REPORT["evidence"]["expected_part_count"] = len(full_parts)
    REPORT["evidence"]["missing_part_paths"] = [
        p["path"] for p in full_parts if not p["loaded"]]
    verify("all_45_actual_clothed_skeletal_parts_installed",
           len(full_parts) == 45 and all(p["type"] == "SkeletalMesh"
                                        for p in full_parts))

    catalog = unreal.EditorAssetLibrary.load_asset(CATALOG_PATH)
    verify("native_visual_catalog_asset", bool(catalog and
        catalog.get_class().get_name() == "OGGate12CharacterVisualCatalog"))
    if catalog:
        entries = catalog.get_editor_property("entries")
        REPORT["evidence"]["gate11_accepted_visual_catalog_entries"] = len(entries)
        verify("no_fabricated_canonical_character", len(entries) == 0)

    level = unreal.EditorAssetLibrary.load_asset(WORLD_PATH)
    verify("starting_world_asset_exists", bool(level))
except Exception as exc:
    REPORT["failures"].append("exception:" + str(exc))
REPORT["status"] = ("NATIVE_ASSET_PREFLIGHT_PASS_NO_COOK"
                    if not REPORT["failures"] else "FAILED")
OUT.parent.mkdir(parents=True,exist_ok=True)
OUT.write_text(json.dumps(REPORT,indent=2)+"\n",encoding="utf-8")
unreal.log("GATE12_ISSUE25_PREFLIGHT " + json.dumps(
    {"status":REPORT["status"],"failures":REPORT["failures"],
     "result":str(OUT)}))
