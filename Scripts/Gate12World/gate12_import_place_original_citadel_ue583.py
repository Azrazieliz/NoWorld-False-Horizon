"""Gate12 world candidate: non-destructive direct original royal_citadel.fbx import.
Original Gate02 FBX is world-space in meters. Spawn at world origin, not
another +center translation. No art approval, no replacing GameDefaultMap."""
import unreal, json, hashlib, traceback
from pathlib import Path
ROOT=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
G2=Path(r"D:\NoWorld_Art_Production\NoWorld_FirstWorld_RegionKit_R1_20261019")
FBX=G2/"anchors/royal_citadel.fbx"
WORLD="/Game/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_SourceAssembly"
PACKAGE="/Game/NoWorld/FirstWorld/R1/Anchors"
NAME="SM_R1_RoyalCitadel_OriginalFBX"
LABEL="Gate02_R1_Actual_RoyalCitadel_SourceWorldSpace"
RECEIPT=ROOT/"Saved/Gate12Readiness/FirstWorldR1_MergedSource/royal_citadel_true_source_receipt.json"
R={"status":"STARTED","source":str(FBX),"original_source_sha256":None,
   "mesh":PACKAGE+"/"+NAME,"level":WORLD,"errors":[]}
def save():
 RECEIPT.write_text(json.dumps(R,ensure_ascii=False,indent=2),encoding="utf8")
save()
try:
 assert FBX.is_file(),str(FBX)
 R["original_source_sha256"]=hashlib.sha256(FBX.read_bytes()).hexdigest()
 contract=json.loads((G2/"contracts/anchor_registry_candidate.json").read_text(encoding="utf8"))
 entry=next(e for e in contract["authored_anchors"] if e["id"]=="NW_FW_R1_CITADEL")
 assert entry["asset_space"]=="WORLD_SPACE_BLENDER_METERS_SOURCE"
 assert entry["canonical_id"] is None
 assert entry["scene_placement_translation_m"]==[0,0,0]
 assert "OfflineGame_Gate12_Koikatsu_20261010" in unreal.Paths.get_project_file_path()
 object_path=PACKAGE+"/"+NAME
 if not unreal.EditorAssetLibrary.does_asset_exist(object_path):
  opts=unreal.FbxImportUI()
  opts.set_editor_property("import_mesh",True)
  opts.set_editor_property("import_as_skeletal",False)
  opts.set_editor_property("import_materials",True)
  opts.set_editor_property("import_textures",True)
  opts.static_mesh_import_data.set_editor_property("combine_meshes",True)
  opts.static_mesh_import_data.set_editor_property("auto_generate_collision",False)
  task=unreal.AssetImportTask()
  task.set_editor_property("filename",str(FBX))
  task.set_editor_property("destination_path",PACKAGE)
  task.set_editor_property("destination_name",NAME)
  task.set_editor_property("automated",True)
  task.set_editor_property("save",True)
  task.set_editor_property("replace_existing",False)
  task.set_editor_property("options",opts)
  unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
  R["import_status"]="SOURCE_FBX_IMPORTED_NEW_ASSET"
 else:
  R["import_status"]="SOURCE_MESH_ALREADY_PRESENT_NOT_OVERWRITTEN"
 mesh=unreal.EditorAssetLibrary.load_asset(object_path)
 if not isinstance(mesh,unreal.StaticMesh):
  raise RuntimeError("original Gate02 Citadel StaticMesh not available after import")
 if not unreal.EditorLevelLibrary.load_level(WORLD):
  raise RuntimeError("Gate02 original first World source map not loadable")
 all_actors=unreal.EditorLevelLibrary.get_all_level_actors()
 existing=next((a for a in all_actors if a.get_actor_label()==LABEL),None)
 if existing:
  R["actor_status"]="PROTECTED_EXISTING_ACTOR"
  actor=existing
 else:
  # Original FBX vertices are WORLD-space in meters and imported as UE cm;
  # another (x,y) anchor translation would displace original geometry.
  actor=unreal.EditorLevelLibrary.spawn_actor_from_class(
    unreal.StaticMeshActor,unreal.Vector(0.0,0.0,0.0))
  if not actor:raise RuntimeError("Cannot create original Gate02 Citadel actor")
  actor.get_editor_property("static_mesh_component").set_static_mesh(mesh)
  actor.set_actor_label(LABEL)
  R["actor_status"]="REAL_ORIGINAL_CITADEL_PLACED_WITH_AUTHORED_WORLD_COORDINATES"
 if not unreal.EditorLevelLibrary.save_current_level():
  raise RuntimeError("Citadel populated source map could not be saved")
 R["actor_label"]=actor.get_actor_label()
 R["mesh_path"]=mesh.get_path_name()
 R["candidate_anchor_id"]=entry["id"]
 R["canonical_id"]=None
 R["status"]="CITADEL_ORIGINAL_SOURCE_NATIVE_IMPORTED_AND_PLACED_NOT_MOBILE_OR_ART_APPROVED"
except Exception as exc:
 R["status"]="FAILED_OR_PARTIAL"
 R["errors"]=[str(exc),traceback.format_exc()[-6000:]]
finally:
 save()
 unreal.log("GATE12_FIRST_WORLD_CITADEL="+R["status"])
