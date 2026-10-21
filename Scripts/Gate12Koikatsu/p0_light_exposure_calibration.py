"""Transient UE5.8.3 no-cook lighting/exposure sweep on same skinned QA actor.
Capture actual GPU frames at fixed camera before any material fidelity claims.
"""
import json,pathlib,shutil,traceback,unreal
ROOT=pathlib.Path(unreal.Paths.project_saved_dir())/"P0CharacterQuality"
F=ROOT/"Rendered";F.mkdir(parents=True,exist_ok=True)
result={"schema":"P0_LIGHT_EXPOSURE_SWEEP_V1","no_cook":True,"frames":[],"errors":[]}
try:
    unreal.EditorLevelLibrary.load_level("/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/Maps/L_LuminousV3_Playable_RuntimeQA")
    kl=unreal.load_class(None,"/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/BP_LuminousValentineV3_CLOTHED_RuntimeQA.BP_LuminousValentineV3_CLOTHED_RuntimeQA_C")
    a=unreal.EditorLevelLibrary.spawn_actor_from_class(
        kl,unreal.Vector(0,0,10000),unreal.Rotator(),transient=True)
    if not a:raise RuntimeError("QA actor missing")
    for x in a.get_components_by_class(unreal.SkeletalMeshComponent):
        if x.get_name()=="CharacterMesh0":
            x.set_visibility(False,True)
            x.set_hidden_in_game(True,True)
    lamps=[]
    for xyz in ((230,-130,10180),(180,200,10150),(-130,25,10250)):
        lamp=unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.PointLight,unreal.Vector(*xyz),unreal.Rotator(),transient=True)
        component=lamp.get_components_by_class(unreal.PointLightComponent)[0]
        component.set_editor_property("attenuation_radius",900.)
        lamps.append(component)
    loc=unreal.Vector(360,0,9992)
    camera=unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SceneCapture2D,loc,
        unreal.MathLibrary.find_look_at_rotation(loc,unreal.Vector(0,0,9992)),
        transient=True)
    cap=camera.get_components_by_class(unreal.SceneCaptureComponent2D)[0]
    cap.set_editor_property("capture_every_frame",False)
    cap.set_editor_property("capture_source",unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    cap.set_editor_property("fov_angle",31.)
    try:
        pp=cap.get_editor_property("post_process_settings")
        pp.set_editor_property("b_override_auto_exposure_method",True)
        pp.set_editor_property("auto_exposure_method",unreal.AutoExposureMethod.AEM_MANUAL)
        cap.set_editor_property("post_process_settings",pp)
        cap.set_editor_property("post_process_blend_weight",1.)
        result["fixed_exposure"]=True
    except Exception as ex:
        result["fixed_exposure"]=False
        result["exposure_warning"]=str(ex)
    world=unreal.EditorLevelLibrary.get_editor_world()
    rt=unreal.RenderingLibrary.create_render_target2d(
        world,512,768,unreal.TextureRenderTargetFormat.RTF_RGBA8)
    cap.set_editor_property("texture_target",rt)
    for level in (300,800,1800,4000):
        for lamp,frac in zip(lamps,(1.,0.48,0.65)):
            lamp.set_editor_property("intensity",float(level)*frac)
        cap.capture_scene()
        name="P0_StudioSweep_%04d"%level
        unreal.RenderingLibrary.export_render_target(world,rt,str(F),name)
        raw=F/name
        if not raw.is_file() or raw.read_bytes()[:4]!=bytes([137,80,78,71]):
            raise RuntimeError("Missing actual rendered PNG: "+name)
        dst=F/(name+".png")
        shutil.copy2(raw,dst)
        result["frames"].append({"label":name,"key_intensity":level,
                                 "path":str(dst),"bytes":dst.stat().st_size,
                                 "camera_cm":360,"horizontal_fov":31})
    result["status"]="FRAMES_RENDERED_REVIEW_REQUIRED"
except Exception as ex:
    result["status"]="FAILED";result["errors"].append(str(ex))
    result["traceback"]=traceback.format_exc()[-1700:]
(ROOT/"studio_lighting_sweep_result.json").write_text(json.dumps(result,indent=2)+"\n")
unreal.log("P0_LIGHT_SWEEP "+str(result["status"])+" FRAME_COUNT="+str(len(result["frames"])))
