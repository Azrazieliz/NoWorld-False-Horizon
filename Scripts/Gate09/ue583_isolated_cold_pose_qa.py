"""Gate09 cold native Unreal 5.8.3 source animation integrity and pose differences.
QA project is isolated from Gate12, always read-only.
"""
import unreal,json,traceback,os,math,time
R=r"D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019"
IN=os.path.join(R,"Saved","Gate09QA","unreal_02_track_verified_batch.json")
OUT=os.path.join(R,"Saved","Gate09QA","unreal_03_cold_pose_qa.json")
reports={"status":"STARTED","read_only":True,"project":str(unreal.Paths.get_project_file_path()),"clips":[]}
def save():
 with open(OUT,"w",encoding="utf8") as f:json.dump(reports,f,indent=2,default=str)
save()
try:
 assert "Gate09_Unreal583_Animation_QA_20261019" in reports["project"]
 rows=json.load(open(IN,encoding="utf8"))["records"]
 assert len(rows)==4
 for row in rows:
  obj=unreal.load_asset(row["asset"])
  assert isinstance(obj,unreal.AnimSequence),row["asset"]
  sk=obj.get_editor_property("skeleton")
  assert sk.get_path_name()==row["skeleton"]
  model=obj.get_editor_property("controller").get_model_interface()
  names=[str(x) for x in model.get_bone_track_names()]
  bones=[str(x) for x in sk.get_reference_pose().get_bone_names()]
  canon={x.casefold() for x in bones}
  common={x.casefold() for x in names}&canon
  clip={"name":row["filename"],"path":row["asset"],
  "native_unreal_tracks":len(names),"skeleton_bones":len(bones),
  "case_insensitive_bone_matches":len(common),
  "missing_skeleton_tracks":sorted(canon-{x.casefold() for x in names})[:25],
  "unknown_track_names":sorted({x.casefold() for x in names}-canon)[:25],
  "frames":model.get_number_of_frames(),
  "duration":obj.get_play_length(),"frame_rate":str(model.get_frame_rate())}
  try:
   opts=unreal.AnimPoseEvaluationOptions()
   frames=[0,max(1,model.get_number_of_frames()//3),max(2,model.get_number_of_frames()*2//3)]
   poses=[obj.get_anim_pose_at_frame(int(i),opts) for i in frames]
   clip["pose_evaluation_type"]=str(type(poses[0]))
   clip["pose_methods"]=[x for x in dir(poses[0]) if "bone" in x or "pose" in x][:25]
   if hasattr(poses[0],"get_bone_pose"):
    moving={}
    wanted=["Center","Hips","Spine","Chest","Upper-Chest","Head","Left arm","Right arm","Left leg","Right leg","cf_j_hips"]
    for name in wanted:
     if name.casefold() not in canon:continue
     vals=[]
     for pose in poses:
      try:
       tf=pose.get_bone_pose(unreal.Name(name),unreal.AnimPoseSpaces.LOCAL)
       q=tf.rotation;v=tf.translation
       vals.append([q.x,q.y,q.z,q.w,v.x,v.y,v.z])
      except Exception as e:
       clip.setdefault("individual_pose_errors",[]).append(name+":"+str(e)[:125]);break
     if len(vals)==3:
      first=vals[0]
      dp=max(math.sqrt(sum((a-b)**2 for a,b in zip(first,v))) for v in vals[1:])
      moving[name]=dp
    clip["sampled_bone_pose_changes"]=moving
  except Exception as e:clip["pose_eval_error"]=str(e)
  reports["clips"].append(clip);save()
 assert len(reports["clips"])==4
 assert all(x["native_unreal_tracks"]>=100 for x in reports["clips"])
 assert all(x["case_insensitive_bone_matches"]>=900 for x in reports["clips"]), "unresolved skeleton tracks"
 reports["status"]="PASS_FOUR_NATIVE_UNREAL_COLD_LOAD_AND_CASEFOLD_BONE_MAPPING"
except Exception:
 reports["status"]="FAIL_NATIVE_COLD_LOAD";reports["exception"]=traceback.format_exc()
finally:
 save();unreal.log("GATE09_NATIVE_UNREAL_COLD_QA="+reports["status"])
