"""NO-COOK actual GPU Unreal render capture in transient experimental QA map.
Captures same genuine Luminous QA actor under current/near camera geometry and
isolated upper-eyeline corrected material. Does NOT claim production roster/world
or Gate00 art approval. No map saves or original material changes.
"""
import pathlib,json,traceback,shutil,unreal
ROOT=pathlib.Path(unreal.Paths.project_saved_dir())/"P0CharacterQuality"
FRAMES=ROOT/"Rendered"
FRAMES.mkdir(parents=True,exist_ok=True)
QA="/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/Maps/L_LuminousV3_Playable_RuntimeQA"
CLS="/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/BP_LuminousValentineV3_CLOTHED_RuntimeQA.BP_LuminousValentineV3_CLOTHED_RuntimeQA_C"
MATERIAL="/Game/Experimental/Gate12P0MaterialQA/M_P0_Eyeline_AlphaBlend_2S"
result={"schema":"P0_UNREAL_RENDER_COMPARATIVE_V1","no_cook":True,
        "source":"GENUINE_EXPERIMENTAL_QA_ACTOR_NOT_GATE11_PRODUCTION",
        "frame_specs":[],"errors":[],"material_only_transient":True}
def capture(world,actor,label,dist,fov,w,h,aim_z,camera,comp):
    eye_z=10000 + (aim_z - 120)
    location=unreal.Vector(dist,0,eye_z)
    target=unreal.Vector(0,0,eye_z)
    camera.set_actor_location(location,False,False)
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location,target))
    rt=unreal.RenderingLibrary.create_render_target2d(
        world,w,h,unreal.TextureRenderTargetFormat.RTF_RGBA8)
    comp.set_editor_property("texture_target",rt)
    comp.set_editor_property("capture_source",unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    comp.set_editor_property("fov_angle",float(fov))
    comp.set_editor_property("capture_every_frame",False)
    comp.capture_scene()
    unreal.RenderingLibrary.export_render_target(world,rt,str(FRAMES),label)
    original_file=FRAMES/label
    if original_file.is_file() and original_file.read_bytes()[:4]==bytes([137,80,78,71]):
        shutil.copy2(original_file,FRAMES/(label+".png"))
    files=[x for x in FRAMES.glob(label+".png") if x.is_file()]
    if not files:raise RuntimeError("Unreal GPU frame missing: "+label)
    row={"label":label,"distance_cm":dist,"horizontal_fov":fov,
         "dimensions_px":[w,h],"aim_z_cm":aim_z,
         "files":[{"path":str(x),"bytes":x.stat().st_size} for x in files]}
    result["frame_specs"].append(row)
    unreal.log("P0_FRAME "+json.dumps(row))
try:
    klass=unreal.load_class(None,CLS)
    mat=unreal.load_asset(MATERIAL)
    if not klass or not mat:raise RuntimeError("Required existing QA Blueprint/material unavailable.")
    unreal.EditorLevelLibrary.load_level(QA)
    actor=unreal.EditorLevelLibrary.spawn_actor_from_class(
        klass,unreal.Vector(0,0,10000),unreal.Rotator(0,0,0),transient=True)
    world=unreal.EditorLevelLibrary.get_editor_world()
    if not actor or not world:raise RuntimeError("Transient QA world actor missing.")
    for pos,intensity in [((230,-130,10180),300.0),((180,200,10150),144.0),((-130,25,10250),195.0)]:
        light=unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.PointLight,unreal.Vector(*pos),unreal.Rotator(0,0,0),transient=True)
        if not light:raise RuntimeError("Transient QA studio light failed")
        light.get_components_by_class(unreal.PointLightComponent)[0].set_editor_property("intensity",intensity)
        light.get_components_by_class(unreal.PointLightComponent)[0].set_editor_property("attenuation_radius",900.)
    generic=None;eyeline=None
    for component in actor.get_components_by_class(unreal.SkeletalMeshComponent):
        if component.get_name()=="CharacterMesh0":generic=component
        elif "LV3_C_M03P05" in component.get_name():eyeline=component
    if not generic or not eyeline:
        raise RuntimeError("Source QA actor skeletal parts have changed.")
    result["source_generic_component_originally_visible"]=bool(generic.is_visible())
    camera=unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SceneCapture2D,unreal.Vector(360,0,112),unreal.Rotator(0,180,0),
        transient=True)
    if not camera:raise RuntimeError("GPU SceneCapture2D actor creation failed.")
    component=camera.get_components_by_class(unreal.SceneCaptureComponent2D)[0]
    # Reference shot explicitly shows any inherited tutorial mesh bug.
    capture(world,actor,"P0_QA_original_tutorial_visible_360cm",360,31,512,768,112,camera,component)
    generic.set_visibility(False,True)
    generic.set_hidden_in_game(True,True)
    capture(world,actor,"P0_raw_import_current_roster_360cm",360,31,512,768,112,camera,component)
    capture(world,actor,"P0_raw_import_closer_roster_260cm",260,31,512,768,112,camera,component)
    original=eyeline.get_material(0)
    eyeline.set_material(0,mat)
    capture(world,actor,"P0_eyeline_alpha_QA_roster_260cm",260,31,512,768,112,camera,component)
    # Close face makes the alpha-gradient comparison genuinely visible.
    eyeline.set_material(0,original)
    capture(world,actor,"P0_eyeline_original_face_130cm",130,45,512,768,170,camera,component)
    eyeline.set_material(0,mat)
    capture(world,actor,"P0_eyeline_alpha_face_130cm",130,45,512,768,170,camera,component)
    eyeline.set_material(0,original)
    capture(world,actor,"P0_raw_import_world_current_395cm_80deg",395,80,960,540,112,camera,component)
    capture(world,actor,"P0_raw_import_world_closer_300cm_70deg",300,70,960,540,112,camera,component)
    result["status"]="RENDERED_QA_ACTOR_COMPARISONS_NOT_PRODUCTION_UI"
except Exception as e:
    result["status"]="RENDER_FAILURE"
    result["errors"].append(str(e))
    result["traceback"]=traceback.format_exc()[-2500:]
(ROOT/"controlled_render_result.json").write_text(json.dumps(result,indent=2)+"\n")
unreal.log("P0_RENDER_SUITE "+str(result["status"])+" frames="+str(len(result["frame_specs"])))
