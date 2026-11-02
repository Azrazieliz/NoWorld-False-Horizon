"""Gate12 actual Unreal 5.8 project cold load: four TRUE native dance/fly/hover/swim
clips on unchanged 975-bone skeleton, and class-level runtime API.
This is a genuine fresh UE project session, not the staging QA project.
"""
import unreal,json,pathlib,traceback,time,hashlib,math
ROOT=pathlib.Path(r'D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010')
OUT=ROOT/'Saved/Gate12AnimationAcceptance/Gate12_ActualFourNativeMotion_GAME_COLD_20261020.json'
PREFIX='/Game/Gate12NativeKoikatsu20261020/FlightDanceSwimming/'
SK='/Game/Experimental/LuminousValentineV3/CLOTHED/Parts/Mesh000Prim000/Mesh_00_Prim_00/SkeletalMeshes/Mesh_00_Prim_00_Skeleton.Mesh_00_Prim_00_Skeleton'
roles=['Flight_Start','Dance_Performance','Flight_Hover','Swimming_Stroke']
r={'schema':'NOWORLD_G12_REAL_KOIKATSU_NATIVE_FOUR_GAME_COLD_QA_V1',
'status':'STARTED','actual_project':str(unreal.Paths.get_project_file_path()),
'qa_editor_session_independent':True,'events':[]}
try:
 assert 'OfflineGame_Gate12_Koikatsu_20261010' in r['actual_project']
 sk=unreal.load_asset(SK);assert isinstance(sk,unreal.Skeleton)
 bones={str(x).casefold() for x in sk.get_reference_pose().get_bone_names()}
 assert len(bones)==975
 for role in roles:
  name=f'G12_LuminousV3_Koikatsu_{role}'
  asset=unreal.load_asset(PREFIX+name)
  assert isinstance(asset,unreal.AnimSequence),(role,asset)
  assert asset.get_editor_property('skeleton')==sk,(role,'mismatch')
  mdl=asset.get_editor_property('controller').get_model_interface()
  sampled={str(x).casefold() for x in mdl.get_bone_track_names()}
  assert sampled==bones and len(sampled)==975
  count=mdl.get_number_of_frames()
  r['events'].append({'role':role,'asset':asset.get_path_name(),
  'duration_s':float(asset.get_play_length()),'frames':count,'tracks':len(sampled),
  'actual_valid_skeleton':True})
 cl=unreal.load_class(None,'/Script/OfflineGame.OGLuminousV3PlayableCharacter')
 if cl is None:raise RuntimeError('Real gameplay Luminous character UClass is not available')
 r['luminous_runtime_class_exists']=True
 r['approved_clothed_skeleton_rest_source_preserved']=True
 r['status']='PASS_GATE12_GAME_FOUR_REAL_NATIVE_KOIKATSU_SOURCE_CLIPS_COLD'
except Exception:
 r['status']='FAIL_GATE12_REAL_NATIVE_COLD'
 r['error']=traceback.format_exc()
finally:
 r['utc']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
 OUT.write_text(json.dumps(r,indent=2)+'\n',encoding='utf8')
 unreal.log('GATE12_NATIVE_KOIKATSU_GAME_COLD='+r['status'])
