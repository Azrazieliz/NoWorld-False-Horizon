"""Gate09: native UE5.8 import of authentic Mixamo/Koikatsu sword source motions.
Three source motions only, NOT a finished A1 montage. No existing source replacement.
"""
import unreal,os,json,hashlib,traceback,time
ROOT=r"D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019"
DIR=r"D:\UnrealProjects\Gate09_Protected_Production_20261016\DharmaWeaponInterchange_v6_20261019"
OUT="/Game/Gate09_QA_20261019/DharmaWeaponSource"
SK="/Game/Experimental/LuminousValentineV3/CLOTHED/Parts/Mesh000Prim000/Mesh_00_Prim_00/SkeletalMeshes/Mesh_00_Prim_00_Skeleton.Mesh_00_Prim_00_Skeleton"
R=os.path.join(ROOT,"Saved","Gate09QA","unreal_06_dharma_native_sword_sources.json")
d={'status':'STARTED','actual_motion_authored':True,'full_signature_combo':False,'source':DIR,
   'character_source_mutated':False,'gate12_mutated':False,'assets':[]}
def save():
 os.makedirs(os.path.dirname(R),exist_ok=True)
 with open(R,'w',encoding='utf8') as f:json.dump(d,f,indent=2,default=str)
save()
try:
 assert "Gate09_Unreal583_Animation_QA_20261019" in str(unreal.Paths.get_project_file_path())
 manifest=json.load(open(os.path.join(DIR,'INTERCHANGE_MANIFEST.json'),encoding='utf8'))
 assert len(manifest['actions'])==3
 sk=unreal.load_asset(SK)
 assert isinstance(sk,unreal.Skeleton)
 expected_refs=set(str(n).casefold() for n in sk.get_reference_pose().get_bone_names())
 assert len(expected_refs)==975
 for rec in manifest['actions']:
  source=os.path.join(DIR,rec['fbx'])
  with open(source,'rb') as f:srcsha=hashlib.sha256(f.read()).hexdigest()
  assert srcsha==rec['fbx_sha256']
  dest=OUT+'/'+os.path.splitext(rec['fbx'])[0]
  if unreal.EditorAssetLibrary.does_asset_exist(dest):
   raise FileExistsError('Will not overwrite prior UE source combat action '+dest)
  task=unreal.AssetImportTask()
  task.filename=source;task.destination_path=OUT
  task.automated=True;task.save=True;task.replace_existing=False
  opts=unreal.FbxImportUI();opts.import_mesh=False
  opts.import_as_skeletal=True;opts.import_animations=True
  opts.automated_import_should_detect_type=False
  opts.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION
  opts.skeleton=sk;task.options=opts
  unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
  paths=list(task.imported_object_paths)
  if len(paths)!=1:raise RuntimeError('Expected one native Unreal asset for '+rec['fbx']+': '+str(paths))
  a=unreal.load_asset(paths[0])
  if not isinstance(a,unreal.AnimSequence):raise TypeError('UE imported asset not AnimSequence')
  m=a.get_editor_property('controller').get_model_interface()
  names=[str(x).casefold() for x in m.get_bone_track_names()]
  if len(names)!=975 or set(names)!=expected_refs:raise RuntimeError('Unreal 975 bone mapping check failed')
  row={'source_clip':rec['source_clip'],'role':rec['motion_role'],
       'native_asset':a.get_path_name(),'source_fbx_sha256':srcsha,
       'native_frame_count':int(m.get_number_of_frames()),
       'native_fps':str(m.get_frame_rate()),
       'seconds':float(a.get_play_length()),'skeleton_matches_975':True,
       'track_count':len(names),'unreal_motion_delta_qa':'NOT_YET'}
  d['assets'].append(row)
  save()
  unreal.log('GATE09_DHARMA_NATIVE_WEAPON_SOURCE_ACCEPTED='+row['native_asset'])
 d['status']='PASS_THREE_AUTHENTIC_SWORD_SOURCE_NATIVE_UNREAL_ASSETS'
except Exception:
 d['status']='FAIL_SOURCE_COMBAT_UNREAL_IMPORT'
 d['exception']=traceback.format_exc()
finally:
 d['finished_utc']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
 save()
 unreal.log('G09_DHARMA_IMPORT='+d['status'])
