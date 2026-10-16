"""NO-COOK probe of UE5.8.3 Python skeletal-mesh API; read-only assets."""
import json, pathlib, traceback
import unreal
P="/Game/Experimental/LuminousValentineV3/CLOTHED/Parts/Mesh003Prim011/Mesh_03_Prim_11"
N=P+"/SkeletalMeshes/Mesh_03_Prim_11"
T=P+"/Textures/cf_m_body_light"
out={"status":"API_PROBE","assets":[],"methods":{},"errors":[]}
def safe(f):
    try:
        return str(f())[:500]
    except Exception as e:
        return "NOT_AVAILABLE: "+repr(e)[:190]
try:
    o=unreal.load_asset(N)
    if not o:
        raise RuntimeError("Body asset not found at "+N)
    out["methods"]["skeletal_mesh"]= [
        v for v in dir(o) if any(k in v.lower() for k in (
            "lod","bounds","morph","skin","material","vertices","vertex","skeleton"))]
    props={}
    for f in ("lod_info","imported_bounds","materials","morph_targets","skeleton","min_lod"):
        props[f]=safe(lambda f=f: o.get_editor_property(f))
    for f in ("get_num_lods","get_lod_num_vertices","get_bounds","get_morph_targets",
              "get_lod_num_vertices","get_lod_num_triangles"):
        obj=getattr(o,f,None)
        props[f]=safe(lambda obj=obj: obj(0) if "vertices" in f or "triangles" in f else obj())
    out["assets"].append({"path":N,"cls":o.get_class().get_name(),"probe":props})
except Exception as e:
    out["errors"].append(str(e))
try:
    t=unreal.load_asset(T)
    if t:
        out["methods"]["texture"]= [
            v for v in dir(t) if any(k in v.lower() for k in (
                "size","mip","compression","srgb","stream","filter"))]
        out["assets"].append({"path":T,"cls":t.get_class().get_name(),
            "probe":{f:safe(lambda f=f: t.get_editor_property(f)) for f in
                ("compression_settings","lod_group","lod_bias","srgb","never_stream",
                 "mip_gen_settings","virtual_texture_streaming","max_texture_size")}})
except Exception as e:
    out["errors"].append(str(e))
out["methods"]["libraries"]= {
    v: [x for x in dir(getattr(unreal,v)) if any(k in x.lower() for k in
         ("lod","vertex","morph","texture","poly","triang"))][:60]
    for v in ("EditorSkeletalMeshLibrary","SkeletalMeshEditingLibrary",
              "EditorAssetLibrary","MaterialEditingLibrary") if hasattr(unreal,v)}
p=pathlib.Path(unreal.Paths.project_saved_dir())/"P0CharacterQuality"/"native_api_probe.json"
p.parent.mkdir(parents=True,exist_ok=True)
p.write_text(json.dumps(out,indent=2)+"\n")
unreal.log("P0_QUALITY_API_PROBE "+json.dumps({"output":str(p),"errors":out["errors"]}))
