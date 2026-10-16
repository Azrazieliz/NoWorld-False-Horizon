"""Compare exact *effective* source Blender shape keys to GLB sparse decoded vectors.
Match mesh identity and names, not inflated primitive/placeholder counts.
"""
import pathlib,json,collections,statistics,math
OUT=pathlib.Path(r'D:\UnrealProjects\NW_SoftwareMaster_P0_20261016')
base_src=json.loads((OUT/'P0_BLEND_SOURCE_EFFECTIVE_MORPHS.json').read_text())
glb=json.loads((OUT/'P0_GLB_EFFECTIVE_SPARSE_MORPH_DELTAS.json').read_text())
mesh_map={'Body バレンタインLuminous':'model','Gag Eyes バレンタインLuminous':'model.006',
          'Hair Outfit 00 バレンタインLuminous':'model.003','Outfit 00 バレンタインLuminous':'model.001',
          'Tears バレンタインLuminous':'model.005','Tongue (rigged) バレンタインLuminous':'model.002'}
results={}
for state in ('CLOTHED','UNDERWEAR','NUDE'):
 src=base_src if state=='CLOTHED' else json.loads((OUT/('P0_BLEND_SOURCE_EFFECTIVE_MORPHS_'+state+'.json')).read_text())
 byname={}
 for target in glb[state]['per_target']:
  key=(target['mesh'],target['name'])
  a=byname.setdefault(key,{'nonzero':0,'max_delta':0,'num_primitives':0,'target_indices':set()})
  a['nonzero']+=target['nonzero_deltas'];a['max_delta']=max(a['max_delta'],target['max_delta'])
  a['num_primitives']+=1;a['target_indices'].add(target['target_index'])
 expected=[]
 for source_mesh,rec in src.items():
  glbmesh=mesh_map[source_mesh]
  for name,v in rec['morphs'].items():
   if v['effective']:
    a=byname.get((glbmesh,name))
    expected.append({'source_mesh':source_mesh,'glb_mesh':glbmesh,'name':name,'source_index':v['source_target_index'],
                     'source_nonzero':v['nonzero_vertices'],'source_max_delta':v['max_delta'],
                     'glb_present':bool(a),'glb_effective':bool(a and a['nonzero']),
                     'glb_max_delta':a['max_delta'] if a else None,
                     'glb_nonzero_across_primitives':a['nonzero'] if a else 0,
                     'index_consistent_across_primitives':len(a['target_indices'])==1 if a else False})
 failures=[r for r in expected if not r['glb_effective']]
 # Compare source and GLB magnitude with conversion differences isolated.
 ratio=[r['glb_max_delta']/r['source_max_delta'] for r in expected if r['glb_effective'] and r['source_max_delta']>1e-6]
 inconsistent=[r for r in expected if not r['index_consistent_across_primitives']]
 out={'state':state,'source_effective_shape_keys':len(expected),'found_effective_glb_keys':len(expected)-len(failures),
      'missing_effective_keys':failures,
      'index_inconsistent_keys':inconsistent,
      'delta_ratio_min':min(ratio) if ratio else None,'delta_ratio_max':max(ratio) if ratio else None,
      'delta_ratio_median':statistics.median(ratio) if ratio else None,
      'sample_deltas':[r for r in expected if 'eye' in r['name'].lower() or 'mouth' in r['name'].lower()][:14]}
 results[state]=out
 print('SOURCE_TO_GLB_MORPH_PARITY',state,'source_effective',len(expected),'glb_effective',out['found_effective_glb_keys'],
       'lost',len(failures),'bad_index',len(inconsistent),'ratio_median',round(out['delta_ratio_median'],5) if ratio else None,flush=True)
 print('  FIRST_LOST',[(r['glb_mesh'],r['name']) for r in failures[:12]],flush=True)
dest=OUT/'P0_MORPH_SOURCE_TO_GLB_NAME_DELTA_PARITY.json';dest.write_text(json.dumps(results,indent=2))
print('SAVED',dest)
