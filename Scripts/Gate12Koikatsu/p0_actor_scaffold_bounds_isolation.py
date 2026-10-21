"""Transient read-only P0 bound isolation for genuine experimental Luminous QA.
Only modifies a spawned disposable actor. Original Blueprint/assets never saved.
"""
import unreal,pathlib,json
OUT=pathlib.Path(unreal.Paths.project_saved_dir())/"P0CharacterQuality"/"scaffold_geometry_bound_isolation.json"
MAP="/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/Maps/L_LuminousV3_Playable_RuntimeQA"
CLASS="/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/BP_LuminousValentineV3_CLOTHED_RuntimeQA.BP_LuminousValentineV3_CLOTHED_RuntimeQA_C"
result={"schema":"P0_TUTORIAL_MESH_BOUND_ISOLATION_V1",
        "no_original_actor_or_asset_mutation":True,"measurements":{},"warnings":[]}
def b(a):
    o,e=a.get_actor_bounds(False)
    return {"origin_cm":[o.x,o.y,o.z],"size_cm":[2*e.x,2*e.y,2*e.z],
            "min_cm":[o.x-e.x,o.y-e.y,o.z-e.z],
            "max_cm":[o.x+e.x,o.y+e.y,o.z+e.z]}
try:
    unreal.EditorLevelLibrary.load_level(MAP)
    cls=unreal.load_class(None,CLASS)
    a=unreal.EditorLevelLibrary.spawn_actor_from_class(
        cls,unreal.Vector(0,0,120),unreal.Rotator(0,0,0),transient=True)
    if not a:raise RuntimeError("Cannot spawn existing QA Blueprint.")
    result["measurements"]["raw_qa_actor"]=b(a)
    components=a.get_components_by_class(unreal.SkeletalMeshComponent)
    result["skeletal_components"]=len(components)
    generic=next((c for c in components if c.get_name()=="CharacterMesh0"),None)
    if not generic:raise RuntimeError("Existing tutorial skeletal component absent.")
    inherited=generic.get_editor_property("skeletal_mesh_asset")
    result["inherited_tutorial_mesh"]=inherited.get_path_name() if inherited else None
    result["tutorial_initially_visible"]=bool(generic.is_visible())
    generic.set_visibility(False,True)
    generic.set_hidden_in_game(True,True)
    result["measurements"]["tutorial_mesh_hidden"]=b(a)
    try:
        generic.set_skeletal_mesh_asset(None)
        result["measurements"]["tutorial_mesh_unbound_transient_actor"]=b(a)
        result["qa_scaffold_removed_only_from_transient_actor"]=True
    except Exception as e:
        result["warnings"].append("Cannot transiently remove scaffold mesh: "+str(e))
    result["non_skeletal_primitives"]=[]
    for c in a.get_components_by_class(unreal.PrimitiveComponent):
        if isinstance(c,unreal.SkeletalMeshComponent):continue
        result["non_skeletal_primitives"].append({
            "name":c.get_name(),"class":c.get_class().get_name(),
            "visible":bool(c.is_visible()),
            "relative_location":str(c.get_editor_property("relative_location"))
            if hasattr(c,"relative_location") else None})
    unreal.EditorLevelLibrary.destroy_actor(a)
except Exception as e:result["error"]=str(e)
OUT.parent.mkdir(parents=True,exist_ok=True)
OUT.write_text(json.dumps(result,indent=2,default=str)+"\n")
unreal.log("P0_ASSEMBLED_SCAFFOLD_TEST "+json.dumps(result)[:1500])
