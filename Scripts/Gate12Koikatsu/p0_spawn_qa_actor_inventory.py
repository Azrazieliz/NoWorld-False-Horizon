"""P0 no-cook, no-save inspection of genuine existing experimental QA Blueprints.
Spawn transiently in an isolated QA map and destroy before returning.
"""
import json,pathlib,unreal
OUT=pathlib.Path(unreal.Paths.project_saved_dir())/"P0CharacterQuality"/"runtime_qa_actor_inventory.json"
MAP="/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/Maps/L_LuminousV3_Playable_RuntimeQA"
CLASSES={
 "clothed_qa":"/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/BP_LuminousValentineV3_CLOTHED_RuntimeQA.BP_LuminousValentineV3_CLOTHED_RuntimeQA_C",
 "world_qa":"/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/BP_LuminousV3_WorldMode_RuntimeQA.BP_LuminousV3_WorldMode_RuntimeQA_C",
 "android_qa":"/Game/Experimental/LuminousValentineV3/AndroidReady_20261011/BP_LuminousV3_IsolatedAndroid.BP_LuminousV3_IsolatedAndroid_C"}
def v(z):return [round(float(x),4) for x in (z.x,z.y,z.z)]
def bounds(a):
    o,e=a.get_actor_bounds(False)
    return {"origin_cm":v(o),"half_cm":v(e),"size_cm":[2*x for x in v(e)],
            "aabb_min_cm":[round(x-y,4) for x,y in zip(v(o),v(e))],
            "aabb_max_cm":[round(x+y,4) for x,y in zip(v(o),v(e))]}
unreal.EditorLevelLibrary.load_level(MAP)
result={"schema":"GATE12_P0_TRANSIENT_GENUINE_QA_ACTORS_V1",
        "map":MAP,"production_catalog_unchanged":True,"test_assets_only":True,
        "actors":{}}
for label,path in CLASSES.items():
    record={"path":path}
    actor=None
    try:
        klass=unreal.load_class(None,path)
        if not klass:
            raise RuntimeError("Existing genuine QA actor class is unavailable.")
        actor=unreal.EditorLevelLibrary.spawn_actor_from_class(
            klass,unreal.Vector(0,0,120),unreal.Rotator(0,0,0),transient=True)
        if not actor:raise RuntimeError("Actor spawn failed.")
        comps=actor.get_components_by_class(unreal.SkeletalMeshComponent)
        record["actor_class"]=actor.get_class().get_name()
        record["world_bounds"]=bounds(actor)
        record["actor_scale"]=v(actor.get_actor_scale3d())
        record["skeletal_component_count"]=len(comps)
        record["components"]=[]
        for c in comps:
            try:
                mesh=c.get_editor_property("skeletal_mesh_asset")
                mats=[c.get_material(i).get_path_name()
                      if c.get_material(i) else None
                      for i in range(c.get_num_materials())]
                record["components"].append({
                    "name":c.get_name(),
                    "skeletal_mesh":mesh.get_path_name() if mesh else None,
                    "relative_location_cm":v(c.relative_location),
                    "relative_scale":v(c.relative_scale3d),
                    "visible":bool(c.is_visible()),
                    "material_paths":mats,
                    "asset_lods":unreal.EditorSkeletalMeshLibrary.get_lod_count(mesh)
                                 if mesh else None,
                })
            except Exception as ex:record["components"].append({"error":str(ex)})
        record["missing_component_meshes"]=sum(
            int(not c.get("skeletal_mesh")) for c in record["components"])
        record["skeletal_component_visible"]=sum(
            int(bool(c.get("visible"))) for c in record["components"])
    except Exception as ex:record["error"]=str(ex)
    finally:
        if actor:unreal.EditorLevelLibrary.destroy_actor(actor)
    result["actors"][label]=record
    unreal.log("P0_QA_ACTOR "+label+" comps="+str(record.get("skeletal_component_count"))+
               " bounds="+str(record.get("world_bounds"))+" error="+str(record.get("error")))
OUT.parent.mkdir(parents=True,exist_ok=True)
OUT.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
unreal.log("P0_QA_WORLD_REPORT "+str(OUT))
