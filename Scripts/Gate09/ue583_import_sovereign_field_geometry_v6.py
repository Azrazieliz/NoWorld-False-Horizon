"""Gate09 isolated UE 5.8 native import of genuine sculpted 16m Sovereign Eclipse geometry.
Does NOT claim gameplay field/negation runtime or Gate00 visual acceptance.
"""
import unreal,os,json,hashlib,traceback,time
ROOT=r'D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019'
SRC=r'D:\UnrealProjects\Gate09_Protected_Production_20261016\G09_SovereignEclipse_Architectural_UE_GROUPED_v6.glb'
DEST='/Game/Gate09_QA_20261019/SovereignEclipseGeometry'
FILE=os.path.join(ROOT,'Saved/Gate09QA/unreal_08_sovereign_geometry_import.json')
d={'status':'STARTED','source':SRC,'world_radius_m':16,
 'native_effect_assets':[],'source_character_modified':False,'gameplay_implemented':False}
def save():
 with open(FILE,'w',encoding='utf8') as f:json.dump(d,f,indent=2,default=str)
save()
try:
 assert 'Gate09_Unreal583_Animation_QA_20261019' in str(unreal.Paths.get_project_file_path())
 with open(SRC,'rb') as f:sh=hashlib.sha256(f.read()).hexdigest()
 d['source_sha256']=sh
 if unreal.EditorAssetLibrary.does_directory_exist(DEST):
  raise FileExistsError('Never overwrite a preexisting 16m field asset folder')
 task=unreal.AssetImportTask();task.filename=SRC
 task.destination_path=DEST;task.automated=True;task.save=True
 task.replace_existing=False
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
 paths=[str(p) for p in task.imported_object_paths]
 d['imported_asset_paths']=paths
 for path in paths:
  obj=unreal.load_asset(path)
  if isinstance(obj,unreal.StaticMesh):
   d['native_effect_assets'].append({'path':obj.get_path_name(),'triangles_unverified':True,
     'native_type':'StaticMesh'})
 if len(d['native_effect_assets'])<4:
  raise RuntimeError('Expected at least 4 native Unreal static meshes from 4 grouped source meshes, got '+str(len(d['native_effect_assets'])))
 d['status']='PASS_SOURCE_NATIVE_UE_FIELD_GEOMETRY_IMPORT_ONLY'
except Exception:
 d['status']='FAIL_SOVEREIGN_GEOMETRY_UNREAL_IMPORT'
 d['exception']=traceback.format_exc()
finally:
 d['finished_utc']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
 save();unreal.log('G09_SOVEREIGN_UE_IMPORT='+d['status'])
