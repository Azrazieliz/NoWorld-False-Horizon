import unreal,json,pathlib,traceback,time
Q=pathlib.Path(r'D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019\Saved\Gate09QA')
p=json.loads((Q/'unreal_24_a1_v10_physical_blade_trails.json').read_text(encoding='utf8'))
out=Q/'unreal_25_a1_v10_final_mesh_cold.json'
r={'status':'STARTED','meshes':[],'materials':[],'read_only':True}
try:
 assert 'Gate09_Unreal583_Animation_QA_20261019' in str(unreal.Paths.get_project_file_path())
 assert p['status']=='PASS_NINE_NATIVE_REAL_MOTION_SWEEP_MESHES_THREE_MATERIALS'
 for s in p['static_mesh_fx']:
  o=unreal.load_asset(s)
  assert isinstance(o,unreal.StaticMesh)
  r['meshes'].append({'path':o.get_path_name(),'lods':o.get_num_lods()})
 for s in p['materials']:
  o=unreal.load_asset(s)
  assert isinstance(o,unreal.MaterialInterface)
  r['materials'].append(o.get_path_name())
 assert len(r['meshes'])==9 and len(r['materials'])==3
 r['status']='PASS_UE5_V10_PHYSICAL_FX_NINE_MESHES_THREE_MATERIALS_COLD'
except Exception:
 r['status']='FAIL_SWORD_FX_COLD'
 r['exception']=traceback.format_exc()
finally:
 r['date_utc']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
 out.write_text(json.dumps(r,indent=2)+'\n',encoding='utf8')
 unreal.log('GATE09_A1_SWORD_FX_COLD='+r['status'])
