"""Gate09 correct UE5.8 cold reload: empty notify tracks are culled; recreate tracks
before adding five concrete AnimNotify Blueprint events, reuse prior created class.
Do not modify Gate12 / frozen assets. Validate persisted after separate cold editor.
"""
import unreal,pathlib,json,traceback,time
ROOT=pathlib.Path(r'D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019')
OUT=ROOT/'Saved/Gate09QA/unreal_17_a1_v7_concrete_notify_blueprints_FORCED_SAVE.json'
ASSET='/Game/Gate09_QA_20261019/LuminousA1NLA_v7/G09_LuminousV3_A1_ScarletCadence_RealSword_NLA_v7'
DEST='/Game/Gate09_QA_20261019/LuminousA1NotifyClasses_v7'
r={'status':'STARTED','registered_classes':[],'events':[],'copied_origins_modified':False}
def save():OUT.write_text(json.dumps(r,indent=2,default=str)+'\n',encoding='utf8')
save()
try:
 assert 'Gate09_Unreal583_Animation_QA_20261019' in str(unreal.Paths.get_project_file_path())
 a=unreal.load_asset(ASSET);assert isinstance(a,unreal.AnimSequence)
 a.modify()  # Unreal 5.8 AnimationLibrary notify edits do not reliably dirty package
 events=[('G09_A1_Hit01_Diagonal_180ms',.18),
  ('G09_A1_Hit02_Return_390ms',.39),('G09_A1_DodgeCancel_520ms',.52),
  ('G09_A1_Hit03_Finisher_650ms',.65),
  ('G09_A1_ActionComplete',float(a.get_play_length())-.03)]
 if len(unreal.AnimationLibrary.get_animation_notify_events(a))!=0:
  raise RuntimeError('Require empty source sequence')
 for tr_name,seconds in events:
  bp_uri=DEST+'/AN_'+tr_name
  if not unreal.EditorAssetLibrary.does_asset_exist(bp_uri):
   factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.AnimNotify)
   b=unreal.AssetToolsHelpers.get_asset_tools().create_asset('AN_'+tr_name,DEST,unreal.Blueprint,factory)
   assert isinstance(b,unreal.Blueprint)
   assert unreal.EditorAssetLibrary.save_loaded_asset(b)
  cls=unreal.EditorAssetLibrary.load_blueprint_class(bp_uri)
  assert cls is not None
  r['registered_classes'].append({'uri':bp_uri,'generated_class':str(cls)})
  tr=unreal.Name(tr_name)
  if str(tr) not in {str(x) for x in unreal.AnimationLibrary.get_animation_notify_track_names(a)}:
   unreal.AnimationLibrary.add_animation_notify_track(a,tr)
  assert unreal.AnimationLibrary.is_valid_anim_notify_track_name(a,tr)
  obj=unreal.AnimationLibrary.add_animation_notify_event(a,tr,seconds,cls)
  native=unreal.AnimationLibrary.get_animation_notify_events_for_track(a,tr)
  if len(native)!=1:raise RuntimeError('Not persisted in model: '+str(tr)+', returned='+str(obj))
  actual=float(unreal.AnimationLibrary.get_anim_notify_event_trigger_time(native[0]))
  assert abs(actual-seconds)<.035
  r['events'].append({'track':tr_name,'expected_seconds':seconds,'actual_seconds':actual,'notify_class':str(cls)})
  save()
 assert len(unreal.AnimationLibrary.get_animation_notify_events(a))==5
 assert unreal.EditorAssetLibrary.save_loaded_asset(a, only_if_is_dirty=False)
 r['status']='PASS_FIVE_CONCRETE_BLUEPRINT_NOTIFY_FORCED_PERSIST_COLD_PENDING'
except Exception:
 r['status']='FAIL_NATIVE_CONCRETE_NOTIFY_V3'
 r['error']=traceback.format_exc()
finally:
 r['time_utc']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
 save();unreal.log('G09_A1_BP_NOTIFY_V2_'+r['status'])
