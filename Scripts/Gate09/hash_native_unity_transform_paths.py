"""Hash native Unity Transform paths against real animation binding CRCs. Read-only."""
import pathlib,sys,json,gzip,zlib,collections
BASE=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty')
sys.path.insert(0,str(BASE/'pipeline/vendor_unitypy'))
import UnityPy
path=pathlib.Path(r'D:\Tools\Steam\steamapps\common\Koikatsu Party\abdata\chara\oo_base.unity3d')
env=UnityPy.load(str(path))
items={}
for o in env.objects:
 if o.type.name != 'Transform':continue
 t=o.read()
 try:
  name=str(t.m_GameObject.read().m_Name)
  parent=int(t.m_Father.path_id)
  items[int(o.path_id)]=(name,parent)
 except Exception:continue
print('READ',len(items))
cr=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011\ANIMATION_REUSE\STAND_IDLE_f_tachi_00_18_L.unitycurves.json.gz')
with gzip.open(cr,'rt') as f:d=json.load(f)
hashes={b['path'] for b in d['native_bindings']}
hits=collections.defaultdict(list)
for i,(name,parent) in items.items():
 chain=[];seen=set();j=i
 while j in items and j not in seen:
  seen.add(j);nm,pp=items[j];chain.insert(0,nm);j=pp
 for o in range(len(chain)):
  suffix='/'.join(chain[o:])
  h=zlib.crc32(suffix.encode('utf-8'))&0xffffffff
  if h in hashes:hits[h].append({'node':i,'path':suffix,'ancestor_levels_removed':o})
print('NATIVE PREFAB CRC RESOLVED',len(hits),'/',len(hashes),'MULTI',sum(len(x)>1 for x in hits.values()),flush=True)
for h in list(hits)[:36]:print(h,hits[h][:2])
r=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016\G09_Unity_Native_Transform_Hash_Map_20261019.json')
if r.exists():raise FileExistsError(r)
r.write_text(json.dumps({'schema':'G09_KOIKATSU_NATIVE_PREFAB_TRANSFORM_PATH_MAP_V1','source':str(path),
'mapped_binding_paths':len(hits),'total_binding_paths':len(hashes),
'resolved':{str(k):v for k,v in hits.items()}},indent=2,ensure_ascii=False)+'\n',encoding='utf8')
