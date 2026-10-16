import gzip,json,pathlib,collections
root=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011\ANIMATION_REUSE')
for p in root.glob('*.gz'):
 with gzip.open(p,'rt',encoding='utf-8') as f:d=json.load(f)
 print('CLIP',p.name,'bytes',p.stat().st_size,'top keys',list(d))
 for k,v in d.items():
  if isinstance(v,dict):print('  ',k,'dict keys',list(v)[:20])
  elif isinstance(v,list):print('  ',k,'list len',len(v),'first',repr(v[0])[:180] if v else None)
  else:print('  ',k,str(v)[:100])
r=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016\G09_Luminous_Rig_Influences_Bind.json')
d=json.loads(r.read_text(encoding='utf-8'))
for pair,p in d['pairs'].items():
 print('PAIR',pair,'FIRST UNIQUES',[(x.split('/')[-1],x[:85]) for x in p['a_only_paths'][:12]])
 print(' MISMATCH',[(x['path'].split('/')[-1],round(x['rest_local_max_delta'],5),round(x['state_a_skin_weight'],3),round(x['state_b_skin_weight'],3)) for x in p['mismatched_detail'][:15]])
