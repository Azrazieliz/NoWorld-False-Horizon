"""No-cook GPU QA frame with three point lights and distant isolated actor.
No source assets/levels altered, disposable actor/light/camera only.
"""
import pathlib,json,traceback,shutil,unreal
ROOT=pathlib.Path(unreal.Paths.project_saved_dir())/"P0CharacterQuality"
OUT=ROOT/"Rendered"
OUT.mkdir(parents=True,exist_ok=True)
out={"schema":"P0_GPU_STUDIO_LIGHT_SMOKE_V1","no_cook":True,"error":None}
try:
    q="/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/Maps/L_LuminousV3_Playable_RuntimeQA"
    k="/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/BP_LuminousValentineV3_CLOTHED_RuntimeQA.BP_LuminousValentineV3_CLOTHED_RuntimeQA_C"
    unreal.EditorLevelLibrary.load_level(q)
    a=unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.load_class(None,k),unreal.Vector(0,0,10000),unreal.Rotator(0,0,0),transient=True)
    if not a:raise RuntimeError("QA actor spawn failed")
    for c in a.get_components_by_class(unreal.SkeletalMeshComponent):
        if c.get_name()=="CharacterMesh0":
            c.set_visibility(False,True);c.set_hidden_in_game(True,True)
    lights=[]
    for pos,intensity in [((230,-130,10180),55000.0),
                          ((180,200,10150),25000.0),
                          ((-130,25,10250),40000.0)]:
        light=unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.PointLight,unreal.Vector(*pos),unreal.Rotator(0,0,0),transient=True)
        if not light:raise RuntimeError("QA transient light spawn failed")
        component=light.get_components_by_class(unreal.PointLightComponent)[0]
        component.set_editor_property("intensity",intensity)
        component.set_editor_property("attenuation_radius",900.)
        lights.append(light)
    eye_z=9992
    origin=unreal.Vector(360,0,eye_z)
    cam=unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SceneCapture2D,origin,
        unreal.MathLibrary.find_look_at_rotation(origin,unreal.Vector(0,0,eye_z)),
        transient=True)
    sc=cam.get_components_by_class(unreal.SceneCaptureComponent2D)[0]
    world=unreal.EditorLevelLibrary.get_editor_world()
    rt=unreal.RenderingLibrary.create_render_target2d(
        world,512,768,unreal.TextureRenderTargetFormat.RTF_RGBA8)
    sc.set_editor_property("texture_target",rt)
    sc.set_editor_property("capture_source",unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    sc.set_editor_property("fov_angle",31.0)
    sc.set_editor_property("capture_every_frame",False)
    sc.capture_scene()
    name="p0_gpu_smoke_studio_lighting"
    unreal.RenderingLibrary.export_render_target(world,rt,str(OUT),name)
    src=OUT/name
    if not src.is_file() or src.read_bytes()[:8]!=b"\x89PNG\r\n\x1a\n":
        raise RuntimeError("GPU capture output missing or not PNG")
    dst=OUT/(name+".png")
    shutil.copy2(src,dst)
    out.update({"status":"GPU_QA_STUDIO_FRAME_CREATED","png":str(dst),
                "bytes":dst.stat().st_size,"render_resolution":[512,768],
                "stage_distance_cm":360,"horizontal_fov":31,
                "source_gameplay_actors_modified":False})
except Exception as e:
    out["status"]="RENDER_FAILURE";out["error"]=str(e)
    out["traceback"]=traceback.format_exc()[-1700:]
(ROOT/"gpu_lit_smoke_result.json").write_text(json.dumps(out,indent=2)+"\n")
unreal.log("P0_LIT_GPU_RESULT "+json.dumps(out))
