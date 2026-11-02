import unreal,pathlib,json,traceback,time
ROOT=pathlib.Path(r'D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010')
MAP='/Game/Gate12NativeKoikatsu20261020/VisualQA/L_LuminousV3_Dance_Full45_UE583_VisualQA'
CLIP='/Game/Gate12NativeKoikatsu20261020/FlightDanceSwimming/G12_LuminousV3_Koikatsu_Dance_Performance.G12_LuminousV3_Koikatsu_Dance_Performance'
OUT=ROOT/'Saved/Gate12AnimationAcceptance/Gate12_FULL45_DANCE_COLD_PERSISTENCE_20261020.json'
r={'status':'STARTED','map':MAP,'actors':[]}
try:
 assert 'OfflineGame_Gate12_Koikatsu_20261010' in str(unreal.Paths.get_project_file_path())
 world=unreal.EditorLoadingAndSavingUtils.load_map(MAP)
 assert world
 actors=[a for a in unreal.EditorLevelLibrary.get_all_level_actors() if a.get_actor_label().startswith('G12_Luminous_Dance_CLOTHED_')]
 assert len(actors)==45,len(actors)
 ids=set()
 anim=unreal.load_asset(CLIP);assert isinstance(anim,unreal.AnimSequence)
 skeleton=anim.get_editor_property('skeleton')
 for a in actors:
  comp=a.get_component_by_class(unreal.SkeletalMeshComponent)
  assert comp
  mesh=comp.get_skeletal_mesh_asset()
  assert mesh and len(mesh.get_editor_property('skeleton').get_reference_pose().get_bone_names())==975
  idx=int(a.get_actor_label().rsplit('_',1)[1]);ids.add(idx)
  data={'index':idx,'name':a.get_actor_label(),'skeletal_mesh':mesh.get_path_name(),
        'skeletal_skeleton':mesh.get_editor_property('skeleton').get_path_name()}
  try:
   link=comp.get_editor_property('leader_pose_component')
   data['leader_pose_component']=str(link.get_path_name()) if link else None
  except Exception as err:
   data['leader_pose_getter_unavailable']=str(err)[:200]
  try:
   ad=comp.get_editor_property('animation_data')
   aclip=ad.get_editor_property('anim_to_play')
   data['serialized_animation']=aclip.get_path_name() if aclip else None
   data['is_playing']=bool(ad.get_editor_property('is_playing'))
   data['is_looping']=bool(ad.get_editor_property('is_looping'))
  except Exception as err:
   data['animation_data_error']=str(err)[:200]
  r['actors'].append(data)
 assert ids==set(range(45))
 leader=r['actors'][0]
 r['rendered_character_parts_loaded']=len(actors)
 r['serialized_native_anim_count']=sum(bool(x.get('serialized_animation')) for x in r['actors'])
 r['pose_link_count']=sum(bool(x.get('leader_pose_component')) for x in r['actors'])
 r['native_animation_asset']=CLIP
 if r['serialized_native_anim_count']!=1:raise RuntimeError('Leader motion asset not persisted exactly once '+str(r['serialized_native_anim_count']))
 if r['pose_link_count']!=44:raise RuntimeError('Follower pose links did not persist '+str(r['pose_link_count']))
 r['status']='PASS_COLD_45_NATIVE_MESHES_SINGLE_ANIMATION_AND_44_FOLLOWERS'
 print('GATE12_UNREAL_45_FULL_CHARACTER_DANCE_COLD_PERSISTED',r['pose_link_count'],flush=True)
except Exception:
 r['status']='FAIL_COLD_FULL_CHARACTER_WIRING'
 r['error']=traceback.format_exc()
 print(r['error'],flush=True)
finally:
 OUT.write_text(json.dumps(r,indent=2,default=str)+'\n',encoding='utf8')
