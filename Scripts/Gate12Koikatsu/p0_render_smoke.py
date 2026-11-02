"""Actual GPU RHI no-cook smoke test of Luminous QA actor.
No map saves, no production assets mutated, no screenshot fabrication.
"""
import pathlib,json,traceback,unreal,os
OUT=pathlib.Path(unreal.Paths.project_saved_dir())/"P0CharacterQuality"/"Rendered"
OUT.mkdir(parents=True,exist_ok=True)
result={"schema":"P0_NATIVE_RENDER_SMOKE_V1","no_cook":True,"output":str(OUT),
        "frame":None,"error":None}
try:
    map_name="/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/Maps/L_LuminousV3_Playable_RuntimeQA"
    klass=unreal.load_class(None,
        "/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/BP_LuminousValentineV3_CLOTHED_RuntimeQA.BP_LuminousValentineV3_CLOTHED_RuntimeQA_C")
    if not klass:raise RuntimeError("Existing QA Luminous actor class not loaded.")
    unreal.EditorLevelLibrary.load_level(map_name)
    char=unreal.EditorLevelLibrary.spawn_actor_from_class(
        klass,unreal.Vector(0,0,120),unreal.Rotator(0,0,0),transient=True)
    if not char:raise RuntimeError("Could not spawn existing QA Luminous.")
    original_count=len(char.get_components_by_class(unreal.SkeletalMeshComponent))
    for c in char.get_components_by_class(unreal.SkeletalMeshComponent):
        if c.get_name()=="CharacterMesh0":
            c.set_visibility(False,True)
            c.set_hidden_in_game(True,True)
    loc=unreal.Vector(360,0,112)
    target=unreal.Vector(0,0,112)
    rot=unreal.MathLibrary.find_look_at_rotation(loc,target)
    camera=unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SceneCapture2D,loc,rot,transient=True)
    if not camera:raise RuntimeError("Could not spawn transient SceneCapture2D.")
    cc=camera.get_components_by_class(unreal.SceneCaptureComponent2D)
    if not cc:raise RuntimeError("SceneCapture2D component absent.")
    capture=cc[0]
    world=unreal.EditorLevelLibrary.get_editor_world()
    rt=unreal.RenderingLibrary.create_render_target2d(
        world,512,768,unreal.TextureRenderTargetFormat.RTF_RGBA8)
    capture.set_editor_property("texture_target",rt)
    capture.set_editor_property("capture_source",unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    capture.set_editor_property("fov_angle",31.0)
    capture.set_editor_property("capture_every_frame",False)
    capture.capture_scene()
    unreal.RenderingLibrary.export_render_target(
        world,rt,str(OUT),"p0_gpu_smoke_roster")
    frame=OUT/"p0_gpu_smoke_roster.png"
    result.update({"frame":str(frame),"exists":frame.is_file(),
                   "bytes":frame.stat().st_size if frame.is_file() else 0,
                   "QA_skeletal_parts":original_count,
                   "character_tutorial_mesh_hidden":True})
except Exception as e:
    result["error"]=str(e)
    result["traceback"]=traceback.format_exc()[-1700:]
result_path=OUT.parent/"gpu_render_smoke_result.json"
result_path.write_text(json.dumps(result,indent=2)+"\n")
unreal.log("P0_GPU_RENDER_SMOKE "+json.dumps(result))
