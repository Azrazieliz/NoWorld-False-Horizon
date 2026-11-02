"""Gate12 real source World: create a different streaming-runtime map.
Original complete 12-section Gate02 authoring map and all source assets are
immutable. Duplicate source map, verify exact genuine market actors and the
other three anchors, remove ONLY original market actor references from duplicate;
native market streaming instantiates the SAME meshes at authored coordinates.
"""
import unreal,json,hashlib,traceback
from pathlib import Path
ROOT=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
CONTENT=ROOT/"Content/NoWorld/FirstWorld/Maps"
E=ROOT/"Saved/Gate12Readiness/FirstWorldR1_MergedSource"
SOURCE="/Game/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_SourceAssembly"
DEST="/Game/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_StreamingRuntime"
FILENAME=CONTENT/"L_FirstWorld_Gate02_R1_SourceAssembly.umap"
RECEIPT=E/"FirstWorld_R1_RuntimeStreamingMap_UE_20261021.json"
ORIGINAL_SHA256="9d856018c15bcdfcc1ce7bd320d88b3f299808a164c395b90f56d1444d594f83"
R={"status":"STARTED","source_map":SOURCE,"streaming_map":DEST,"original_sha256":ORIGINAL_SHA256,"errors":[]}
def save():RECEIPT.write_text(json.dumps(R,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
save()
try:
 assert FILENAME.is_file()
 assert hashlib.sha256(FILENAME.read_bytes()).hexdigest()==ORIGINAL_SHA256
 backup=E/"L_FirstWorld_All12Market_FrozenSourceBeforeRuntimeStreaming_20261021.umap"
 assert backup.is_file() and hashlib.sha256(backup.read_bytes()).hexdigest()==ORIGINAL_SHA256
 assert "OfflineGame_Gate12_Koikatsu_20261010" in unreal.Paths.get_project_file_path()
 assert unreal.EditorAssetLibrary.does_asset_exist(SOURCE)
 if not unreal.EditorAssetLibrary.does_asset_exist(DEST):
  assert unreal.EditorAssetLibrary.duplicate_asset(SOURCE,DEST), "Duplicate source map failed"
  R["copy"]="AUTHENTIC_12_SECTION_SOURCE_MAP_DUPLICATED"
 else:
  R["copy"]="PREVIOUS_STREAMING_RUNTIME_MAP_PROTECTED"
 assert unreal.EditorLevelLibrary.load_level(DEST),"Duplicated runtime map not loadable"
 actors=unreal.EditorLevelLibrary.get_all_level_actors()
 originals=[]
 for i in range(12):
  label=f"Gate02_R1_Actual_EasternMarket_SourceSection_{i:02d}"
  matches=[a for a in actors if a.get_actor_label()==label]
  if len(matches)==1:
   actor=matches[0]
   assert isinstance(actor,unreal.StaticMeshActor),label+" incorrect class"
   part=f"SM_R1_EasternMarket_SourceSection_{i:02d}"
   mesh=actor.get_editor_property("static_mesh_component").get_editor_property("static_mesh")
   assert mesh is not None and mesh.get_path_name()==(
    f"/Game/NoWorld/FirstWorld/R1/Anchors/Market/{part}.{part}"),label+" incorrect source mesh"
   pos=actor.get_actor_location()
   assert abs(pos.x-33000)<1 and abs(pos.y+11500)<1 and abs(pos.z)<1,label+" moved"
   originals.append(actor)
  elif len(matches)==0 and R["copy"]=="PREVIOUS_STREAMING_RUNTIME_MAP_PROTECTED":
   continue
  else:raise RuntimeError(f"Unexpected original source section {label}: {len(matches)}")
 assert len(originals)==12 or (len(originals)==0 and R["copy"]=="PREVIOUS_STREAMING_RUNTIME_MAP_PROTECTED")
 retained=["Gate02_R1_Actual_RoyalCitadel_SourceWorldSpace",
          "Gate02_R1_Actual_watchwood_hamlet_SourceAnchored",
          "Gate02_R1_Actual_southern_farmland_windmill_SourceAnchored"]
 for label in retained:
  assert len([a for a in actors if a.get_actor_label()==label])==1,"Missing retained source anchor "+label
 assert len([a for a in actors if a.get_class().get_name()=="PlayerStart"])>=1
 assert len([a for a in actors if "StartingRegionGenerator" in a.get_class().get_name()])>=1
 roads=[a for a in actors if "Gate02" in a.get_actor_label() and "road" in a.get_actor_label().lower()]
 assert len(roads)>=6
 for actor in originals:
  assert unreal.EditorLevelLibrary.destroy_actor(actor),"Could not remove duplicate-market actor "+actor.get_actor_label()
 assert unreal.EditorLevelLibrary.save_current_level(),"Cannot save source-preserving runtime World map"
 remaining=unreal.EditorLevelLibrary.get_all_level_actors()
 assert not any("Gate02_R1_Actual_EasternMarket_SourceSection_" in a.get_actor_label() for a in remaining)
 assert all(any(a.get_actor_label()==label for a in remaining) for label in retained)
 assert FILENAME.is_file() and hashlib.sha256(FILENAME.read_bytes()).hexdigest()==ORIGINAL_SHA256
 R["status"]="STREAMING_FIRST_WORLD_SAVED_ALL_12_REAL_MARKET_SOURCE_ASSETS_PRESERVED"
 R["market_original_actors_removed_from_DUPLICATE_ONLY"]=len(originals)
 R["retained_original_anchors"]=retained
 R["retained_original_roads"]=len(roads)
 R["runtime_world_actor_count"]=len(remaining)
 R["native_market_runtime"]="AOGStartingRegionGenerator::UpdateR1MarketStreaming"
 R["unverified"]=["Windows native build","runtime streaming","Android gameplay","visual art"]
except Exception:
 R["status"]="FAILED_OR_PARTIAL_PROTECTED_ORIGINAL"
 R["errors"].append(traceback.format_exc()[-7500:])
finally:
 save()
 unreal.log("GATE12_R1_STREAMING_WORLD="+R["status"])
