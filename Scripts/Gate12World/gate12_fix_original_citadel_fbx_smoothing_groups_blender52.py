"""Strict Gate02 citadel source derivative, original geometry and topology
preserved with explicit per-face FBX smoothing groups for UE Interchange.
Does not overwrite the original source or prior QA derivatives.
"""
import bpy,json,hashlib,traceback
from pathlib import Path
ROOT=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
D=ROOT/"Saved/Gate12Readiness/FirstWorldR1_MergedSource"
old=json.loads((D/"royal_citadel_triangle_repair_receipt.json").read_text())
SOURCE=Path(old["output"])
OUT=D/"royal_citadel_TRIANGULATED_FACE_SMOOTHING_DERIVATIVE.fbx"
REC=D/"royal_citadel_smoothing_repair_receipt.json"
R={"status":"STARTED","source":str(SOURCE),"output":str(OUT),"errors":[]}
def save(): REC.write_text(json.dumps(R,ensure_ascii=False,indent=2)+"\n",encoding="utf8")
save()
try:
 assert old["status"]=="SOURCE_FBX_TRIANGULATED_DERIVATIVE_PRESERVES_VERTICES_PENDING_UE_IMPORT"
 assert SOURCE.is_file()
 assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==old["output_sha256"]
 assert not OUT.exists(),"Protected derivative exists, refusing overwrite"
 bpy.ops.import_scene.fbx(filepath=str(SOURCE),use_custom_normals=True)
 objs=[o for o in bpy.data.objects if o.type=="MESH"]
 R["source_mesh_objects"]=len(objs)
 R["mesh_vertices"]=sum(len(o.data.vertices) for o in objs)
 R["mesh_triangles"]=sum(len(o.data.polygons) for o in objs)
 assert R["source_mesh_objects"]==old["imported_mesh_count"]
 assert R["mesh_vertices"]==old["source_mesh_vertices"]
 assert R["mesh_triangles"]==old["output_mesh_triangles"]
 assert all(len(p.vertices)==3 for o in objs for p in o.data.polygons)
 # Blender exporter FACE writes polygon smoothing flags rather than OFF
 # (which UE Interchange warned for each of 2411 original mesh parts).
 bpy.ops.export_scene.fbx(filepath=str(OUT),use_selection=False,
     apply_unit_scale=True,use_mesh_modifiers=True,use_triangles=True,
     mesh_smooth_type="FACE",add_leaf_bones=False,path_mode="AUTO")
 R["output_sha256"]=hashlib.sha256(OUT.read_bytes()).hexdigest()
 R["output_bytes"]=OUT.stat().st_size
 R["status"]="SAME_ORIGINAL_VERTICES_TRIANGLES_EXPLICIT_FACE_SMOOTHING_PENDING_UNREAL"
except Exception:
 R["status"]="FAILED_OR_PARTIAL"
 R["errors"].append(traceback.format_exc()[-7000:])
finally:
 save()
 print("GATE12_CITADEL_FACE_SMOOTHING_REPAIR="+R["status"],flush=True)
