"""Post-assembly Gate02 R1 first-world native-source integrity audit.
Checks all 12 actual market geometry parts, terrain generator, source roads,
three earlier real anchors and Ruler PlayerStart in saved candidate World.
No world mutation, art approval or default map selection.
"""
import unreal,json,hashlib,traceback
from pathlib import Path
ROOT=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
E=ROOT/"Saved/Gate12Readiness/FirstWorldR1_MergedSource"
M=E/"eastern_market_original_object_shards"
LEVEL="/Game/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_SourceAssembly"
OUT=E/"Gate12_FirstWorld_AllFourGate02Anchors_UE_Integrity.json"
R={"status":"STARTED","map":LEVEL,"errors":[],"market_sections":[]}
try:
 manifest=json.loads((M/"eastern_market_original_object_shards_receipt.json").read_text())
 assert manifest["status"]=="ORIGINAL_EASTERN_MARKET_SOURCE_PRESERVED_12_FBXS_PENDING_NATIVE_UE"
 assert manifest["original_mesh_count"]==508 and manifest["original_vertices"]==9455450
 assert manifest["original_polygons"]==10607212
 assert unreal.EditorLevelLibrary.load_level(LEVEL)
 actors=unreal.EditorLevelLibrary.get_all_level_actors()
 lookup={a.get_actor_label():a for a in actors}
 previous=[
  "Gate02_R1_Actual_RoyalCitadel_SourceWorldSpace",
  "Gate02_R1_Actual_southern_farmland_windmill_SourceAnchored",
  "Gate02_R1_Actual_watchwood_hamlet_SourceAnchored"]
 for label in previous:assert label in lookup,"Missing prior original Gate02 anchor "+label
 expected_tris=0
 for i in range(12):
  label=f"Gate02_R1_Actual_EasternMarket_SourceSection_{i:02d}"
  assert label in lookup,"Missing original Market shard actor: "+label
  actor=lookup[label]
  assert isinstance(actor,unreal.StaticMeshActor)
  pos=actor.get_actor_location()
  assert abs(pos.x-33000)<1 and abs(pos.y+11500)<1 and abs(pos.z)<1
  m=actor.get_editor_property("static_mesh_component").get_editor_property("static_mesh")
  assert isinstance(m,unreal.StaticMesh)
  native=json.loads((M/f"market_native_intake_section_{i:02d}.json").read_text())
  assert native["status"]=="REAL_SOURCE_MARKET_SECTION_NATIVE_IMPORTED_NOT_MOBILE_OR_ART_APPROVED"
  assert m.get_path_name()==native["native_asset_path"]
  tris=m.get_num_triangles(0)
  assert tris==native["ue_triangles"]
  expected_tris+=tris
  R["market_sections"].append({"actor":label,"mesh":m.get_path_name(),
                               "native_triangles":tris,"world_cm":[pos.x,pos.y,pos.z]})
 roads=[a for a in actors if "Gate02" in a.get_actor_label() and "road" in a.get_actor_label().lower()]
 starts=[a for a in actors if "PlayerStart" in a.get_class().get_name()]
 generators=[a for a in actors if "StartingRegionGenerator" in a.get_class().get_name()]
 assert len(roads)>=6
 assert len(starts)>=1 and len(generators)>=1
 assert expected_tris>10_000_000*.95
 assert (ROOT/"Config/DefaultEngine.ini").read_text(encoding="utf-8-sig").find("GameDefaultMap=/Game/Maps/StartingWorld")!=-1
 R["native_market_triangles"]=expected_tris
 R["road_actors"]=len(roads)
 R["terrain_generators"]=len(generators)
 R["player_starts"]=len(starts)
 R["saved_level_actors"]=len(actors)
 R["status"]="FOUR_GATE02_REAL_SOURCE_ANCHORS_PRESENT_NATIVE_SOURCE_MAP_UNBUILT_NOT_ART_APPROVED"
except Exception:
 R["status"]="FAILED_OR_PARTIAL"
 R["errors"].append(traceback.format_exc()[-7000:])
finally:
 OUT.write_text(json.dumps(R,indent=2)+"\n",encoding="utf8")
 unreal.log("GATE12_R1_FOUR_REAL_ANCHORS="+R["status"])
