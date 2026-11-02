"""Place verified real Gate02 Eastern Market sections incrementally in the
existing candidate World, using original GLB anchor local_export_origin_m.
No asset overwrite, no automated canon, no Ruler teleport, no fixture World.
"""
import unreal,os,json,hashlib,traceback
from pathlib import Path
ROOT=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
G2=Path(r"D:\NoWorld_Art_Production\NoWorld_FirstWorld_RegionKit_R1_20261019")
STORE=ROOT/"Saved/Gate12Readiness/FirstWorldR1_MergedSource"
DIR=STORE/"eastern_market_original_object_shards"
MAP="/Game/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_SourceAssembly"
FIRST=int(os.environ.get("GATE12_MARKET_START","-1"))
LAST=int(os.environ.get("GATE12_MARKET_END","-1"))
OUT=DIR/f"market_native_placement_sections_{FIRST:02d}_{LAST:02d}.json"
R={"status":"STARTED","first":FIRST,"last":LAST,
   "map":MAP,"actors":[],"errors":[]}
def save(): OUT.write_text(json.dumps(R,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
save()
try:
 assert 0<=FIRST<=LAST<=11
 manifest=json.loads((DIR/"eastern_market_original_object_shards_receipt.json").read_text())
 assert manifest["status"]=="ORIGINAL_EASTERN_MARKET_SOURCE_PRESERVED_12_FBXS_PENDING_NATIVE_UE"
 auth=json.loads((G2/"contracts/authored_GLTF_asset_exports.json").read_text(encoding="utf8"))
 entry=next(e for e in auth["exports"] if e.get("anchor")=="eastern_market_town")
 assert entry["sha256"]==manifest["source_sha256"]
 assert entry["local_export_origin_m"]==[330,-115,0]
 assert "OfflineGame_Gate12_Koikatsu_20261010" in unreal.Paths.get_project_file_path()
 backup=STORE/"L_FirstWorld_R1_PreEasternMarketPlacement_20261020.umap"
 if not backup.is_file():
  raise RuntimeError("Source map backup is required before market placement; create it externally first")
 assert unreal.EditorLevelLibrary.load_level(MAP)
 all_actors=unreal.EditorLevelLibrary.get_all_level_actors()
 existing={a.get_actor_label():a for a in all_actors}
 origin=unreal.Vector(33000.,-11500.,0.)
 for i in range(FIRST,LAST+1):
  name=f"SM_R1_EasternMarket_SourceSection_{i:02d}"
  mesh_path=f"/Game/NoWorld/FirstWorld/R1/Anchors/Market/{name}"
  receipt=DIR/f"market_native_intake_section_{i:02d}.json"
  assert receipt.is_file(),"Unimported source section "+str(i)
  native=json.loads(receipt.read_text(encoding="utf8"))
  assert native["status"]=="REAL_SOURCE_MARKET_SECTION_NATIVE_IMPORTED_NOT_MOBILE_OR_ART_APPROVED"
  assert native["original_source_glb_sha256"]==manifest["source_sha256"]
  mesh=unreal.EditorAssetLibrary.load_asset(mesh_path)
  assert isinstance(mesh,unreal.StaticMesh),"Missing true Unreal source mesh "+mesh_path
  label=f"Gate02_R1_Actual_EasternMarket_SourceSection_{i:02d}"
  if label in existing:
   actor=existing[label]
   assert isinstance(actor,unreal.StaticMeshActor)
   old=actor.get_editor_property("static_mesh_component").get_editor_property("static_mesh")
   assert old and old.get_path_name()==mesh.get_path_name(),"Cannot replace an unfamiliar existing source actor"
   R["actors"].append({"label":label,"status":"PRESERVED_EXISTING_NATIVE_SOURCE"})
   continue
  actor=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor,origin)
  assert actor,"Could not instantiate original Gate02 Market section"
  actor.get_editor_property("static_mesh_component").set_static_mesh(mesh)
  actor.set_actor_label(label)
  R["actors"].append({"label":label,"status":"REAL_ORIGINAL_GEO_ANCHORED",
                       "mesh":mesh.get_path_name(),"source_triangles":native["ue_triangles"],
                       "original_local_origin_m":[330,-115,0]})
  existing[label]=actor
 if not unreal.EditorLevelLibrary.save_current_level():
  raise RuntimeError("Failed to save original first-world map with market sections")
 R["status"]="REAL_GATE02_MARKET_SECTION_BATCH_SAVED_NOT_ART_OR_ANDROID_APPROVED"
except Exception:
 R["status"]="FAILED_OR_PARTIAL"
 R["errors"].append(traceback.format_exc()[-7800:])
finally:
 save()
 unreal.log("GATE12_MARKET_SOURCE_ANCHORS="+R["status"])
