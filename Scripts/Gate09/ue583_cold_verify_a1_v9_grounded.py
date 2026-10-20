"""Single independent cold engine verification of changed A1 v9 root motion,
reused v7 notify classes and unchanged approved Luminous reference skeleton.
"""
import unreal,json,pathlib,math,time,traceback
ROOT=pathlib.Path(r'D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019')
Q=ROOT/'Saved/Gate09QA'
source=json.loads((Q/'unreal_21_a1_v9_grounded_original_native.json').read_text(encoding='utf8'))
OUT=Q/'unreal_22_a1_v9_FINAL_cold_root_notifies.json'
r={'status':'STARTED','no_asset_reimports':True,'events':[]}
try:
 assert source['status'].startswith('PASS_975_TRACKS_')
 a=unreal.load_asset(source['native_asset']);assert isinstance(a,unreal.AnimSequence)
 mdl=a.get_editor_property('controller').get_model_interface()
 refs={str(x).casefold() for x in a.get_editor_property('skeleton').get_reference_pose().get_bone_names()}
 bones={str(x).casefold() for x in mdl.get_bone_track_names()}
 assert len(refs)==len(bones)==975 and refs==bones
 ts=[]
 for k in [0,5,11,19,31]:
  pose=a.get_anim_pose_at_frame(k,unreal.AnimPoseEvaluationOptions())
  v=pose.get_bone_pose(unreal.Name('Center'),unreal.AnimPoseSpaces.LOCAL).translation
  ts.append((k,float(v.x),float(v.y),float(v.z)))
 offset=max(math.dist(row[1:],ts[0][1:]) for row in ts)
 assert offset>=30,(offset,ts)
 r['root_motion_cm']=offset
 r['native_bone_tracks']=len(bones)
 r['center_motion_samples']=ts
 for e in source['notify_events']:
  tr=unreal.Name(e['name'])
  events=unreal.AnimationLibrary.get_animation_notify_events_for_track(a,tr)
  assert len(events)==1,(e['name'],len(events))
  evt=events[0]
  actual=float(unreal.AnimationLibrary.get_anim_notify_event_trigger_time(evt))
  assert abs(actual-e['seconds'])<.035,(e['name'],actual,e['seconds'])
  cls=evt.get_editor_property('notify')
  assert cls is not None,(e['name'],'non-concrete')
  r['events'].append({'name':e['name'],'t':actual,'class':cls.get_class().get_name()})
 assert len(r['events'])==5
 assert len(unreal.AnimationLibrary.get_animation_notify_events(a))==5
 r['status']='PASS_GROUNDED_V9_35CM_AND_FIVE_CONCRETE_EVENTS_COLD'
except Exception:
 r['status']='FAIL_GROUNDED_V9_COLD'
 r['error']=traceback.format_exc()
finally:
 r['utc']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
 OUT.write_text(json.dumps(r,indent=2)+'\n',encoding='utf8')
 unreal.log('G09_A1_V9_COLD='+r['status'])
