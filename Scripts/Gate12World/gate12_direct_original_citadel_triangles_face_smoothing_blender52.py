"""Exact source repair: triangulate existing Gate02 royal citadel FBX faces
without decimation, vertex relocation, material remapping or replacement art.
The original FBX, original .umap and previous import are NEVER overwritten.
"""
import bpy,bmesh,hashlib,json,traceback,time
from pathlib import Path
ROOT=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
SOURCE=Path(r"D:\NoWorld_Art_Production\NoWorld_FirstWorld_RegionKit_R1_20261019\anchors\royal_citadel.fbx")
OUT=ROOT/"Saved/Gate12Readiness/FirstWorldR1_MergedSource/royal_citadel_TRIANGULATED_FACE_SMOOTHING_DERIVATIVE.fbx"
RECEIPT=ROOT/"Saved/Gate12Readiness/FirstWorldR1_MergedSource/royal_citadel_direct_source_smoothing_receipt.json"
FIX=[ "FLYING_RIB", "PEDIMENT" ]
R={"status":"STARTED","source":str(SOURCE),"output":str(OUT),"errors":[]}
def save():RECEIPT.write_text(json.dumps(R,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
save()
try:
 assert SOURCE.exists() and SOURCE.stat().st_size>1000000
 R["source_sha256"]=hashlib.sha256(SOURCE.read_bytes()).hexdigest()
 if OUT.exists():
  R["status"]="PROTECTED_EXISTING_DERIVATIVE_NOT_OVERWRITTEN"
  raise RuntimeError("Derivative already exists; refusing overwrite")
 bpy.ops.object.select_all(action='SELECT')
 bpy.ops.object.delete(use_global=False)
 bpy.ops.import_scene.fbx(filepath=str(SOURCE),use_custom_normals=True)
 meshes=[o for o in bpy.data.objects if o.type=="MESH"]
 R["imported_mesh_count"]=len(meshes)
 R["source_mesh_vertices"]=sum(len(o.data.vertices) for o in meshes)
 R["source_mesh_polygons"]=sum(len(o.data.polygons) for o in meshes)
 R["source_problem_meshes"]=[
  {"name":o.name,"vertices":len(o.data.vertices),"polygons":len(o.data.polygons)}
  for o in meshes if any(k in o.name for k in FIX)]
 tri_objects=0;tri_ngons=0
 for idx,o in enumerate(meshes):
  if not any(len(p.vertices)>3 for p in o.data.polygons):continue
  bm=bmesh.new(); bm.from_mesh(o.data)
  faces=[f for f in bm.faces if len(f.verts)>3]
  if faces:
   tri_ngons+=len(faces)
   bmesh.ops.triangulate(bm,faces=faces,quad_method='BEAUTY',ngon_method='BEAUTY')
   bm.to_mesh(o.data);o.data.update();tri_objects+=1
  bm.free()
  if idx%300==0:print("CITADEL_TRIANGULATED_OBJECTS",idx,len(meshes),flush=True)
 R["triangulated_objects"]=tri_objects
 R["triangulated_ngons"]=tri_ngons
 assert len(meshes)>0
 assert all(len(poly.vertices)==3 for obj in meshes for poly in obj.data.polygons)
 R["output_mesh_vertices"]=sum(len(o.data.vertices) for o in meshes)
 R["output_mesh_triangles"]=sum(len(o.data.polygons) for o in meshes)
 assert R["output_mesh_vertices"]==R["source_mesh_vertices"]
 bpy.ops.export_scene.fbx(filepath=str(OUT),use_selection=False,
    apply_unit_scale=True,use_mesh_modifiers=True,use_triangles=True,
    mesh_smooth_type='FACE',add_leaf_bones=False,path_mode='AUTO')
 R["output_sha256"]=hashlib.sha256(OUT.read_bytes()).hexdigest()
 R["output_bytes"]=OUT.stat().st_size
 R["status"]="SOURCE_FBX_DIRECT_TRIANGULATED_FACE_SMOOTHING_PRESERVED_GEOMETRY"
except Exception:
 R["errors"].append(traceback.format_exc()[-7000:])
 if R["status"]=="STARTED":R["status"]="FAILED_OR_PARTIAL_DERIVATIVE"
finally:
 save()
 print("GATE12_R1_CITADEL_TRIANGLE_REPAIR="+R["status"],flush=True)
