"""NO-COOK, NO-ASSET-EDIT UE5.8.3 imported native character audit.
Loads selected representative skeletal meshes and one body texture per state.
NullRHI-compatible: cannot claim rendered parity or actual streaming residency.
"""
import json
import pathlib
import unreal

ROOT="/Game/Experimental/LuminousValentineV3"
OUT=pathlib.Path(unreal.Paths.project_saved_dir())/"P0CharacterQuality"/"unreal_native_quality.json"


def attempt(function):
    try:
        return function()
    except Exception as exc:
        return {"error":str(exc)[:240]}


def item_path(obj):
    return obj.get_path_name() if obj else None


def fmt_bounds(bounds):
    if isinstance(bounds,dict):
        return bounds
    try:
        return {
            "origin_cm":[float(bounds.origin.x),float(bounds.origin.y),
                         float(bounds.origin.z)],
            "half_extent_cm":[float(bounds.box_extent.x),float(bounds.box_extent.y),
                              float(bounds.box_extent.z)],
            "size_cm":[2*float(bounds.box_extent.x),2*float(bounds.box_extent.y),
                       2*float(bounds.box_extent.z)]
        }
    except Exception as exc:
        return {"error":str(exc)}


def class_of(obj):
    return obj.get_class().get_name() if obj else "MISSING"


def audit_mesh(asset):
    r={"asset_path":asset,"class":None}
    m=unreal.load_asset(asset)
    r["class"]=class_of(m)
    if not m or not isinstance(m,unreal.SkeletalMesh):
        r["missing_or_wrong_class"]=True
        return r
    r["render_bounds"]=fmt_bounds(attempt(m.get_bounds))
    r["imported_bounds"]=fmt_bounds(attempt(m.get_imported_bounds))
    r["lod_count"]=attempt(lambda:unreal.EditorSkeletalMeshLibrary.get_lod_count(m))
    r["morph_target_count"]=attempt(lambda:len(m.get_all_morph_target_names()))
    r["morph_target_sample"]=attempt(lambda: [str(x) for x in
        m.get_all_morph_target_names()[:7]])
    skeleton=attempt(lambda:m.get_editor_property("skeleton"))
    r["skeleton"]=item_path(skeleton) if not isinstance(skeleton,dict) else skeleton
    mats=attempt(lambda:m.get_editor_property("materials"))
    if not isinstance(mats,dict):
        r["material_slots"]=[]
        for slot in mats:
            try:
                ref=slot.get_editor_property("material_interface")
                r["material_slots"].append({
                    "slot":str(slot.get_editor_property("material_slot_name")),
                    "material_path":item_path(ref),
                    "material_class":class_of(ref),
                })
            except Exception as exc:
                r["material_slots"].append({"error":str(exc)})
    else:
        r["material_slots"]=mats
    r["lod0_build_settings"]=str(attempt(
        lambda:unreal.EditorSkeletalMeshLibrary.get_lod_build_settings(m,0)))[:900]
    return r


def audit_texture(asset):
    t=unreal.load_asset(asset)
    result={"asset_path":asset,"class":class_of(t)}
    if not t or not isinstance(t,unreal.Texture2D):
        return result
    result["size_x"]=attempt(t.blueprint_get_size_x)
    result["size_y"]=attempt(t.blueprint_get_size_y)
    result["built_size"]=str(attempt(t.blueprint_get_built_texture_size))
    for p in ("compression_settings","lod_group","lod_bias","srgb",
              "never_stream","mip_gen_settings","virtual_texture_streaming",
              "max_texture_size","compression_quality","lossy_compression_amount"):
        result[p]=str(attempt(lambda p=p:t.get_editor_property(p)))
    return result


def main():
    report={
        "schema":"GATE12_P0_NATIVE_UNREAL_SKELETAL_FIDELITY_V1",
        "engine":str(unreal.SystemLibrary.get_engine_version()),
        "source_code_modified":False,"asset_mutations":False,
        "status":"SELECTED_NATIVE_ASSETS_QUANTIFIED_NOT_VISUAL_PARITY",
        "states":{},"limitations":[
            "Only representative meshes; UE import may split source mesh into many parts.",
            "LOD0 vertex and triangle count not exposed by this Python API;",
            "final frame render, texture residency and material shader parity need separate evidence.",
        ],
    }
    for state in ("CLOTHED","UNDERWEAR","NUDE"):
        folder=ROOT+"/"+state+"/Parts"
        r={}
        for mesh, part in [("body","Mesh003Prim011"),
                           ("face_qa","Mesh003Prim000"),
                           ("other_qa","Mesh005Prim000")]:
            stem="Mesh_%02d_Prim_%02d" % (int(part[4:7]), int(part[-3:]))
            r[mesh]=audit_mesh(
                folder+"/"+part+"/"+stem+"/SkeletalMeshes/"+stem)
        r["body_texture"]=audit_texture(
            folder+"/Mesh003Prim011/Mesh_03_Prim_11/Textures/cf_m_body_light")
        report["states"][state]=r
        unreal.log("P0_UNREAL_STATE "+state+" body_bounds="+str(r["body"].get("render_bounds")))
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    unreal.log("P0_UNREAL_NATIVE_AUDIT "+str(OUT))


main()
