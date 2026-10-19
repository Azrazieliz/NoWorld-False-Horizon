"""Gate09 independent second UE editor session validates actual field mesh/material reload."""
import unreal,json,pathlib,traceback,time
q=pathlib.Path(r'D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019\Saved\Gate09QA')
orig=json.loads((q/'unreal_08_sovereign_geometry_import.json').read_text(encoding='utf8'))
out=q/'unreal_09_sovereign_geometry_cold_qa.json'
r={'status':'STARTED','native_meshes':[],'materials':[],'read_only':True}
try:
 assert 'Gate09_Unreal583_Animation_QA_20261019' in str(unreal.Paths.get_project_file_path())
 assert orig['status']=='PASS_SOURCE_NATIVE_UE_FIELD_GEOMETRY_IMPORT_ONLY'
 for record in orig['native_effect_assets']:
  obj=unreal.load_asset(record['path'])
  assert isinstance(obj,unreal.StaticMesh)
  row={'asset':obj.get_path_name(),'lod_count':obj.get_num_lods()}
  try:
   b=obj.get_bounds();e=b.box_extent
   row['box_extent_cm']=[float(e.x),float(e.y),float(e.z)]
   row['radius_cm']=float(b.sphere_radius)
  except Exception as ex:row['bounds_error']=str(ex)
  assert row['lod_count']>=1
  r['native_meshes'].append(row)
 for name in orig['imported_asset_paths']:
  if '/Materials/' not in name:continue
  o=unreal.load_asset(name)
  assert isinstance(o,unreal.MaterialInterface)
  r['materials'].append(o.get_path_name())
 assert len(r['native_meshes'])==4 and len(r['materials'])==8
 r['status']='PASS_NATIVE_SOVEREIGN_GEOMETRY_COLD_RELOAD'
except Exception:
 r['status']='FAIL_GEOMETRY_COLD'
 r['exception']=traceback.format_exc()
finally:
 r['time_utc']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
 out.write_text(json.dumps(r,indent=2,default=str)+'\n',encoding='utf8')
 unreal.log('G09_SOVEREIGN_GEOMETRY_COLD_'+r['status'])
