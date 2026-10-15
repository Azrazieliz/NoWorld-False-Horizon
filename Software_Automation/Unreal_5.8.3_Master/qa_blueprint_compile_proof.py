"""Unreal native isolated Blueprint compiler proof; no game branch writes."""
import unreal,json,pathlib,traceback,os,re
params=json.loads(os.environ.get('NW_UNREAL_JOB_PARAMS','{}'))
job_id=str(params.get('job_id','BlueprintCompileProof_20261015_2120'))
if not re.fullmatch('[A-Za-z0-9_]{4,75}',job_id):raise ValueError('Unsafe Blueprint QA job id')
qa=pathlib.Path(r'D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015').resolve()
out=pathlib.Path(params.get('receipt',str(qa/(job_id+'.json')))).resolve()
if not out.is_relative_to(qa):raise ValueError('Receipt outside private QA folder')
folder='/Game/NW_MasterHub_QA/'+job_id
result={"schema":"NW_UE_BP_COMPILE_PROOF_V1","sandbox":True,"production_modified":False}
try:
 if out.exists():raise FileExistsError(str(out))
 unreal.EditorAssetLibrary.make_directory(folder)
 fac=unreal.BlueprintFactory()
 fac.set_editor_property("parent_class",unreal.Actor)
 name="NW_BP_CompilationProof"
 path=folder+"/"+name
 if unreal.EditorAssetLibrary.does_asset_exist(path):raise FileExistsError(path)
 bp=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,unreal.Blueprint,fac)
 if not bp:raise RuntimeError("Blueprint creation failed")
 unreal.BlueprintEditorLibrary.compile_blueprint(bp)
 if not unreal.EditorAssetLibrary.save_loaded_asset(bp):raise RuntimeError("Unreal did not save Blueprint")
 reloaded=unreal.load_asset(path)
 cls=reloaded.generated_class() if reloaded else None
 result.update(asset_path=path,created=True,saved_and_reloaded=bool(reloaded),generated_class=bool(cls),
   compiled_via_native_editor_library=True,status="PASS_NATIVE_BLUEPRINT_COMPILE" if cls else "FAIL_NO_CLASS")
except Exception:
 result.update(status="FAIL",error=traceback.format_exc()[-2500:])
out.write_text(json.dumps(result,indent=2),encoding="utf-8")
print("NW_BP_COMPILE_PROOF="+json.dumps(result),flush=True)
