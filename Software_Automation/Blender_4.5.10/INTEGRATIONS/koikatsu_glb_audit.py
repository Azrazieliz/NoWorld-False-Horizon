"""Read-only Koikatsu-derived GLB -> Blender mesh/material/rig roundtrip audit.
Run: blender -b --factory-startup -t 2 --python koikatsu_glb_audit.py -- SOURCE.glb REPORT.json
Never saves or modifies the user's source or exported asset.
"""
import bpy,os,sys,json,time,traceback
from collections import Counter
args=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
if len(args)!=2:raise SystemExit("Usage: -- SOURCE.glb REPORT.json")
src,report_path=map(os.path.abspath,args)
if not os.path.isfile(src):raise FileNotFoundError(src)
start=time.perf_counter()
r={"source":src,"source_bytes":os.path.getsize(src),
 "blender":bpy.app.version_string,"status":"STARTED","provenance":"Read-only preexisting Koikatsu-derived shard. No rights to redistribute inferred."}
try:
 bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
 bpy.ops.import_scene.gltf(filepath=src)
 meshes=[ob for ob in bpy.data.objects if ob.type=="MESH"]
 rigs=[ob for ob in bpy.data.objects if ob.type=="ARMATURE"]
 mats=list(bpy.data.materials)
 imgs=list(bpy.data.images)
 r["object_count"]=len(bpy.data.objects)
 r["mesh_objects"]=len(meshes)
 r["armatures"]=len(rigs)
 r["armature_bones"]={o.name:len(o.data.bones) for o in rigs}
 r["mesh_vertices_total"]=sum(len(o.data.vertices) for o in meshes)
 r["mesh_polygons_total"]=sum(len(o.data.polygons) for o in meshes)
 r["mesh_metrics"]=[{"name":o.name,"vertices":len(o.data.vertices),
  "faces":len(o.data.polygons),"uv":list(x.name for x in o.data.uv_layers),
  "shape_keys":list(k.name for k in o.data.shape_keys.key_blocks) if o.data.shape_keys else [],
  "material_slots":[x.name for x in o.data.materials]} for o in meshes]
 r["material_count"]=len(mats)
 r["image_count"]=len(imgs)
 r["image_details"]=[{"name":i.name,"size":list(i.size),"colorspace":i.colorspace_settings.name,
    "packed":bool(i.packed_file)} for i in imgs]
 r["missing_uv_on_meshes"]=[o.name for o in meshes if not o.data.uv_layers and o.data.polygons]
 r["blend_shapes_total"]=sum(len(o.data.shape_keys.key_blocks)-1 for o in meshes if o.data.shape_keys)
 r["materials_in_node_graphs"]=sum(1 for m in mats if m.use_nodes and m.node_tree)
 r["empty_images"]=[i.name for i in imgs if i.size[0]==0 or i.size[1]==0]
 r["status"]="EXECUTED"
 r["quality_accepted"]=False
 r["original_KK_materials_preserved"]="UNVERIFIED"
 r["identity_fidelity"]="UNVERIFIED"
 r["character_reconstructed"]=False
except Exception:
 r["status"]="FAILED";r["error"]=traceback.format_exc()
finally:
 r["seconds"]=round(time.perf_counter()-start,3)
 os.makedirs(os.path.dirname(report_path),exist_ok=True)
 with open(report_path,"w",encoding="utf-8") as f:json.dump(r,f,indent=2)
 print("NW_KK_AUDIT="+json.dumps({k:v for k,v in r.items() if k not in ("mesh_metrics","image_details")}))
if r["status"]=="FAILED":raise SystemExit(2)
