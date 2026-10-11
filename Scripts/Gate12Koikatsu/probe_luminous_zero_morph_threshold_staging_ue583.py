"""Isolated no-overwrite comparison, never modifies original CLOTHED native assets."""
import unreal,pathlib,json,hashlib,time,traceback
W=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
Q=W/"Saved/Gate12Koikatsu/LuminousV3/CLOTHED"
SOURCE=W/"Saved/Gate12Koikatsu/LuminousV3/LosslessSources/LuminousV3CLOTHED/shards"
manifest=json.loads((SOURCE/"manifest_full_fidelity_primitives.json").read_text("utf-8-sig"))
audit=json.loads((Q/"source_shards_actual_nonzero_morph_inventory.json").read_text("utf-8-sig"))
destroot="/Game/Experimental/LuminousValentineV3/MorphThresholdProbe_20261011"
result={"status":"START","method":"ZERO_THRESHOLD_DIAGNOSTIC_NEW_STAGING_DESTINATIONS","originals_modified":False,"parts":[]}
out=Q/"staging_zero_morph_threshold_7_16_ue583.json"
def save():out.write_text(json.dumps(result,indent=2,default=str)+"\n",encoding="utf8")
save()
try:
 for index in (7,16):
  info=manifest["shards"][index]
  source=pathlib.Path(info["path"])
  if hashlib.sha256(source.read_bytes()).hexdigest()!=info["sha256"]:raise RuntimeError("source hash mismatch")
  dest=destroot+"/Index%03d"%index
  if unreal.EditorAssetLibrary.does_directory_exist(dest):raise RuntimeError("staging destination already exists: "+dest)
  pipeline=unreal.InterchangeGenericAssetsPipeline()
  mp=pipeline.get_editor_property("mesh_pipeline")
  before=mp.get_editor_property("morph_threshold_position")
  mp.set_editor_property("morph_threshold_position",0.0)
  after=mp.get_editor_property("morph_threshold_position")
  if after!=0.0: raise RuntimeError("zero threshold was not accepted")
  task=unreal.AssetImportTask()
  task.filename=str(source)
  task.destination_path=dest
  task.automated=True
  task.async_=False
  task.save=True
  task.replace_existing=False
  task.options=pipeline
  unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
  names=unreal.EditorAssetLibrary.list_assets(dest,recursive=True,include_folder=False)
  sk=[]
  for n in names:
   a=unreal.EditorAssetLibrary.load_asset(n)
   if isinstance(a,unreal.SkeletalMesh):
    sk.append({"path":str(n),"morph_count":len(a.get_editor_property("morph_targets")),
               "morph_names":[x.get_name() for x in a.get_editor_property("morph_targets")]})
  expected=int(audit["nonzero_by_shard"][index]["nonzero_position_morphs"])
  journal={"index":index,"threshold_original_cm":before,"threshold_test_cm":after,
    "expected_source_nonzero_morphs":expected,"skeletal_meshes":sk,"asset_count":len(names),
    "source_sha256":info["sha256"],"stage_destination":dest}
  journal["status"]="PASS_ZERO_THRESHOLD_SOURCE_MORPHS" if sk and sum(a["morph_count"] for a in sk)>=expected else "FAIL_STILL_MISSING_MORPHS"
  result["parts"].append(journal);save()
 result["status"]="PASS_ZERO_THRESHOLD_STAGING_BOTH" if all(p["status"].startswith("PASS") for p in result["parts"]) else "FAIL_ZERO_THRESHOLD_MORPH_PROBE"
except Exception as e:
 result["status"]="FAIL_STAGING_PRESERVED_DIAGNOSTIC"
 result["exception"]=repr(e);result["traceback"]=traceback.format_exc()[-8000:]
finally:
 result["updated_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime());save()
 unreal.log("G12_ZERO_THRESHOLD_PROBE="+result["status"])