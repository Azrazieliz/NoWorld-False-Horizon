"""Isolated REAL Unreal 5.8.3 full-body Luminous dance acceptance map.
Reuse 45 existing Gate12 CLOTHED native USkeletalMesh shards and original
7.3-second authored Koikatsu action, with ONE animation leader + 44 followers.
Never modify imported/approved character assets, runtime C++, World or Registry.
Requires the previous 45-part inventory. Source-only QA stage.
"""
import unreal,pathlib,json,traceback,time
ROOT=pathlib.Path(r'D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010')
REG=ROOT/'Saved/Gate12Koikatsu/LuminousV3/CLOTHED/luminous_playable_45part_native_blueprint_ue583.json'
MAP='/Game/Gate12NativeKoikatsu20261020/VisualQA/L_LuminousV3_Dance_Full45_UE583_VisualQA'
CLIP='/Game/Gate12NativeKoikatsu20261020/FlightDanceSwimming/G12_LuminousV3_Koikatsu_Dance_Performance.G12_LuminousV3_Koikatsu_Dance_Performance'
OUT=ROOT/'Saved/Gate12AnimationAcceptance/Gate12_Dance_Full45_UnrealVisualQA_Authoring_20261020.json'
status={'status':'STARTED','source_character':'Luminous Valentine V3 CLOTHED',
 'actual_game_project':str(unreal.Paths.get_project_file_path()),'map':MAP,
 'animation_asset':CLIP,'actors':[],'original_canonical_assets_modified':False}
def save():OUT.write_text(json.dumps(status,indent=2,default=str)+'\n',encoding='utf8')
save()
try:
 assert 'OfflineGame_Gate12_Koikatsu_20261010' in str(unreal.Paths.get_project_file_path())
 # The immediately prior attempt created an empty isolated map before a
 # camera-helper API failure. Reopen only that new map; never edit another level.
 if unreal.EditorAssetLibrary.does_asset_exist(MAP):
  pass
 manifest=json.loads(REG.read_text(encoding='utf8'))
 parts=manifest['parts']
 assert len(parts)==45 and sorted(int(p['index']) for p in parts)==list(range(45))
 source=unreal.load_asset(CLIP)
 assert isinstance(source,unreal.AnimSequence)
 assert abs(source.get_play_length()-7.3)<.08
 # Real native 975-track sequence, not a made-up keyframed actor.
 tracks=source.get_editor_property('controller').get_model_interface().get_bone_track_names()
 assert len(tracks)==975
 if unreal.EditorAssetLibrary.does_asset_exist(MAP):
   world=unreal.EditorLoadingAndSavingUtils.load_map(MAP)
   existing=[a for a in unreal.EditorLevelLibrary.get_all_level_actors() if a.get_actor_label().startswith('G12_Luminous_Dance_CLOTHED_')]
   assert not existing,'Refusing duplicate Luminous in QA map'
 else:
   assert unreal.EditorLevelLibrary.new_level(MAP),'Unable to create isolated QA level'
 world=unreal.EditorLevelLibrary.get_editor_world()
 assert world
 leader=None
 for item in sorted(parts,key=lambda p:int(p['index'])):
  idx=int(item['index'])
  sk=unreal.load_asset(item['path'])
  assert isinstance(sk,unreal.SkeletalMesh),(idx,item['path'])
  assert len(sk.get_editor_property('skeleton').get_reference_pose().get_bone_names())==975
  actor=unreal.EditorLevelLibrary.spawn_actor_from_class(
    unreal.SkeletalMeshActor,unreal.Vector(0,0,-96),unreal.Rotator(0,-90,0))
  assert actor is not None
  actor.set_actor_label(f'G12_Luminous_Dance_CLOTHED_{idx:02d}')
  comp=actor.get_component_by_class(unreal.SkeletalMeshComponent)
  assert comp is not None
  comp.set_skeletal_mesh_asset(sk)
  comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
  comp.set_editor_property('cast_shadow',True)
  if leader is None:
   if sk.get_editor_property('skeleton')!=source.get_editor_property('skeleton'):
    raise RuntimeError('First part skeleton must be identical to native dance author skeleton')
   comp.override_animation_data(source,is_looping=True,is_playing=True,position=0,play_rate=1)
   leader=comp
  else:
   # One evaluated animation, all 975 bones shared by exact source rig.
   comp.set_leader_pose_component(leader,force_update=True,follower_should_tick_pose=False)
  status['actors'].append({'index':idx,'label':actor.get_actor_label(),
    'source_mesh':sk.get_path_name(),'source_skeleton':sk.get_editor_property('skeleton').get_path_name(),
    'is_leader':idx==0})
  if idx%9==8:
   print('GATE12_FULL_BODY_DANCE_SCENE_ACTORS',idx+1,'/45',flush=True);save()
 assert len(status['actors'])==45
 # Neutral editor-only presentation aids, no procedural world replacement.
 sun=unreal.EditorLevelLibrary.spawn_actor_from_class(
   unreal.DirectionalLight,unreal.Vector(0,0,350),unreal.Rotator(-40,-35,0))
 if sun:sun.set_actor_label('VisualQA_StudioSun')
 sky=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,280))
 if sky:sky.set_actor_label('VisualQA_StudioFill')
 camera=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.CameraActor,
   unreal.Vector(350,310,145))
 if camera:
  camera.set_actor_label('VisualQA_Dance_Full45_Camera')
  look=unreal.Rotator(-8,-138,0) # Authentic full-body animation QA camera

  camera.set_actor_rotation(look,False)
 assert unreal.EditorLoadingAndSavingUtils.save_map(world,MAP),'Failed map persistence'
 status['camera_qa_only']=camera.get_path_name() if camera else None
 status['actual_character_parts']=45
 status['native_975_animation_tracks']=len(tracks)
 status['serialized_override_animation_data_leader']=True
 status['followers_set_leader_pose_component']=44
 status['status']='CREATED_FULL45_TRUE_UNREAL_ANIMATED_DANCE_MAP_PENDING_COLD_AND_VISUAL_CHECK'
 print('GATE12_REAL_UNREAL_FULL_CHARACTER_DANCE_QA_MAP_SAVED',MAP,flush=True)
except Exception:
 status['status']='FAIL_NO_PRETEND_FULLBODY'
 status['exception']=traceback.format_exc()[-9000:]
 print(status['exception'],flush=True)
finally:
 status['created_utc']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
 save()
