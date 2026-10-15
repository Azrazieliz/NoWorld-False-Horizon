"""Test 4.5.10 animation-only FBX in isolated Unreal 5.8.3 sandbox."""
import unreal,json,os,traceback
ROOT=r"D:\UnrealProjects\NW_BlenderInterop_Sandbox_20261015"
FBX=r"D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Blender_4.5.10\VALIDATION\phase2\NW_Mount_Walk_Test_AnimOnly.fbx"
r={"source":FBX,"engine":unreal.SystemLibrary.get_engine_version(),"status":"STARTED"}
try:
 path="/Game/NW_BlenderImportTests/Mount_Skeletal/NW_ArticulatedMount_Skeletal_Skeleton.NW_ArticulatedMount_Skeletal_Skeleton"
 skeleton=unreal.EditorAssetLibrary.load_asset(path)
 assert skeleton
 task=unreal.AssetImportTask();task.filename=FBX
 task.destination_path="/Game/NW_BlenderImportTests/Mount_Walk_AnimOnly"
 task.automated=True;task.save=True;task.replace_existing=False
 ui=unreal.FbxImportUI();ui.import_mesh=False;ui.import_as_skeletal=True
 ui.import_animations=True;ui.automated_import_should_detect_type=False
 ui.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION
 ui.skeleton=skeleton
 task.options=ui
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
 r["paths"]=list(task.imported_object_paths)
 r["classes"]=[unreal.EditorAssetLibrary.load_asset(p).get_class().get_name() for p in task.imported_object_paths if unreal.EditorAssetLibrary.load_asset(p)]
 r["anim_sequence_count"]=r["classes"].count("AnimSequence")
 r["status"]="PASS" if r["anim_sequence_count"]>0 else "FAILED_NO_ANIM_SEQUENCE"
except Exception:r["status"]="ERROR";r["exception"]=traceback.format_exc()
finally:
 with open(os.path.join(ROOT,"ue_anim_only_results.json"),"w") as f:json.dump(r,f,indent=2)
 print("NW_UE_ANIMONLY="+json.dumps(r))
