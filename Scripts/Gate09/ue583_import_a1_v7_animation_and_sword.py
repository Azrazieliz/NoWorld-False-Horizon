"""Real Gate09 A1 v7 Blender5 NLA+bone-parent sword sources -> UE5.8 native,
five individually named hit/cancel/completion notify tracks; sword static meshes
imported separately as explicit non-canonical visual model, never phantom equipped.
"""
import unreal,os,json,hashlib,traceback,time
ROOT=r'D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019'
SRC=r'D:\UnrealProjects\Gate09_Protected_Production_20261016\A1_ConstraintDriven_UnrealExports_v7\G09_LuminousV3_A1_ScarletCadence_RealSword_NLA_v7.fbx'
SWORD=r'D:\UnrealProjects\Gate09_Protected_Production_20261016\G09_GothicSword_VisualFitting_UE_Optimized_v2.glb'
SK='/Game/Experimental/LuminousValentineV3/CLOTHED/Parts/Mesh000Prim000/Mesh_00_Prim_00/SkeletalMeshes/Mesh_00_Prim_00_Skeleton.Mesh_00_Prim_00_Skeleton'
ANIM='/Game/Gate09_QA_20261019/LuminousA1NLA_v7'
SWORD_DEST='/Game/Gate09_QA_20261019/LuminousSwordVisualSource_v7'
REC=os.path.join(ROOT,'Saved/Gate09QA/unreal_12_a1_v7_native_import_notify_weapon.json')
r={'status':'STARTED','hit_timing_s':[.18,.39,.65],'dodge_cancel_s':.52,
 'source_visual_weapon_canonical_gate07':False,'gameplay_hit_collision_connected':False,
 'source_character_mutated':False,'gate12_project_mutated':False,
 'notify_tracks':[],'sword_native_meshes':[]}
def save():
 with open(REC,'w',encoding='utf8') as f:json.dump(r,f,indent=2,default=str)
save()
try:
 assert 'Gate09_Unreal583_Animation_QA_20261019' in str(unreal.Paths.get_project_file_path())
 def sha(path):
  h=hashlib.sha256()
  with open(path,'rb') as f:
   for b in iter(lambda:f.read(1048576),b''):h.update(b)
  return h.hexdigest()
 r['fbx_sha256']=sha(SRC);r['sword_source_glb_sha256']=sha(SWORD)
 sk=unreal.load_asset(SK)
 assert isinstance(sk,unreal.Skeleton)
 assert len(sk.get_reference_pose().get_bone_names())==975
 source_name='G09_LuminousV3_A1_ScarletCadence_RealSword_NLA_v7'
 if unreal.EditorAssetLibrary.does_asset_exist(ANIM+'/'+source_name):
  raise FileExistsError('V7 source must not overwrite')
 task=unreal.AssetImportTask();task.filename=SRC
 task.destination_path=ANIM;task.automated=True;task.save=True;task.replace_existing=False
 opts=unreal.FbxImportUI();opts.import_mesh=False;opts.import_as_skeletal=True;opts.import_animations=True
 opts.automated_import_should_detect_type=False
 opts.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION
 opts.skeleton=sk;task.options=opts
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
 paths=list(task.imported_object_paths)
 assert len(paths)==1,paths
 a=unreal.load_asset(paths[0]);assert isinstance(a,unreal.AnimSequence)
 mdl=a.get_editor_property('controller').get_model_interface()
 tracks={str(x).casefold() for x in mdl.get_bone_track_names()}
 refs={str(x).casefold() for x in sk.get_reference_pose().get_bone_names()}
 assert len(tracks)==len(refs)==975 and tracks==refs
 r['actual_native_asset']=a.get_path_name()
 r['native_track_count']=len(tracks);r['frame_count']=mdl.get_number_of_frames()
 r['duration_s']=a.get_play_length()
 events=[('Hit01_Diagonal_180ms',.18),('Hit02_Return_390ms',.39),
         ('DodgeCancel_520ms',.52),('Hit03_Finisher_650ms',.65),
         ('ActionComplete',float(a.get_play_length())-.03)]
 for name,t in events:
  track=unreal.Name('G09_A1_'+name)
  unreal.AnimationLibrary.add_animation_notify_track(a,track)
  n=unreal.AnimationLibrary.add_animation_notify_event(a,track,t,unreal.AnimNotify)
  assert n is not None
  instances=unreal.AnimationLibrary.get_animation_notify_events_for_track(a,track)
  assert len(instances)==1,(name,len(instances))
  actual=unreal.AnimationLibrary.get_anim_notify_event_trigger_time(instances[0])
  assert abs(float(actual)-t)<.035,(name,actual,t)
  r['notify_tracks'].append({'name':str(track),'request_s':t,'native_actual_s':float(actual)})
 assert len(unreal.AnimationLibrary.get_animation_notify_events(a))==5
 assert unreal.EditorAssetLibrary.save_loaded_asset(a)
 r['saved_native_notify_count']=5
 save()
 if unreal.EditorAssetLibrary.does_directory_exist(SWORD_DEST):
  raise FileExistsError('sword lookdev dest exists, avoid overwrite')
 sword_task=unreal.AssetImportTask()
 sword_task.filename=SWORD;sword_task.destination_path=SWORD_DEST
 sword_task.automated=True;sword_task.save=True;sword_task.replace_existing=False
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([sword_task])
 for name in sword_task.imported_object_paths:
  obj=unreal.load_asset(name)
  if isinstance(obj,unreal.StaticMesh):
   r['sword_native_meshes'].append(obj.get_path_name())
 if len(r['sword_native_meshes'])<3:raise RuntimeError('gothic sword native StaticMesh geometry failed to import')
 r['status']='PASS_A1_NATIVE_SKELETON_NOTIFIES_AND_SWORD_MESHES'
except Exception:
 r['status']='FAIL_A1_SOURCE_NATIVE_IMPORT'
 r['error']=traceback.format_exc()
finally:
 r['finished_utc']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
 save();unreal.log('G09_A1_V7_NATIVE='+r['status'])
