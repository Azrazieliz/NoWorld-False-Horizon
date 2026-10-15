"""UE 5.8 native verification of non-destructive 200-to-9 Blender export batching."""
import unreal,json,os,traceback
ROOT=r"D:\UnrealProjects\NW_BlenderInterop_Sandbox_20261015"
FBX=r"D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Blender_4.5.10\VALIDATION\phase2\NW_ParametricSanctuary_Batched.fbx"
r={"engine":unreal.SystemLibrary.get_engine_version(),"source":FBX,"status":"STARTED"}
try:
 task=unreal.AssetImportTask();task.filename=FBX
 task.destination_path="/Game/NW_BlenderImportTests/Sanctuary_Batched"
 task.automated=True;task.save=True;task.replace_existing=False
 opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_as_skeletal=False
 opt.import_materials=False;opt.import_textures=False
 task.options=opt
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
 r['asset_paths']=[str(x) for x in task.imported_object_paths]
 r['classes']=[unreal.EditorAssetLibrary.load_asset(x).get_class().get_name() for x in task.imported_object_paths if unreal.EditorAssetLibrary.load_asset(x)]
 r['num_native_static_meshes']=sum(x=="StaticMesh" for x in r['classes'])
 r['status']="PASS" if r['num_native_static_meshes']==9 else "FAIL"
except Exception:
 r['status']="ERROR";r['exception']=traceback.format_exc()
finally:
 with open(os.path.join(ROOT,'ue_batch_results.json'),'w') as f:json.dump(r,f,indent=2)
 print("NOWORLD_UE_BATCH="+json.dumps(r))
