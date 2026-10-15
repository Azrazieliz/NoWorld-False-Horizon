"""Gate02 architectural modular source -> real Unreal 5.8.3 StaticMeshes, sandbox ONLY."""
import unreal,os,json,traceback
ROOT=r"D:\UnrealProjects\NW_BlenderInterop_Sandbox_20261015"
QA=r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015"
result={"engine":unreal.SystemLibrary.get_engine_version(),"status":"STARTED"}
try:
 task=unreal.AssetImportTask()
 task.filename=os.path.join(QA,"NW_ArchGenerator3Styles.fbx")
 task.destination_path="/Game/NW_MasterHub_QA/Architecture_Gate02"
 task.automated=True;task.replace_existing=False;task.save=True
 ui=unreal.FbxImportUI();ui.import_mesh=True;ui.import_as_skeletal=False
 ui.import_materials=False;ui.import_textures=False
 task.options=ui
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
 res=[]
 for p in task.imported_object_paths:
  asset=unreal.load_asset(p)
  if asset:res.append({"path":str(p),"type":asset.get_class().get_name()})
 result["assets"]=res
 result["native_static_meshes"]=sum(x["type"]=="StaticMesh" for x in res)
 result["status"]="PASS_UNREAL_NATIVE" if result["native_static_meshes"]==3 else "FAILED_UNEXPECTED_IMPORT"
 result["real_production_game_import"]=False
 result["artistic_quality_accepted"]=False
except Exception:result["status"]="ERROR";result["error"]=traceback.format_exc()
finally:
 with open(os.path.join(QA,"Gate02_UE583_native_import.json"),"w",encoding="utf-8") as f:json.dump(result,f,indent=2)
 print("NOWORLD_ARCH_UE="+json.dumps({k:v for k,v in result.items() if k!="error"}))
