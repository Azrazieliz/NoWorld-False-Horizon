"""Repair a critical retarget issue: default IK Retargeter was missing chain assignments,
resulting in static imported target despite 44 sampled frames.
UE 5.8.3: explicit automatic chain matching + automatic pose alignment + FBX export.
"""
import unreal,json,traceback,pathlib
OUT=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015")
r={"engine":unreal.SystemLibrary.get_engine_version(),"status":"START"}
try:
 rt=unreal.load_asset("/Game/NW_StudioMocap/RTG_NW_CMU_to_KK")
 ctl=unreal.IKRetargeterController.get_controller(rt)
 r["ops_before"]=int(ctl.get_num_retarget_ops())
 if r["ops_before"]==0:ctl.add_default_ops()
 r["ops_after"]=int(ctl.get_num_retarget_ops())
 assert r["ops_after"]>0,"Retarget operation stack is empty"
 srcmesh=unreal.load_asset("/Game/NW_StudioMocap/Source/NW_CMU_FemalePunch_SourceSkeleton")
 trgmesh=unreal.load_asset("/Game/NW_StudioMocap/Target/NW_KK_121Bone_TargetSkeletal_Private")
 source=unreal.load_asset("/Game/NW_StudioMocap/ShortClip/NW_CMU_Punch_1p875sec_AnimOnly")
 assert rt and srcmesh and trgmesh and source
 targetchains=["Spine","Head","LeftLeg","LeftFoot","LeftClavicle","LeftArm","RightLeg","RightFoot","RightClavicle","RightArm"]
 r["before_mapping"]={k:str(ctl.get_source_chain(k)) for k in targetchains}
 ctl.auto_map_chains(unreal.AutoMapChainType.FUZZY,True)
 src_names=["Spine","Head","LeftLeg","LeftFoot","LeftClavicle","LeftArm","RightLeg","RightFoot","RightClavicle","RightArm"]
 r["manual_verification"]={}
 for name in src_names:
  r["manual_verification"][name]=bool(ctl.set_source_chain(name,name))
 r["after_mapping"]={k:str(ctl.get_source_chain(k)) for k in targetchains}
 r["mapped_count"]=sum(r["after_mapping"][k]==k for k in targetchains)
 assert r["mapped_count"]==len(targetchains),r["after_mapping"]
 try:
  ctl.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
  r["alignment"]="TARGET_AUTO_ALIGNED_CHAINS"
 except Exception as e:r["alignment"]="POSE_AUTO_ALIGNMENT_FAIL "+str(e)
 unreal.EditorAssetLibrary.save_loaded_asset(rt)
 d=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem).find_asset_data(source.get_path_name())
 out=unreal.IKRetargetBatchOperation.duplicate_and_retarget(
 [d],srcmesh,trgmesh,rt,search="",replace="",
 prefix="NW_KK_FixMapped_",suffix="_ChainMappedV3")
 assert out,"No retarget action"
 r["native_paths"]=[str(x.package_name) for x in out]
 anim=unreal.load_asset(str(out[0].package_name))
 assert anim and anim.get_class().get_name()=="AnimSequence"
 unreal.EditorAssetLibrary.save_loaded_asset(anim)
 r["native_frames"]=int(anim.get_editor_property("number_of_sampled_frames"))
 r["native_target_skeleton"]=anim.get_editor_property("skeleton").get_path_name()
 exporter=unreal.AssetExportTask()
 exporter.object=anim
 exporter.filename=str(OUT/"NW_KK_ChainMappedV2_ActualMotion.fbx")
 exporter.automated=True;exporter.prompt=False;exporter.replace_identical=True
 exporter.exporter=unreal.AnimSequenceExporterFBX()
 exporter.options=unreal.AnimSeqExportOption()
 r["export_ok"]=bool(unreal.Exporter.run_asset_export_task(exporter))
 r["export_bytes"]=(OUT/"NW_KK_ChainMappedV2_ActualMotion.fbx").stat().st_size if exporter and (OUT/"NW_KK_ChainMappedV2_ActualMotion.fbx").exists() else 0
 r["status"]="CHAIN_MAPPED_NATIVE_EXPORT_PASS" if r["export_ok"] and r["export_bytes"]>5000 else "PARTIAL"
 r["pose_deformation_accepted"]=False
except Exception:
 r["status"]="ERROR";r["traceback"]=traceback.format_exc()
(OUT/"UE58_IK_FixedChainMap.json").write_text(json.dumps(r,indent=2))
print("NOWORLD_IK_MAPPING_FIX="+json.dumps({k:v for k,v in r.items() if k!="traceback"}))
