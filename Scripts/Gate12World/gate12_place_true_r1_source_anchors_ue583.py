"""Non-destructive precise Gate02 R1 anchor placement in saved real World.
Original GLB local exporter center maps to authored Blender x/y metric.
Retains materials and source meshes, no fake gameplay or installed canon."""
import unreal,json,hashlib,traceback
from pathlib import Path
P=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
LEVEL="/Game/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_SourceAssembly"
OUT=P/"Saved/Gate12Readiness/FirstWorldR1_MergedSource/source_anchor_placement_receipt.json"
ROOT=Path(r"D:\NoWorld_Art_Production\NoWorld_FirstWorld_RegionKit_R1_20261019")
DOC=json.loads((ROOT/"contracts/authored_GLTF_asset_exports.json").read_text(encoding="utf8"))
mapping={x["anchor"]:x for x in DOC["exports"] if x.get("status")=="FILE_VERIFIED_NOT_NATIVE_UNREAL_VALIDATED"}
R={"status":"STARTED","anchors":[],"errors":[]}
def save():OUT.write_text(json.dumps(R,indent=2),encoding="utf8")
save()
try:
 assert unreal.EditorLevelLibrary.load_level(LEVEL),"Cannot open preserved original sourced region map"
 actors=unreal.EditorLevelLibrary.get_all_level_actors()
 existing={x.get_actor_label() for x in actors}
 for source,asset in [
      ("southern_farmland_windmill","SM_R1_SouthernFarmland"),
      ("watchwood_hamlet","SM_R1_WatchwoodHamlet")]:
  authored=mapping[source]
  loc=authored["local_export_origin_m"]
  path=f"/Game/NoWorld/FirstWorld/R1/Anchors/{asset}"
  obj=unreal.EditorAssetLibrary.load_asset(path)
  if not isinstance(obj,unreal.StaticMesh):
   raise RuntimeError("Genuine original StaticMesh source unavailable "+path)
  label=f"Gate02_R1_Actual_{source}_SourceAnchored"
  if label in existing:
   R["anchors"].append({"name":source,"status":"PRESERVED_EXISTING","label":label})
   continue
  assert len(loc)==3
  origin=unreal.Vector(float(loc[0])*100,float(loc[1])*100,float(loc[2])*100)
  actor=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor,origin)
  if not actor:raise RuntimeError("Cannot instantiate actual original "+source)
  actor.get_editor_property("static_mesh_component").set_static_mesh(obj)
  actor.set_actor_label(label)
  R["anchors"].append({"name":source,"mesh":obj.get_path_name(),"label":label,
     "authored_center_m":loc,"original_source_sha256":authored["sha256"],
     "placement":"Original GLB local origin -> UE centimeters, no secondary offset"})
  save()
 if not unreal.EditorLevelLibrary.save_current_level():
  raise RuntimeError("Could not persist actual anchored additions")
 R["status"]="REAL_SOURCE_ANCHORS_PLACED_IN_R1_WORLD_NOT_QUALITY_APPROVED"
except Exception as e:
 R.update(status="PARTIAL_OR_FAILED",errors=[str(e),traceback.format_exc()[-6500:]])
finally:
 save();unreal.log("GATE12_R1_REAL_SOURCE_PLACEMENT="+R["status"])
