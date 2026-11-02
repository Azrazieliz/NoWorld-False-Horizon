import unreal,pathlib,json,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu")
o={"status":"STARTED","api":{},"result":{}}
try:
 for n in ("AnimPose","AnimPoseExtensions","AnimationLibrary","AnimDataController","FrameRate","AnimSequenceFactory","AssetToolsHelpers"):
  try:
   cls=getattr(unreal,n)
   o["api"][n]={m:str(getattr(cls,m).__doc__)[:500] for m in dir(cls) if (any(k in m.lower() for k in ("bone","pose","local","model","curve","bracket","set_frame","number_of_frames","get_num","get_controller","set_sequence","factory")) and not m.startswith("_"))}
  except Exception as e:o["api"][n]={"ERROR":str(e)}
 sk=unreal.load_asset("/Game/Experimental/Gate12Koikatsu/FullPrimitives/Mesh03Prim11/Mesh_03_Prim_11/SkeletalMeshes/Mesh_03_Prim_11_Skeleton")
 if not sk:raise RuntimeError("Missing exact target skeleton")
 pose=sk.get_reference_pose()
 o["result"]["pose_class"]=pose.__class__.__name__
 o["result"]["pose_repr"]=str(pose)[:400]
 o["result"]["pose_public_methods"]=[x for x in dir(pose) if "bone" in x.lower() or "pose" in x.lower() or "transform" in x.lower()]
 o["status"]="PASS"
except Exception as e:o["status"]="FAIL";o["error"]=str(e);o["trace"]=traceback.format_exc()[-1200:]
finally:
 (R/"probe_reference_pose_extension.json").write_text(json.dumps(o,indent=2,default=str),encoding="utf8")
 unreal.log("G12_REFPOSE_METHODS_"+o["status"])
