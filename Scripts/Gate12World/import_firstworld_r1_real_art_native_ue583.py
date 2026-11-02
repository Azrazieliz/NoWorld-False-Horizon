"""Gate02 R1 actual author-created art intake, QA only. Never edit StartingWorld."""
import unreal,json,hashlib,traceback,time
from pathlib import Path
ROOT=Path('D:/UnrealProjects/OfflineGame_Gate12_Koikatsu_20261010')
SRC=ROOT/'Saved/Gate12Readiness/FirstWorldR1_RealSources'
OUTPUT=ROOT/'Saved/Gate12Readiness/Gate12_20261020_R1_NativeImport_Receipt.json'
DEST='/Game/Experimental/FirstWorldR1'
data={'status':'STARTED','timestamp':time.time(),'source_source_archive':'D:/NoWorld_Art_Production/NoWorld_FirstWorld_RegionKit_R1_20261019.zip','assets':[],'map':None,'errors':[]}
def save():
 OUTPUT.parent.mkdir(parents=True,exist_ok=True)
 OUTPUT.write_text(json.dumps(data,indent=2,default=str),encoding='utf8')
save()
try:
 assert 'OfflineGame_Gate12_Koikatsu_20261010' in unreal.Paths.get_project_file_path()
 # Cold-load all four original Native Gate09 assets and record exact
 # Skeleton package names before judging Base-CLOTHED compatibility.
 clips=[
   '/Game/Gate09_QA_20261019/TestSingle/G09_LuminousV3_Idle_01_Thoughtful',
   '/Game/Gate09_QA_20261019/NativeKoikatsu/G09_LuminousV3_Idle_02_Greeting',
   '/Game/Gate09_QA_20261019/NativeKoikatsu/G09_LuminousV3_Idle_03_Expressive',
   '/Game/Gate09_QA_20261019/NativeKoikatsu/G09_LuminousV3_Emote_Bow_Source',
 ]
 data['gate09_native_asset_cold_reload']=[]
 for path in clips:
  anim=unreal.EditorAssetLibrary.load_asset(path)
  if not anim or not isinstance(anim,unreal.AnimSequence):
   raise RuntimeError('Gate09 verified source asset did not load in Gate12: '+path)
  sk=anim.get_editor_property('skeleton')
  rec={'clip':path,'seconds':anim.get_play_length(),'skeleton':sk.get_path_name() if sk else None}
  data['gate09_native_asset_cold_reload'].append(rec)
  save()
 art=unreal.AssetToolsHelpers.get_asset_tools()
 for rel,target in [('anchors/regional_roads.fbx','Roads'),('modules/moss_rock.glb','MossRock'),('terrain/NW_RiverVale_CrownBasinRivenEscarpment_height_513_R16.png','HeightSource')]:
  file=SRC/rel
  assert file.is_file(),file
  folder=DEST+'/'+target
  if unreal.EditorAssetLibrary.does_directory_exist(folder) and unreal.EditorAssetLibrary.list_assets(folder,recursive=True,include_folder=False):
   raise RuntimeError('Protected previous import already exists: '+folder)
  task=unreal.AssetImportTask()
  task.set_editor_property('filename',str(file))
  task.set_editor_property('destination_path',folder)
  task.set_editor_property('automated',True)
  task.set_editor_property('save',True)
  task.set_editor_property('replace_existing',False)
  task.set_editor_property('async_',False)
  art.import_asset_tasks([task])
  paths=list(unreal.EditorAssetLibrary.list_assets(folder,recursive=True,include_folder=False))
  loaded=[]
  for package in paths:
   obj=unreal.EditorAssetLibrary.load_asset(package)
   if obj: loaded.append({'package':str(package),'type':obj.get_class().get_name()})
  data['assets'].append({'source':rel,'source_sha256':hashlib.sha256(file.read_bytes()).hexdigest(),'native_objects':loaded})
  save()
  expected_type='Texture2D' if rel.endswith('.png') else 'StaticMesh'
  if not any(item['type']==expected_type for item in loaded):
   raise RuntimeError('No genuine '+expected_type+' produced from '+rel)
 if not unreal.EditorLevelLibrary.new_level(DEST+'/Maps/L_FirstWorld_R1_REAL_ART_QA'):
  raise RuntimeError('UE QA map creation failed')
 data['map']=DEST+'/Maps/L_FirstWorld_R1_REAL_ART_QA'
 tile=json.loads((SRC/'procedural/seed1412_tile_rock_verified.json').read_text(encoding='utf8'))
 assert tile['source_seed']==1412 and tile['tile_id']=='T_12_11'
 planted=0
 for imported in data['assets']:
  objs=[unreal.EditorAssetLibrary.load_asset(x['package']) for x in imported['native_objects'] if x['type']=='StaticMesh']
  if not objs:continue
  obj=objs[0]
  if 'roads' in imported['source']:
   # FBX world-position vertices: identity actor transform, no doubled position.
   actor=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,0))
   if not actor:raise RuntimeError('R1 actual FBX regional road actor creation failed')
   actor.get_editor_property('static_mesh_component').set_static_mesh(obj)
   actor.set_actor_label('R1_SourceRegionalRoads_UnchangedWorldOrigin')
  else:
   for placement in tile['placements']:
    if placement['prototype']!='modules/moss_rock.glb':continue
    xyz=placement['position_m']
    actor=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor,
     unreal.Vector(xyz[0]*100,xyz[1]*100,xyz[2]*100),
     unreal.Rotator(0,placement['yaw_degrees'],0))
    if not actor:raise RuntimeError('R1 deterministic real moss-rock actor missing')
    actor.get_editor_property('static_mesh_component').set_static_mesh(obj)
    sc=float(placement['uniform_scale'])
    actor.set_actor_scale3d(unreal.Vector(sc,sc,sc))
    actor.set_actor_label(placement['persistent_id'])
    planted+=1
 data['seed1412_tile_id']='T_12_11'
 data['real_source_rock_actors_with_persistent_ids']=planted
 if planted<1:raise RuntimeError('No deterministic Gate02 moss rocks instantiated')
 if not unreal.EditorLevelLibrary.save_current_level():
  raise RuntimeError('UE QA scene save failed')
 data['status']='NATIVE_IMPORT_AND_REAL_QA_LEVEL_SAVED_NOT_ART_ACCEPTED'
except Exception as exc:
 data['status']='NATIVE_IMPORT_FAILED_OR_PARTIAL'
 data['errors'].append(repr(exc))
 data['errors'].append(traceback.format_exc()[-6500:])
finally:
 save()
 unreal.log('GATE12_FIRSTWORLD_R1_NATIVE_IMPORT='+data['status'])
