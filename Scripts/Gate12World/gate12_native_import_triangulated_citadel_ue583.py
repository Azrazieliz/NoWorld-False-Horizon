"""Import repaired TRIANGULATED derivative of protected Gate02 Royal Citadel.
Never overwrite source/original imported mesh, materials or first World map.
Placement happens only after post-import validation of zero skipped meshes."""
import unreal,json,hashlib,traceback
from pathlib import Path
RROOT=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
STORE=RROOT/"Saved/Gate12Readiness/FirstWorldR1_MergedSource"
REPAIR=json.loads((STORE/"royal_citadel_triangle_repair_receipt.json").read_text())
F=Path(REPAIR["output"])
PACKAGE="/Game/NoWorld/FirstWorld/R1/Anchors"
NAME="SM_R1_RoyalCitadel_TriangulatedSource"
P=PACKAGE+"/"+NAME
OUT=STORE/"royal_citadel_fixed_native_import_receipt.json"
S={"status":"STARTED","source":str(F),"mesh":P,"errors":[]}
def save():OUT.write_text(json.dumps(S,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
save()
try:
 assert REPAIR["status"]=="SOURCE_FBX_TRIANGULATED_DERIVATIVE_PRESERVES_VERTICES_PENDING_UE_IMPORT"
 assert F.is_file()
 assert hashlib.sha256(F.read_bytes()).hexdigest()==REPAIR["output_sha256"]
 assert "OfflineGame_Gate12_Koikatsu_20261010" in unreal.Paths.get_project_file_path()
 if not unreal.EditorAssetLibrary.does_asset_exist(P):
  opts=unreal.FbxImportUI()
  opts.set_editor_property("import_mesh",True)
  opts.set_editor_property("import_as_skeletal",False)
  opts.set_editor_property("import_materials",True)
  opts.set_editor_property("import_textures",True)
  opts.static_mesh_import_data.set_editor_property("combine_meshes",True)
  opts.static_mesh_import_data.set_editor_property("auto_generate_collision",False)
  task=unreal.AssetImportTask()
  task.set_editor_property("filename",str(F))
  task.set_editor_property("destination_path",PACKAGE)
  task.set_editor_property("destination_name",NAME)
  task.set_editor_property("automated",True)
  task.set_editor_property("save",True)
  task.set_editor_property("replace_existing",False)
  task.set_editor_property("async_",False)
  task.set_editor_property("options",opts)
  unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
  S["import_action"]="IMPORTED_GENUINE_TRIANGULATED_SOURCE"
 else:
  S["import_action"]="PRESERVED_EXISTING_NO_REIMPORT"
 mesh=unreal.EditorAssetLibrary.load_asset(P)
 if not isinstance(mesh,unreal.StaticMesh):raise RuntimeError("Native mesh unavailable")
 S["mesh_object_path"]=mesh.get_path_name()
 S["blender_original_mesh_parts"]=REPAIR["imported_mesh_count"]
 S["blender_original_vertices"]=REPAIR["source_mesh_vertices"]
 S["blender_triangle_count"]=REPAIR["output_mesh_triangles"]
 try:
  S["actual_ue_lod0_triangles"]=mesh.get_num_triangles(0)
 except Exception as exc:
  S["actual_ue_lod0_triangles"]="UNAVAILABLE_FROM_PYTHON_API"
 S["status"]="CITADEL_REPAIRED_SOURCE_IMPORTED_PENDING_STRICT_POSTIMPORT_LOG_TRIANGLE_QA"
except Exception:
 S["status"]="FAILED_OR_PARTIAL"
 S["errors"]=[traceback.format_exc()[-6000:]]
finally:
 save()
 unreal.log("GATE12_REPAIRED_CITADEL_NATIVE="+S["status"])
