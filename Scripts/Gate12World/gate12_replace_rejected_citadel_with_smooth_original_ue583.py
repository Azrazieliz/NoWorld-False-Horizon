"""Replace only the nonapproved, partially omitted Royal Citadel actor's
mesh inside the candidate first World; retain both original meshes, original
Gate02 FBX, and preimport map backups unchanged.
"""
import unreal,json,traceback,hashlib
from pathlib import Path
ROOT=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
D=ROOT/"Saved/Gate12Readiness/FirstWorldR1_MergedSource"
LEVEL="/Game/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_SourceAssembly"
OLD="/Game/NoWorld/FirstWorld/R1/Anchors/SM_R1_RoyalCitadel_OriginalFBX"
NEW="/Game/NoWorld/FirstWorld/R1/Anchors/SM_R1_RoyalCitadel_SmoothedTriangulated"
LABEL="Gate02_R1_Actual_RoyalCitadel_SourceWorldSpace"
R={"status":"STARTED","map":LEVEL,"preserved_original":OLD,
    "corrected_source":NEW,"errors":[]}
OUT=D/"citadel_corrected_source_anchor_placement_receipt.json"
def save():OUT.write_text(json.dumps(R,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
save()
try:
 old_map=D/"L_FirstWorld_R1_BeforeRoyalCitadel_20261020.umap"
 assert old_map.is_file(),"non-destructive backup absent"
 assert hashlib.sha256(old_map.read_bytes()).hexdigest()==\
    "c9c513f9b899ba4c7e4fd7bf80d35de1544c845d34ac6e3a2dbb7caf2ff82cd0"
 q=json.loads((D/"royal_citadel_smoothed_native_import_receipt.json").read_text())
 assert q["errors"]==[] and q["actual_ue_lod0_triangles"]>1_200_000
 assert "OfflineGame_Gate12_Koikatsu_20261010" in unreal.Paths.get_project_file_path()
 correct=unreal.EditorAssetLibrary.load_asset(NEW)
 assert isinstance(correct,unreal.StaticMesh)
 assert unreal.EditorLevelLibrary.load_level(LEVEL),"Candidate source map missing"
 matches=[a for a in unreal.EditorLevelLibrary.get_all_level_actors()
          if a.get_actor_label()==LABEL]
 assert len(matches)==1,f"expected one original Citadel actor, found {len(matches)}"
 actor=matches[0]
 assert isinstance(actor,unreal.StaticMeshActor)
 comp=actor.get_editor_property("static_mesh_component")
 previous=comp.get_editor_property("static_mesh")
 assert previous is not None
 previous_path=previous.get_path_name()
 if previous_path==correct.get_path_name():
  R["status"]="PRESERVED_ALREADY_CORRECTED_SOURCE_ANCHOR"
 else:
  assert previous_path.startswith(OLD+"."),f"would override unfamiliar approved actor mesh: {previous_path}"
  location=actor.get_actor_location()
  assert abs(location.x)<1 and abs(location.y)<1 and abs(location.z)<1,\
     f"world-space anchored mesh shifted: {location}"
  comp.set_static_mesh(correct)
  assert unreal.EditorLevelLibrary.save_current_level(), "Cannot save exact source candidate World"
  R["status"]="CITADEL_CORRECTED_SOURCE_ANCHOR_SAVED_UNBUILT_NOT_ART_OR_ANDROID_APPROVED"
 R["actor_label"]=LABEL
 R["before_mesh"]=previous_path
 R["after_mesh"]=correct.get_path_name()
 R["source_world_space_location_cm"]=[0,0,0]
 R["preserved_preimport_world_map"]=str(old_map)
except Exception:
 R["status"]="FAILED_OR_PARTIAL"
 R["errors"].append(traceback.format_exc()[-6000:])
finally:
 save()
 unreal.log("GATE12_CORRECTED_CITADEL_WORLD="+R["status"])
