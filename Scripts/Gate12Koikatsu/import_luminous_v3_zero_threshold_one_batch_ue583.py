"""Gate12 Luminous real KKBP lossless-glTF shards, native UE5.8.3 incremental import.

Run in isolated UE editor, each actual source shard is hash-checked before
unmodified import. May resume only previously PASS native folders. Does NOT
declare a playable character, runtime clothing switch or on-device test.
"""
import pathlib,os,json,hashlib,time,traceback,sys,gc
import unreal
W=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
STATE=os.environ.get("G12_LUMINOUS_STATE","CLOTHED")
ID={"CLOTHED":"LuminousV3CLOTHED","UNDERWEAR":"LuminousV3UNDERWEAR","NUDE":"LuminousV3NUDE"}.get(STATE)
if not ID:raise RuntimeError("Unknown clothing state")
START=int(os.environ.get("G12_LUMINOUS_START_INDEX","0"))
MAX=int(os.environ.get("G12_LUMINOUS_MAX_IMPORT_PARTS","1"))
if MAX<1 or MAX>8 or START<0:raise RuntimeError("Invalid bounded batch parameters")
SOURCE=W/"Saved/Gate12Koikatsu/LuminousV3/LosslessSources"/ID/"shards"
MANIFEST=SOURCE/"manifest_full_fidelity_primitives.json"
QA=W/"Saved/Gate12Koikatsu/LuminousV3"/STATE
QA.mkdir(parents=True,exist_ok=True)
MORPH_AUDIT=QA/"source_shards_actual_nonzero_morph_inventory.json"
OUTPUT=QA/("native_shards_%03d_count%02d_ue583.json"%(START,MAX))
result={"status":"START","source":str(MANIFEST),"state":STATE,"start_index":START,
        "max_this_batch":MAX,"native_parts":[],"generation":"DIRECT_UE_INTERCHANGE_GLB",
        "artwork_fidelity":"SOURCE_BYTES_IMMUTABLE",
        "gameplay_ready":False,"gate11_artifact_ready":False,
        "original_installed_game_changed":False}
