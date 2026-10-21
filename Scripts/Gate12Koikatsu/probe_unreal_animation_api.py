import unreal,json,pathlib,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu")
out={"status":"STARTED","functions":{}}
try:
 funcs={"override_animation_data":unreal.SkeletalMeshComponent.override_animation_data,
 "set_animation":unreal.SkeletalMeshComponent.set_animation,
 "play_animation":unreal.SkeletalMeshComponent.play_animation,
 "skeleton_add_compatible":unreal.Skeleton.add_compatible_skeleton,
 "get_anim_pose_at_time":unreal.AnimSequence.get_anim_pose_at_time,
 "get_morph_target":unreal.SkeletalMeshComponent.get_morph_target,
 "set_morph_target":unreal.SkeletalMeshComponent.set_morph_target}
 for name,f in funcs.items():
  out["functions"][name]=str(f.__doc__)[:4000]
 out["status"]="COMPLETE"
except Exception as e:out["status"]="ERROR";out["error"]=str(e);out["trace"]=traceback.format_exc()
finally:
 (R/"anim_api_docs_probe.json").write_text(json.dumps(out,indent=2,default=str),encoding="utf8")
 unreal.log("G12_ANIM_API_PROBE_"+out["status"])
