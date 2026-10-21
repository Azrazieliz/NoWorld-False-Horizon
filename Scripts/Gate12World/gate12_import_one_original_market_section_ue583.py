"""Native intake of ONE exact Gate02 Eastern Market source object partition.
Serial processing deliberately prevents exhausting UE5.8's RAM budget.
Selected section comes from inherited GATE12_MARKET_SECTION environment.
This does not invent architecture, auto-approve art, or overwrite old assets.
"""
import unreal,os,json,hashlib,traceback
from pathlib import Path
ROOT=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
SHARD=ROOT/"Saved/Gate12Readiness/FirstWorldR1_MergedSource/eastern_market_original_object_shards"
GROUP=int(os.environ.get("GATE12_MARKET_SECTION","-1"))
PACKAGE="/Game/NoWorld/FirstWorld/R1/Anchors/Market"
NAME=f"SM_R1_EasternMarket_SourceSection_{GROUP:02d}"
SOURCE=SHARD/f"eastern_market_source_section_{GROUP:02d}.fbx"
REC=SHARD/f"market_native_intake_section_{GROUP:02d}.json"
R={"status":"STARTED","section":GROUP,"mesh":PACKAGE+"/"+NAME,
   "source":str(SOURCE),"errors":[]}
def save():REC.write_text(json.dumps(R,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
try:
 assert GROUP in range(12),"Market section index absent or invalid"
 manifest=json.loads((SHARD/"eastern_market_original_object_shards_receipt.json").read_text())
 assert manifest["status"]=="ORIGINAL_EASTERN_MARKET_SOURCE_PRESERVED_12_FBXS_PENDING_NATIVE_UE"
 S=next(x for x in manifest["shards"] if x["section"]==GROUP)
 assert SOURCE.is_file() and hashlib.sha256(SOURCE.read_bytes()).hexdigest()==S["fbx_sha256"]
 assert "OfflineGame_Gate12_Koikatsu_20261010" in unreal.Paths.get_project_file_path()
 R["original_source_glb_sha256"]=manifest["source_sha256"]
 R["source_mesh_count"]=S["mesh_count"]
 R["source_verts"]=S["source_vertices"]
 R["source_polygons"]=S["source_polygons"]
 R["source_fbx_sha256"]=S["fbx_sha256"]
 save()
 assetpath=PACKAGE+"/"+NAME
 if not unreal.EditorAssetLibrary.does_asset_exist(assetpath):
  ui=unreal.FbxImportUI()
  ui.set_editor_property("import_mesh",True)
  ui.set_editor_property("import_as_skeletal",False)
  ui.set_editor_property("import_materials",True)
  ui.set_editor_property("import_textures",True)
  ui.static_mesh_import_data.set_editor_property("combine_meshes",True)
  ui.static_mesh_import_data.set_editor_property("auto_generate_collision",True)
  task=unreal.AssetImportTask()
  task.set_editor_property("filename",str(SOURCE))
  task.set_editor_property("destination_path",PACKAGE)
  task.set_editor_property("destination_name",NAME)
  task.set_editor_property("automated",True)
  task.set_editor_property("save",True)
  task.set_editor_property("replace_existing",False)
  task.set_editor_property("async_",False)
  task.set_editor_property("options",ui)
  unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
  R["operation"]="IMPORTED_LOSSLESS_ORIGINAL_OBJECTS_NATIVE_STATIC_MESH"
 else:
  R["operation"]="PRESERVED_PROTECTED_EXISTING_SOURCE_ASSET"
 m=unreal.EditorAssetLibrary.load_asset(assetpath)
 if not isinstance(m,unreal.StaticMesh):
  raise RuntimeError("Authentic Eastern Market SourceSection NativeStaticMesh missing")
 R["native_asset_path"]=m.get_path_name()
 R["ue_triangles"]=m.get_num_triangles(0)
 if R["ue_triangles"]<int(S["source_polygons"]*.93):
  raise RuntimeError("Original source triangles partly missing: received only "+str(R["ue_triangles"]))
 R["status"]="REAL_SOURCE_MARKET_SECTION_NATIVE_IMPORTED_NOT_MOBILE_OR_ART_APPROVED"
except Exception:
 R["status"]="FAILED_OR_PARTIAL"
 R["errors"].append(traceback.format_exc()[-6500:])
finally:
 save()
 unreal.log("GATE12_MARKET_NATIVE_SECTION="+R["status"]+" section="+str(GROUP))