def save():
 result["updated_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())
 OUTPUT.write_text(json.dumps(result,indent=2,default=str)+"\n",encoding="utf8")
def checksum(path):
 with path.open("rb") as f:return hashlib.file_digest(f,"sha256").hexdigest()
save()
try:
 if "OfflineGame_Gate12_Koikatsu_20261010" not in str(unreal.Paths.get_project_file_path()):
  raise RuntimeError("Wrong Unreal project")
 base=json.loads(MANIFEST.read_text(encoding="utf-8-sig"))
 assert base["status"]=="SOURCE_LOSSLESS_SHARDS_VERIFIED"
 shards=base["shards"]
 assert len(shards)==base["full_primitive_count"]
 assert START<len(shards)
 morphaudit=json.loads(MORPH_AUDIT.read_text(encoding="utf-8-sig"))
 assert morphaudit["total_shards"]==len(shards)
 assert len(morphaudit["nonzero_by_shard"])==len(shards)
 result["whole_character_source_sha256"]=base["original_glb_sha256"]
 result["total_character_parts"]=len(shards)
 result["whole_character_morph_links"]=base["full_morph_links"]
 save()
 for index in range(START,min(START+MAX,len(shards))):
  row=shards[index]
  mesh=int(row["mesh_index"]);prim=int(row["primitive_index"])
  expected_path=pathlib.Path(row["path"])
  assert expected_path.parent.resolve()==SOURCE.resolve()
  assert expected_path.exists()
  actual=checksum(expected_path)
  assert actual==row["sha256"],"Lossless source file changed"
  dest="/Game/Experimental/LuminousValentineV3/"+STATE+("/Parts/Mesh%03dPrim%03d"%(mesh,prim))
  expected_nonzero=int(morphaudit["nonzero_by_shard"][index]["nonzero_position_morphs"])
  assert morphaudit["nonzero_by_shard"][index]["source_sha256"]==actual
  journal={"index":index,"mesh":mesh,"primitive":prim,
           "source":str(expected_path),"sha256":actual,"destination":dest,
           "source_morph_count":int(row["shard_primitive_counts"][0]["target_count"]),
           "source_nonzero_position_morphs":expected_nonzero,
           "status":"START"}
  result["native_parts"].append(journal);save()
  if unreal.EditorAssetLibrary.does_directory_exist(dest):
   raise RuntimeError("Fail-closed: destination exists, do not replace "+dest)
  task=unreal.AssetImportTask()
  task.filename=str(expected_path)
  task.destination_path=dest
  task.automated=True
  task.async_=False
  task.save=True
  task.replace_existing=False
  pipeline=unreal.InterchangeGenericAssetsPipeline()
  mesh_pipeline=pipeline.get_editor_property('mesh_pipeline')
  mesh_pipeline.set_editor_property('morph_threshold_position',0.0)
  if mesh_pipeline.get_editor_property('morph_threshold_position') != 0.0:
   raise RuntimeError('Explicit zero morph threshold not retained')
  task.options=pipeline
  unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
  names=list(unreal.EditorAssetLibrary.list_assets(dest,recursive=True,include_folder=False))
  assets=[];skeletons=[];morphs=0
  for path in names:
   asset=unreal.EditorAssetLibrary.load_asset(path)
   if asset is None:continue
   kind=asset.get_class().get_name()
   props={"path":str(path),"class":kind}
   if isinstance(asset,unreal.SkeletalMesh):
    sk=asset.get_editor_property("skeleton")
    count=len(sk.get_editor_property("bone_tree")) if sk else 0
    countm=len(asset.get_editor_property("morph_targets"))
    props.update({"skeletal_bones":count,"skeletal_morphs":countm})
    skeletons.append(props);morphs+=countm
   assets.append(props)
  skeletal_ok=bool(skeletons and max(q["skeletal_bones"] for q in skeletons)>=80)
  if not skeletal_ok:
   verdict="FAIL_NATIVE_SKELETAL_MESH_OR_RIG_NOT_IMPORTED"
  elif morphs>=expected_nonzero:
   verdict="PASS_NATIVE_UE_SKELETAL_SOURCE_AND_MORPHS"
  elif expected_nonzero>0 and morphs>=max(1,int(expected_nonzero*0.7)):
   verdict="PARTIAL_NATIVE_MORPH_LOSS_REQUIRES_REVIEW"
  else:
   verdict="FAIL_MAJOR_NATIVE_MORPH_LOSS"
  journal.update({"ue_asset_count":len(assets),"ue_skeletal_mesh_count":len(skeletons),
          "ue_total_native_morph_targets":morphs,"assets":assets,
          "effective_morphs_not_imported":max(0,expected_nonzero-morphs),
          "status":verdict})
  save()
  if verdict.startswith("FAIL"):raise RuntimeError("Native real source part invalid: "+verdict)
  # Full per-run details saved before memory reclaim.
  unreal.SystemLibrary.collect_garbage()
  gc.collect()
 result["total_effective_morphs_not_imported"]=sum(q.get("effective_morphs_not_imported",0) for q in result["native_parts"])
 result["status"]=("PARTIAL_NATIVE_SKELETAL_SOURCE_WITH_EXPRESSION_DIFFERENCES_NOT_PRODUCTION_ACCEPTED"
             if result["total_effective_morphs_not_imported"] else "PASS_THIS_BATCH_NATIVE_UE_SOURCE_AND_MORPHS")
except Exception as exc:
 result["status"]="FAIL_OR_PARTIAL_NATIVE_SHARD_BATCH"
 result["error"]=repr(exc)
 result["traceback"]=traceback.format_exc()[-5500:]
finally:
 save()
 unreal.log("G12_LUMINOUS_"+STATE+"_SHARD_BATCH="+result["status"])
