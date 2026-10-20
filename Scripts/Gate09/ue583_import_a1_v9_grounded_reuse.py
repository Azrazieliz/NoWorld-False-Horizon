"""Gate09: import physically grounded source v9 once to UE5.8 and REUSE
5 already verified concrete notifies, sword group meshes and reusable blade FX.
No redundant duplicate source assets or prior validation cycles.
"""
import unreal,json,pathlib,hashlib,time,traceback,math
P=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
ROOT=pathlib.Path(r'D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019')
Q=ROOT/'Saved/Gate09QA'
SRC=P/'A1_ActualGrounded_UnrealExports_v9/G09_LuminousV3_A1_ScarletCadence_RealSword_Grounded_v9.fbx'
DEST='/Game/Gate09_QA_20261019/LuminousA1Grounded_v9'
SEQ='G09_LuminousV3_A1_ScarletCadence_RealSword_Grounded_v9'
SKELETON='/Game/Experimental/LuminousValentineV3/CLOTHED/Parts/Mesh000Prim000/Mesh_00_Prim_00/SkeletalMeshes/Mesh_00_Prim_00_Skeleton'
OUT=Q/'unreal_21_a1_v9_grounded_original_native.json'
r={'status':'STARTED','new_source_animation':True,'source_fbx_sha256':hashlib.sha256(SRC.read_bytes()).hexdigest(),
'original_project_untouched':True,'native_track_count':None,'source_sword_meshes_reused':True,
'original_animations_unmodified':True,'created_notify_assets':False,'notify_events':[]}
def save():OUT.write_text(json.dumps(r,indent=2,default=str)+'\n',encoding='utf8')
save()
try:
 assert 'Gate09_Unreal583_Animation_QA_20261019' in str(unreal.Paths.get_project_file_path())
 if unreal.EditorAssetLibrary.does_asset_exist(DEST+'/'+SEQ):raise FileExistsError('V9 native source already exists')
 sk=unreal.load_asset(SKELETON);assert isinstance(sk,unreal.Skeleton)
 task=unreal.AssetImportTask();task.filename=str(SRC);task.destination_path=DEST
 task.automated=True;task.save=True;task.replace_existing=False
 opt=unreal.FbxImportUI();opt.import_mesh=False;opt.import_as_skeletal=True
 opt.import_animations=True;opt.automated_import_should_detect_type=False
 opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION;opt.skeleton=sk
 task.options=opt
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
 paths=list(task.imported_object_paths)
 assert len(paths)==1,paths
 seq=unreal.load_asset(paths[0]);assert isinstance(seq,unreal.AnimSequence)
 model=seq.get_editor_property('controller').get_model_interface()
 ref={str(x).casefold() for x in sk.get_reference_pose().get_bone_names()}
 bones={str(x).casefold() for x in model.get_bone_track_names()}
 assert len(ref)==len(bones)==975 and bones==ref
 r['native_asset']=seq.get_path_name()
 r['native_track_count']=len(bones)
 r['native_frames']=model.get_number_of_frames()
 r['native_duration_s']=float(seq.get_play_length())
 # Real bone baked *translation* (not cosmetic Blender armature world move).
 translations=[]
 for frame in [0,5,11,19,31]:
  pose=seq.get_anim_pose_at_frame(frame,unreal.AnimPoseEvaluationOptions())
  t=pose.get_bone_pose(unreal.Name('Center'),unreal.AnimPoseSpaces.LOCAL).translation
  translations.append({'frame':frame,'cm':[float(t.x),float(t.y),float(t.z)]})
 r['center_root_motion_samples']=translations
 origin=translations[0]['cm']
 r['max_root_motion_cm']=max(math.sqrt(sum((v['cm'][i]-origin[i])**2 for i in range(3))) for v in translations)
 if r['max_root_motion_cm']<15:
  raise RuntimeError('Native Center animated foot-root movement lost on export! '+str(translations))
 r['root_motion_baked_on_existing_skeleton']=True
 # Reuse already authored concrete class notifies, only source sequence new.
 desc=[('G09_A1_Hit01_Diagonal_180ms',.18),('G09_A1_Hit02_Return_390ms',.39),
 ('G09_A1_DodgeCancel_520ms',.52),('G09_A1_Hit03_Finisher_650ms',.65),
 ('G09_A1_ActionComplete',float(seq.get_play_length())-.03)]
 seq.modify()
 for track,at in desc:
  name=unreal.Name(track)
  cls=unreal.EditorAssetLibrary.load_blueprint_class('/Game/Gate09_QA_20261019/LuminousA1NotifyClasses_v7/AN_'+track)
  if cls is None:raise RuntimeError('Verified original concrete class missing '+track)
  unreal.AnimationLibrary.add_animation_notify_track(seq,name)
  evt=unreal.AnimationLibrary.add_animation_notify_event(seq,name,at,cls)
  assert evt is not None
  found=unreal.AnimationLibrary.get_animation_notify_events_for_track(seq,name)
  assert len(found)==1
  r['notify_events'].append({'name':track,'seconds':at,'persistent_class_uri':str(cls)})
 assert len(unreal.AnimationLibrary.get_animation_notify_events(seq))==5
 assert unreal.EditorAssetLibrary.save_loaded_asset(seq,only_if_is_dirty=False)
 r['status']='PASS_975_TRACKS_GROUNDED_ROOT_AND_FIVE_NATIVE_EVENTS_SAVE_COLD_PENDING'
except Exception:
 r['status']='FAIL_V9_ROOT_IMPORT'
 r['error']=traceback.format_exc()
finally:
 r['utc']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
 save();unreal.log('G09_V9_ROOT_IMPORT='+r['status'])
