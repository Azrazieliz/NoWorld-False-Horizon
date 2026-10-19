"""Gate09 independent cold UE load and motion QA for new unmodified combat source AnimSequences."""
import unreal,os,json,traceback,time,math
ROOT=r"D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019"
PATH="/Game/Gate09_QA_20261019/DharmaWeaponSource/"
NAMES=["G09_LuminousV3_Dharma_Hit01_DiagonalDraw","G09_LuminousV3_Dharma_Hit02_ReturnSlash",
       "G09_LuminousV3_Dharma_Hit03_FinishingThrust"]
OUT=os.path.join(ROOT,"Saved/Gate09QA/unreal_07_dharma_native_cold_pose.json")
r={'status':'STARTED','asset_count':0,'records':[],'read_only':True}
def save():
 with open(OUT,'w',encoding='utf8') as f:json.dump(r,f,indent=2,default=str)
save()
try:
 assert "Gate09_Unreal583_Animation_QA_20261019" in str(unreal.Paths.get_project_file_path())
 r['notify_api']={x:str(getattr(unreal.AnimationLibrary,x).__doc__)[:500] for x in dir(unreal.AnimationLibrary) if 'notify' in x.lower()}
 for name in NAMES:
  a=unreal.load_asset(PATH+name)
  assert isinstance(a,unreal.AnimSequence)
  md=a.get_editor_property('controller').get_model_interface()
  bones={str(x).casefold() for x in md.get_bone_track_names()}
  ref={str(x).casefold() for x in a.get_editor_property('skeleton').get_reference_pose().get_bone_names()}
  assert len(bones)==len(ref)==975 and bones==ref
  frames=md.get_number_of_frames()
  poses=[]
  for k in [0,max(1,frames//4),frames//2,frames-1]:
   pose=a.get_anim_pose_at_frame(k,unreal.AnimPoseEvaluationOptions())
   row={}
   for n in ['Head','Center','Left wrist','Right wrist','Left leg','Right leg']:
    try:
     tf=pose.get_bone_pose(unreal.Name(n),unreal.AnimPoseSpaces.LOCAL)
     q=tf.rotation;loc=tf.translation
     row[n]=[float(q.x),float(q.y),float(q.z),float(q.w),
             float(loc.x),float(loc.y),float(loc.z)]
    except Exception:pass
   poses.append({'frame':k,'bones':row})
  diffs={}
  for b in poses[0]['bones']:
   p=[x['bones'][b] for x in poses if b in x['bones']]
   diffs[b]=max(sum((x[i]-p[0][i])**2 for i in range(7))**.5 for x in p)
  assert max(diffs.values())>0.002
  r['records'].append({'name':name,'frames':frames,'duration_s':a.get_play_length(),
      'tracks':len(bones),'casefold_exact_rig':True,'sample_deltas':diffs,
      'motion_nonstatic':True,'source_unmodified':True})
 r['asset_count']=len(r['records'])
 r['status']='PASS_NATIVE_SWORD_SOURCES_COLD_MOTION'
except Exception:
 r['status']='FAIL_COMBAT_COLD';r['error']=traceback.format_exc()
finally:
 r['time_utc']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
 save();unreal.log('G09_DHARMA_COLD_'+r['status'])
