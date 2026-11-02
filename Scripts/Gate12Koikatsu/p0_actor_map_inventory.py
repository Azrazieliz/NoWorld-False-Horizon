"""Read-only, no-cook map and skeletal-actor assembly inventory."""
import json,pathlib,unreal
OUT=pathlib.Path(unreal.Paths.project_saved_dir())/"P0CharacterQuality"/"assembled_actor_native_inventory.json"
LEVELS=[
"/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/Maps/L_LuminousV3_Playable_RuntimeQA",
"/Game/Experimental/LuminousValentineV3/AndroidReady_20261011/Maps/L_LuminousV3_IsolatedAndroid",
"/Game/Experimental/Gate12Koikatsu/FullFidelityQA/Maps/L_KK_CompleteSource_UE583",
"/Game/Maps/StartingWorld"]
out={"schema":"P0_NATIVE_ASSEMBLED_WORLD_V1","no_cook":True,"maps":[]}
def v(v):return [round(float(x),5) for x in (v.x,v.y,v.z)]
for path in LEVELS:
    entry={"map":path,"actors":[],"error":None}
    try:
        unreal.EditorLevelLibrary.load_level(path)
        for actor in unreal.EditorLevelLibrary.get_all_level_actors():
            comps=actor.get_components_by_class(unreal.SkeletalMeshComponent)
            if not comps:continue
            rec={"name":actor.get_name(),"class":actor.get_class().get_name(),
                 "location_cm":v(actor.get_actor_location()),
                 "scale":v(actor.get_actor_scale3d()),
                 "actor_bounds":str(actor.get_actor_bounds(False)),"components":[]}
            for c in comps:
                item={"name":c.get_name()}
                try:
                    mesh=c.get_editor_property("skeletal_mesh_asset")
                    item.update({
                      "skeletal_mesh":mesh.get_path_name() if mesh else None,
                      "relative_location_cm":v(c.relative_location),
                      "relative_scale":v(c.relative_scale3d),
                      "owner_actor_location_cm":v(actor.get_actor_location()),
                      "approx_world_scale":[round(float(x)*float(y),5) for x,y in zip(v(c.relative_scale3d),v(actor.get_actor_scale3d()))],
                      "visible":bool(c.is_visible()),
                      "material_slot_count":c.get_num_materials()})
                    b=c.get_component_bounds()
                    item["bounds"]={"origin_cm":v(b[0]),"half_extent_cm":v(b[1]),
                                    "size_cm":[round(2*x,5) for x in v(b[1])],
                                    "radius_cm":float(b[2])}
                except Exception as e:item["error"]=str(e)
                rec["components"].append(item)
            entry["actors"].append(rec)
        entry["actors_skeletal"]=len(entry["actors"])
        entry["total_skeletal_components"]=sum(len(a["components"]) for a in entry["actors"])
    except Exception as e:entry["error"]=str(e)
    out["maps"].append(entry)
    unreal.log("P0_MAP "+path+" SKEL_ACTORS="+str(len(entry["actors"]))+" ERR="+str(entry["error"]))
OUT.parent.mkdir(parents=True,exist_ok=True)
OUT.write_text(json.dumps(out,indent=2)+"\n",encoding="utf-8")
unreal.log("P0_ASSEMBLED_REPORT "+str(OUT))
