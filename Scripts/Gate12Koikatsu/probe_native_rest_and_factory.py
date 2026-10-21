import unreal,json,pathlib,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu")
out={"status":"STARTED","api":{},"samples":{}}
try:
 seq=unreal.load_asset("/Game/Experimental/Gate12Koikatsu/Animation/AN_ChikaHaruno_SpineSway_UE583")
 for name in ("data_model","controller","skeleton","number_of_sampled_keys"):
  try:
   v=seq.get_editor_property(name)
   out["samples"]["existing_"+name]=str(v)[:500]
   if name=="data_model":
    out["samples"]["old_model_bone_tracks"]=[str(x) for x in v.get_bone_track_names()]
    out["samples"]["old_model_frame_count"]=v.get_number_of_frames()
    for b in ("Center","pelvis","spine_02","upperarm_l","upperarm_r"):
     try:
      tr=v.get_bone_track_by_name(b)
      out["samples"]["old_model_"+b]=str(tr)[:1000]
     except Exception as e:out["samples"]["old_model_"+b+"_error"]=str(e)[:220]
  except Exception as e:out["samples"]["existing_"+name+"_error"]=str(e)[:170]
 for clname in ("SkeletalMesh","Skeleton","SkeletalMeshComponent","AnimSequenceFactory","AnimDataController","AnimDataModel","BoneAnimationTrack","Transform"):
  cls=getattr(unreal,clname)
  out["api"][clname]={n:str(getattr(cls,n).__doc__)[:550] for n in dir(cls) if (
    any(x in n.lower() for x in ("ref_pose","reference","bone","skeleton","animation","target_skeleton","frame_rate","set_","keys","data_model","controller"))
    and not n.startswith("_"))}
 for b in ("Center","pelvis","spine_02","upperarm_l","upperarm_r"):
  try:
   out["samples"]["old_anim_pose_"+b]=[str(unreal.AnimationLibrary.get_bone_pose_for_frame(seq,b,i,False)) for i in (0,15,30,45,60)]
  except Exception as e:out["samples"]["old_anim_pose_"+b+"_error"]=str(e)[:250]
 out["status"]="PASS"
except Exception as e:out["status"]="FAIL";out["error"]=str(e);out["trace"]=traceback.format_exc()[-1200:]
finally:
 (R/"native_rest_and_anim_factory_probe.json").write_text(json.dumps(out,indent=2,default=str),encoding="utf8")
 unreal.log("G12_REST_PROBE_"+out["status"])
