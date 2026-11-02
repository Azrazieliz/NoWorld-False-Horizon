"""One integration checkpoint after replacing the rejected Citadel mesh.
Read-only: inspect the saved Gate02 candidate first World, preserve all actors
and assets, never create canonical Registry IDs or game fallback entities."""
import unreal,json,traceback,hashlib
from pathlib import Path
ROOT=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
LEVEL="/Game/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_SourceAssembly"
SAVE=ROOT/"Saved/Gate12Readiness/FirstWorldR1_MergedSource"
OUT=SAVE/"Gate12_FirstWorld_R1_PostCitadel_UE_ActorInspection.json"
R={"schema":"gate12.world.gate02_r1.post_citadel_inspection.v1",
   "status":"STARTED","level":LEVEL,"actors":[],"errors":[]}
try:
 assert "OfflineGame_Gate12_Koikatsu_20261010" in unreal.Paths.get_project_file_path()
 assert unreal.EditorLevelLibrary.load_level(LEVEL)
 actors=unreal.EditorLevelLibrary.get_all_level_actors()
 assert actors,"Saved source map has no actors"
 roads=[];anchors=[];gens=[];spawns=[]
 for a in actors:
  name=a.get_actor_label()
  row={"label":name,"class":a.get_class().get_name()}
  if "Gate02" in name or "R1" in name or "PlayerStart" in row["class"]:
   p=a.get_actor_location()
   row["world_cm"]=[round(p.x,2),round(p.y,2),round(p.z,2)]
   if isinstance(a,unreal.StaticMeshActor):
    m=a.get_editor_property("static_mesh_component").get_editor_property("static_mesh")
    row["mesh"]=m.get_path_name() if m else None
   R["actors"].append(row)
  if "RoyalCitadel" in name or "southern_farmland" in name or "watchwood" in name:
   anchors.append(row)
  if "Gate02" in name and "road" in name.lower():roads.append(row)
  if "StartingRegionGenerator" in row["class"]:
   gens.append(row)
  if "PlayerStart" in row["class"]:spawns.append(row)
 R["total_loaded_level_actors"]=len(actors)
 R["anchor_labels"]=[x["label"] for x in anchors]
 R["road_labels"]=[x["label"] for x in roads]
 R["source_generator_labels"]=[x["label"] for x in gens]
 R["player_start_labels"]=[x["label"] for x in spawns]
 expected=[
  "Gate02_R1_Actual_RoyalCitadel_SourceWorldSpace",
  "Gate02_R1_Actual_southern_farmland_windmill_SourceAnchored",
  "Gate02_R1_Actual_watchwood_hamlet_SourceAnchored"]
 for x in expected:
  assert x in R["anchor_labels"],"Missing real source anchor "+x
 royal=next(x for x in anchors if "RoyalCitadel" in x["label"])
 assert royal["mesh"]=="/Game/NoWorld/FirstWorld/R1/Anchors/SM_R1_RoyalCitadel_SmoothedTriangulated.SM_R1_RoyalCitadel_SmoothedTriangulated"
 assert len(spawns)>0,"No playable authored PlayerStart"
 assert len(gens)>0,"No Gate02 513-grid native generator"
 assert len(roads)>=3,"Expected actual original source regional roads"
 assert not any("FakeNPC" in x["label"] or "TestingWorld" in x["label"] for x in R["actors"])
 R["status"]="SOURCE_WORLD_CITADEL_ROADS_TERRAIN_PLAYER_START_INSPECTED_NOT_PLAYABILITY_APPROVED"
except Exception:
 R["status"]="FAILED_OR_PARTIAL"
 R["errors"].append(traceback.format_exc()[-9000:])
finally:
 OUT.write_text(json.dumps(R,ensure_ascii=False,indent=2)+"\n",encoding="utf8")
 unreal.log("GATE12_R1_POST_CITADEL_WORLD_INSPECTION="+R["status"])
