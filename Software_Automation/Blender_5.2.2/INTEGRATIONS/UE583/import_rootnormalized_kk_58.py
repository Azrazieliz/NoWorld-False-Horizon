import unreal,json,traceback,pathlib
Q=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015")
r={"engine":unreal.SystemLibrary.get_engine_version(),"status":"START"}
try:
 target=unreal.load_asset("/Game/NW_StudioMocap/Target/NW_KK_121Bone_TargetSkeletal_Private_Skeleton")
 assert target
 opt=unreal.FbxImportUI()
 opt.import_mesh=False;opt.import_as_skeletal=True
 opt.import_animations=True;opt.automated_import_should_detect_type=False
 opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION;opt.skeleton=target
 task=unreal.AssetImportTask()
 task.filename=str(Q/"NW_KK_Mocap_RootNormalized_AnimOnly.fbx")
 task.destination_path="/Game/NW_MasterHub_QA/RetargetRootNormalized"
 task.automated=True;task.save=True;task.replace_existing=False;task.options=opt
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
 a=[unreal.load_asset(p) for p in task.imported_object_paths if unreal.load_asset(p)]
 seq=next((x for x in a if x.get_class().get_name()=="AnimSequence"),None)
 assert seq
 r["native_asset"]=seq.get_path_name()
 r["frames"]=int(seq.get_editor_property("number_of_sampled_frames"))
 r["skeleton"]=seq.get_editor_property("skeleton").get_path_name()
 r["status"]="NATIVE_IMPORT_PASS"
 r["visually_accepted"]=False
except Exception:r["status"]="ERROR";r["error"]=traceback.format_exc()
(Q/"UE58_KK_RootNormalized_NativeImport.json").write_text(json.dumps(r,indent=2))
print("NW_REPAIRED_KK_UE="+json.dumps({k:v for k,v in r.items() if k!="error"}))
