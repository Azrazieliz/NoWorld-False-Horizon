"""Metadata only: match adult-mode motion transform binding CRCs to original
Koikatsu prefab joint paths. Never read/export animation keyframe values.
"""
import pathlib,sys,json,zlib,hashlib,collections
base=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty')
sys.path.insert(0,str(base/'pipeline/vendor_unitypy'))
import UnityPy
protected=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
doc=json.loads((protected/'G09_InstalledAdultNativeAnimation_MetadataOnly_20261019.json').read_text(encoding='utf8'))
prefab=pathlib.Path(r'D:\Tools\Steam\steamapps\common\Koikatsu Party\abdata\chara\oo_base.unity3d')
env=UnityPy.load(str(prefab))
items={}
for obj in env.objects:
 if obj.type.name!='Transform':continue
 o=obj.read()
 try:items[int(obj.path_id)]=(str(o.m_GameObject.read().m_Name),int(o.m_Father.path_id))
 except Exception:continue
hashes=collections.defaultdict(set)
for index in items:
 chain=[];seen=set();j=index
 while j in items and j not in seen:
  seen.add(j);name,parent=items[j];chain.insert(0,name);j=parent
 for off in range(len(chain)):
  path='/'.join(chain[off:])
  hashes[zlib.crc32(path.encode('utf8'))&0xffffffff].add(path)
report=[]
for d in doc['sampled_bundles']:
 if not d.get('clip_count'):continue
 env=UnityPy.load(d['path'])
 first=next((o.read() for o in env.objects if o.type.name=='AnimationClip'),None)
 if first is None:continue
 bb=getattr(first,'m_ClipBindingConstant')
 binds=list(getattr(bb,'genericBindings',[]))
 unique={int(x.path) for x in binds}
 match=unique&hashes.keys()
 ambiguous={h for h in match if len(hashes[h])>1}
 rec={'bundle_sha256':d['sha256'],'clip_count':d['clip_count'],
 'first_clip_bindings':len(binds),'unique_transform_paths':len(unique),
 'resolved_native_paths':len(match),'not_resolved':len(unique-match),
 'ambiguous_crc_paths':len(ambiguous),
 'supported_native_binding_fraction':round(len(match)/max(1,len(unique)),5)}
 report.append(rec)
 print('NATIVE_RIG_COMPAT',len(report),'resolved',len(match),'of',len(unique),flush=True)
res={'schema':'G09_ADULT_MODE_NATIVE_RIG_BINDING_METADATA_V1',
 'native_prefab_sha256':hashlib.sha256(prefab.read_bytes()).hexdigest(),
 'bundles_audited':len(report),'sampled':report,
 'true_native_joints_without_source_modification':True,
 'motion_keys_exported':False,'explicit_pose_generated':False,
 'production_ready':False,'character_eligibility_unverified':True}
out=protected/'G09_AdultNativeRigBindingCompatibility_20261019.json'
if out.exists():raise FileExistsError(out)
out.write_text(json.dumps(res,indent=2)+'\n',encoding='utf8')
print('BINDING_RESULTS',len(report), 'FULLY_RESOLVED',sum(x['not_resolved']==0 for x in report),
'MIN_PCT',min(x['supported_native_binding_fraction'] for x in report),flush=True)
