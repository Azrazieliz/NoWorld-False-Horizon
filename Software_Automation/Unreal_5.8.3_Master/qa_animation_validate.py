"""Read-only Unreal 5.8.3 AnimSequence fidelity auditor. Strictly fail closed.
Checks that joints actually move between frames, not merely that FBX imported.
"""
import unreal,os,pathlib,json,math,traceback
OUT=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015\UE58_Gate09_MotionFidelity.json")
settings=json.loads(os.environ.get("NW_UNREAL_JOB_PARAMS","{}"))
assets=settings.get("animations",[
 "/Game/NW_MasterHub_QA/Gate09_Motion/NW_Gate09_flight_Semantic",
 "/Game/NW_MasterHub_QA/Gate09_Motion/NW_Gate09_swim_Semantic"])
outpath=pathlib.Path(settings.get("receipt",str(OUT)))
if outpath.exists():raise FileExistsError("No overwrite "+str(outpath))
info={"engine":unreal.SystemLibrary.get_engine_version(),"status":"START",
  "game_project_modified":False,"pose_api_doc":str(unreal.AnimationLibrary.get_bone_pose_for_frame.__doc__)[:2100],"assets":[]}
for p in assets:
 r={"path":p,"status":"START"}
 try:
  a=unreal.load_asset(p)
  if not a or a.get_class().get_name()!="AnimSequence":raise ValueError("Not a native AnimSequence")
  frames=int(a.get_editor_property("number_of_sampled_frames"))
  if frames<12:raise ValueError("Too few frames")
  skeleton=a.get_editor_property("skeleton")
  names=list(unreal.AnimationLibrary.get_animation_track_names(a))
  if not names:raise RuntimeError("AnimSequence has zero native animation tracks")
  results=[]
  for bone in names[:min(18,len(names))]:
   try:
    poses=[]
    for f in [0,frames//4,frames//2,frames-1]:
     tr=unreal.AnimationLibrary.get_bone_pose_for_frame(a,bone,f,False)
     loc=tr.get_editor_property("translation")
     rot=tr.get_editor_property("rotation")
     poses.append(([loc.x,loc.y,loc.z],[rot.x,rot.y,rot.z,rot.w]))
    d=max(math.sqrt(sum((v-w)**2 for v,w in zip(poses[0][0],x[0]))+
      sum((v-w)**2 for v,w in zip(poses[0][1],x[1]))) for x in poses[1:])
    results.append({"bone":str(bone),"max_change":round(d,8)})
   except Exception as e:
    results.append({"bone":str(bone),"failed_to_sample":str(e)[:180]})
  moved=[x for x in results if x.get("max_change",0)>0.001]
  r.update(status="MOTION_VERIFIED" if moved else "MOTION_STATIC_OR_UNREADABLE",
   class_name=a.get_class().get_name(),frames=frames,
   skeleton=skeleton.get_path_name() if skeleton else None,
   num_tracks=len(names),sampled_bone_tracks=results,
   moving_bone_count=len(moved),visual_quality_approved=False)
 except Exception:r.update(status="ERROR",traceback=traceback.format_exc()[-1500:])
 info["assets"].append(r)
info["status"]="PASS_NATIVE_ANIMATION_MOTION" if all(x["status"]=="MOTION_VERIFIED" for x in info["assets"]) else "UNVERIFIED_MOTION"
outpath.parent.mkdir(parents=True,exist_ok=True)
outpath.write_text(json.dumps(info,indent=2),encoding="utf-8")
print("NW_UE_ANIM_FIDELITY="+json.dumps({"status":info["status"],
  "rows":[{"path":v["path"],"status":v["status"],"moving_bones":v.get("moving_bone_count")} for v in info["assets"]]}))
