"""Native UE5.8.3 deterministic FBX import with isolated destination and type checks.
UnrealEditor-Cmd -run=pythonscript, job JSON in NW_UNREAL_JOB_PARAMS env.
Never writes to original FBX or any production NoWorld Content paths.
"""
import unreal,json,pathlib,hashlib,datetime,os,traceback
QA=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015")
params=json.loads(os.environ.get("NW_UNREAL_JOB_PARAMS","{}"))
default=QA/"NW_ModularWeapon_V2.fbx"
source=pathlib.Path(params.get("source",str(default))).resolve()
dest=params.get("destination","/Game/NW_MasterHub_QA/Unified_Master_Imports_v1")
kind=str(params.get("kind","static"))
expected=int(params.get("minimum_assets",1))
outfile=pathlib.Path(params.get("receipt",str(QA/"UE58_Unified_Import_Job_v1.json"))).resolve()
r={"status":"START","source":str(source),"destination":dest,"kind":kind,
    "engine":unreal.SystemLibrary.get_engine_version(),"asset_paths":[],
    "production_game_modified":False}
try:
 if not source.is_file():raise FileNotFoundError(source)
 if not dest.startswith("/Game/NW_MasterHub_QA/"):raise ValueError("Refuse unisolated UE Content destination")
 if dest=="/Game/NW_MasterHub_QA/":raise ValueError("Refuse broad folder")
 if outfile.exists():raise FileExistsError("Receipt already exists "+str(outfile))
 if kind not in ("static","animation"):raise ValueError("Unsupported import kind")
 def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
 source_hash=sha(source)
 unreal.EditorAssetLibrary.make_directory(dest)
 task=unreal.AssetImportTask()
 task.filename=str(source);task.destination_path=dest
 task.automated=True;task.replace_existing=False;task.save=True
 ui=unreal.FbxImportUI()
 ui.import_mesh=kind=="static"
 ui.import_as_skeletal=kind=="animation"
 ui.import_animations=kind=="animation"
 ui.import_textures=False;ui.import_materials=False
 if kind=="animation":
  ui.automated_import_should_detect_type=False
  ui.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION
  skeleton=unreal.load_asset(params.get("skeleton","/Game/NW_BlenderImportTests/Mount_Skeletal/NW_ArticulatedMount_Skeletal_Skeleton"))
  if skeleton is None:raise ValueError("Native Skeleton path missing")
  ui.skeleton=skeleton
 task.options=ui
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
 paths=[]
 for p in task.imported_object_paths:
  asset=unreal.load_asset(p)
  if asset:
   cls=asset.get_class().get_name()
   row={"path":str(p),"class":cls}
   if cls=="AnimSequence":
    row["frames"]=int(asset.get_editor_property("number_of_sampled_frames"))
    row["skeleton"]=asset.get_editor_property("skeleton").get_path_name()
   if cls=="StaticMesh":
    try:row["lod_count"]=asset.get_num_lods()
    except:pass
   paths.append(row)
 r["asset_paths"]=paths
 r["source_sha256"]=source_hash
 r["source_untouched"]=sha(source)==source_hash
 cls_required="StaticMesh" if kind=="static" else "AnimSequence"
 r["native_reloaded_count"]=sum(a["class"]==cls_required for a in paths)
 r["status"]="PASS_NATIVE_IMPORT" if r["native_reloaded_count"]>=expected and r["source_untouched"] else "INCOMPLETE"
 r["artist_accepted"]=False
 r["android_runtime_tested"]=False
except Exception:r["status"]="ERROR";r["error"]=traceback.format_exc()
outfile.parent.mkdir(parents=True,exist_ok=True)
outfile.write_text(json.dumps(r,indent=2),encoding="utf-8")
print("NW_UE_BATCH_IMPORT="+json.dumps({k:v for k,v in r.items() if k!="error"}))
