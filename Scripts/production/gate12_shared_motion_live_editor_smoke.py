"""Non-destructive live UE shared-motion activation smoke.
Loads original AnimSequences and a matching original skeletal mesh, then
invokes the ACTUAL native UOGSharedMotionControllerComponent on a disposable
editor-world Actor. Does not save, overwrite or create asset packages.
Only reports PASS after PlayAction returns true on a real skeleton.
"""
import json
import pathlib
import traceback
import unreal

ROOT = pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
MANIFEST = ROOT / "Content/NoWorld/Animation/SharedMotionCatalog.json"
OUTPUT = ROOT / "Saved/Gate12Koikatsu/shared_motion_point1_live_smoke.json"
OUTPUT.parent.mkdir(parents=True, exist_ok=True)
result = {"status": "STARTED", "asset_cases": [], "live_controller": None,
          "error": None, "notes": []}
actor = None

def flush():
    OUTPUT.write_text(json.dumps(result, indent=2, ensure_ascii=False,
                                 default=str) + "\n", encoding="utf-8")

flush()
try:
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    enabled = [m for m in data["motions"] if m.get("approved_for_gameplay")]
    if len(enabled) < 16:
        raise RuntimeError("Missing previously enabled native source motions")
    checked = []
    for motion in enabled:
        asset = unreal.load_asset(motion["native_sequence"].split(".")[0])
        if not isinstance(asset, unreal.AnimSequence):
            raise RuntimeError("Not a real native AnimSequence: " + motion["motion_id"])
        skeleton = asset.get_editor_property("skeleton")
        if not skeleton:
            raise RuntimeError("Missing native AnimSequence Skeleton: " + motion["motion_id"])
        checked.append((motion, asset, skeleton))
        result["asset_cases"].append({
            "id": motion["motion_id"],
            "sequence": asset.get_path_name(),
            "skeleton": skeleton.get_path_name(),
            "native_duration": asset.get_play_length()
        })
    result["asset_load"] = len(checked)
    flush()

    # Use an imported Alter mesh actually sharing the chosen walk source Skeleton.
    folder = "/Game/NoWorld/Characters/Luminous/Alter/NativeOriginalClothed"
    target = next((x for x in checked
                   if x[0]["motion_id"] == "cmu.alter.CLOTHED.Walk"), None)
    if not target:
        raise RuntimeError("Clothed authentic Alter native walk not catalogued")
    matched_mesh = None
    for asset_path in unreal.EditorAssetLibrary.list_assets(folder, recursive=False):
        possible = unreal.load_asset(asset_path)
        if isinstance(possible, unreal.SkeletalMesh) and (
                possible.get_editor_property("skeleton") == target[2]):
            matched_mesh = possible
            break
    if not matched_mesh:
        raise RuntimeError("No original clothed Alter SkeletalMesh sharing real walk skeleton")
    result["matched_real_mesh"] = matched_mesh.get_path_name()
    flush()

    ctrl_class = unreal.load_class(None, "/Script/OfflineGame.OGSharedMotionControllerComponent")
    scene_class = unreal.load_class(None, "/Script/OfflineGame.OGSharedMotionSceneComponent")
    if not ctrl_class or not scene_class:
        raise RuntimeError("Native shared motion Controller/Scene UClass absent")

    # Use the EXISTING native Luminous character's constructor-owned skeletal
    # mesh and shared motion controller. No Python component fabrication.
    pawn_class = unreal.load_class(
        None, "/Script/OfflineGame.OGLuminousV3PlayableCharacter")
    if not pawn_class:
        raise RuntimeError("Canonical Luminous native character class absent")
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        pawn_class, unreal.Vector(500000, 500000, 500000))
    if not actor:
        raise RuntimeError("Could not allocate disposable Luminous native character")
    mesh_component = actor.get_component_by_class(
        unreal.SkeletalMeshComponent.static_class())
    ctrl = actor.get_component_by_class(ctrl_class)
    if not mesh_component or not ctrl:
        raise RuntimeError("Existing native character has no skeletal mesh/controller")
    mesh_component.set_skeletal_mesh_asset(matched_mesh)
    ctrl.set_editor_property("rig_profile", unreal.Name("koikatsu_original"))
    ctrl.set_editor_property("anatomy_profile", unreal.Name("humanoid"))
    ctrl.set_editor_property("outfit_profile", unreal.Name("CLOTHED"))
    ctrl.set_editor_property("capabilities", [unreal.Name("grounded")])
    if not ctrl.register_native_parts([mesh_component]):
        raise RuntimeError("RegisterNativeParts refused the original native mesh")
    action = unreal.Name("Traversal.Walk")
    context = unreal.Name("Traversal")
    if not ctrl.can_play_action(action, context, False, 1, unreal.Name("")):
        raise RuntimeError("Native exact-rig/action preflight rejected original mesh")
    if not ctrl.play_action(action, context, False, 1, unreal.Name("")):
        raise RuntimeError("Native component failed authentic walk playback")
    if not ctrl.is_playing_action():
        raise RuntimeError("Native component has no active action after play")
    result["live_controller"] = {
        "status": "PASS_NATIVE_PLAY_ACTION",
        "action": str(ctrl.get_editor_property("current_action")),
        "source_id": str(ctrl.get_editor_property("current_source_clip_id")),
        "rigged_mesh": matched_mesh.get_path_name(),
        "native_parts": ctrl.get_native_part_count()
    }
    result["status"] = "PASS_NATIVE_ACTION_STARTED"
except Exception as e:
    result["status"] = "FAIL"
    result["error"] = str(e)
    result["traceback"] = traceback.format_exc(limit=8)
    unreal.log_error("GATE12_SHARED_POINT1_RUNTIME_FAIL: " + str(e))
finally:
    try:
        if actor: actor.destroy_actor()
    except Exception as cleanup:
        result["notes"].append("Actor cleanup: " + str(cleanup))
    flush()
    unreal.log("GATE12_SHARED_POINT1_RESULT=" + result["status"])
