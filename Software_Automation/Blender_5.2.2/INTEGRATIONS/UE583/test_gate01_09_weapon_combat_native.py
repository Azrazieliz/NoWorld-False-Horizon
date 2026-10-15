"""Gate01 weapon + Gate09 two real CMU combat captures -> native Unreal 5.8.3.
Real UE assets in isolated testing project only. Retarget anatomy/visual approval NOT implied.
"""
import unreal,json,os,traceback
root=r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015"
r={"engine":unreal.SystemLibrary.get_engine_version(),"status":"STARTED","motions":[]}
try:
 tools=unreal.AssetToolsHelpers.get_asset_tools()
 weapon=unreal.AssetImportTask()
 weapon.filename=os.path.join(root,"NW_ModularWeapon_V2.fbx")
 weapon.destination_path="/Game/NW_MasterHub_QA/Gate01_Weapons"
 weapon.automated=True;weapon.save=True;weapon.replace_existing=False
 wo=unreal.FbxImportUI();wo.import_mesh=True;wo.import_as_skeletal=False
 wo.import_materials=False;wo.import_textures=False;weapon.options=wo
 tools.import_asset_tasks([weapon])
 r["weapon_assets"]=[{"path":str(p),"class":unreal.load_asset(p).get_class().get_name()}
    for p in weapon.imported_object_paths if unreal.load_asset(p)]
 r["weapon_static_mesh_count"]=sum(x["class"]=="StaticMesh" for x in r["weapon_assets"])
 assert r["weapon_static_mesh_count"]>=20,"Weapon assembly not imported"
 skeleton=unreal.load_asset("/Game/NW_StudioMocap/Source/NW_CMU_FemalePunch_SourceSkeleton_Skeleton")
 srcmesh=unreal.load_asset("/Game/NW_StudioMocap/Source/NW_CMU_FemalePunch_SourceSkeleton")
 tgtmesh=unreal.load_asset("/Game/NW_StudioMocap/Target/NW_KK_121Bone_TargetSkeletal_Private")
 rtg=unreal.load_asset("/Game/NW_StudioMocap/RTG_NW_CMU_to_KK")
 assert all((skeleton,srcmesh,tgtmesh,rtg))
 for label in ("Kick","Sword"):
  task=unreal.AssetImportTask()
  task.filename=os.path.join(root,"NW_Combat_"+label+".fbx")
  task.destination_path="/Game/NW_MasterHub_QA/Gate09_FightSource"
  task.automated=True;task.save=True;task.replace_existing=False
  ui=unreal.FbxImportUI();ui.import_mesh=False;ui.import_as_skeletal=True;ui.import_animations=True
  ui.automated_import_should_detect_type=False
  ui.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION
  ui.skeleton=skeleton;task.options=ui
  tools.import_asset_tasks([task])
  imported=[unreal.load_asset(x) for x in task.imported_object_paths if unreal.load_asset(x)]
  seq=next((x for x in imported if x.get_class().get_name()=="AnimSequence"),None)
  assert seq,"Native source AnimSequence missing "+label
  data=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem).find_asset_data(seq.get_path_name())
  clone=unreal.IKRetargetBatchOperation.duplicate_and_retarget(
      [data],srcmesh,tgtmesh,rtg,search="",replace="",prefix="NW_KK_"+label+"_",suffix="_AutoIK")
  assert clone,label
  target=unreal.load_asset(str(clone[0].package_name))
  assert target.get_class().get_name()=="AnimSequence"
  target_frames=int(target.get_editor_property("number_of_sampled_frames"))
  count_seconds=target_frames/24
  unreal.AnimationLibrary.add_animation_notify_track(target,"NW_CombatPhase")
  count=0
  for phase,fraction in [("Anticipation",.17),("Contact",.43),("Recovery",.79)]:
   notification=unreal.AnimationLibrary.add_animation_notify_event(
    target,"NW_CombatPhase",max(.02,min(count_seconds-.04,count_seconds*fraction)),
    unreal.AnimNotify_PlaySound)
   if notification:count+=1
  unreal.EditorAssetLibrary.save_loaded_asset(target)
  final=unreal.load_asset(target.get_path_name())
  r["motions"].append({"name":label,"source":seq.get_path_name(),"native_retarged":final.get_path_name(),
   "frames":target_frames,"native_notify_count":len(unreal.AnimationLibrary.get_animation_notify_events(final)),
   "skeleton":final.get_editor_property("skeleton").get_path_name()})
 r["status"]="PASS_NATIVE_MULTI_DOMAIN" if len(r["motions"])==2 else "PARTIAL"
 r["original_game_assets_modified"]=False
 r["professional_motion_accepted"]=False
 r["runtime_playback_accepted"]=False
except Exception:
 r["status"]="ERROR";r["error"]=traceback.format_exc()
finally:
 with open(os.path.join(root,"Gate01_Gate09_UE583_final_reuse.json"),"w",encoding="utf-8") as f:json.dump(r,f,indent=2)
 print("NW_MULTI_GATE_FINAL_UE="+json.dumps({k:v for k,v in r.items() if k!="error"}))
