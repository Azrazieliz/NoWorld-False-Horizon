"""Gate09 physical, sword-local silver/crimson reusable FX geometry to UE5.8.
Material/native GLB actual source, no placeholder Niagara.
"""
import unreal,os,pathlib,json,hashlib,time,traceback
ROOT=pathlib.Path(r'D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019')
src=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016\Luminous_A1_ScarletCadence_ActualSwordSweptFX_Unreal_v10.glb')
dest='/Game/Gate09_QA_20261019/LuminousA1SweptFX_v10'
out=ROOT/'Saved/Gate09QA/unreal_24_a1_v10_physical_blade_trails.json'
r={'status':'STARTED','static_mesh_fx':[],'materials':[],
 'source_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'runtime_binding':False}
try:
 assert 'Gate09_Unreal583_Animation_QA_20261019' in str(unreal.Paths.get_project_file_path())
 if unreal.EditorAssetLibrary.does_directory_exist(dest):raise FileExistsError(dest)
 t=unreal.AssetImportTask();t.filename=str(src);t.destination_path=dest
 t.automated=True;t.save=True;t.replace_existing=False
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
 for item in t.imported_object_paths:
  asset=unreal.load_asset(item)
  if isinstance(asset,unreal.StaticMesh):
   r['static_mesh_fx'].append(asset.get_path_name())
  elif isinstance(asset,unreal.MaterialInterface):
   r['materials'].append(asset.get_path_name())
 if len(r['static_mesh_fx'])!=9 or len(r['materials'])!=3:
  raise RuntimeError('Expected exactly 9 genuine swept VFX meshes and 3 mats, got '+str(r))
 r['status']='PASS_NINE_NATIVE_REAL_MOTION_SWEEP_MESHES_THREE_MATERIALS'
except Exception:
 r['status']='FAIL_NATIVE_GLB_BLADEVFX'
 r['error']=traceback.format_exc()
finally:
 r['utc']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
 out.write_text(json.dumps(r,indent=2)+'\n',encoding='utf8')
 unreal.log('G09_V7_BLADEVFX_'+r['status'])
