"""Read native Unity bone rest world matrices for precise Blender retarget basis."""
import sys,pathlib,json,hashlib,numpy as np
base=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty')
sys.path.insert(0,str(base/'pipeline/vendor_unitypy'))
import UnityPy
source=pathlib.Path(r'D:\Tools\Steam\steamapps\common\Koikatsu Party\abdata\chara\oo_base.unity3d')
target=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016\G09_Unity_Native_Transform_Hash_Map_20261019.json')
maps=json.loads(target.read_text(encoding='utf8'))['resolved']
env=UnityPy.load(str(source));items={}
for o in env.objects:
 if o.type.name!='Transform':continue
 t=o.read()
 try:
  r=t.m_LocalRotation;p=t.m_LocalPosition;s=t.m_LocalScale
  items[int(o.path_id)]={'name':t.m_GameObject.read().m_Name,'parent':int(t.m_Father.path_id),
   'p':[float(p.x),float(p.y),float(p.z)],'q':[float(r.x),float(r.y),float(r.z),float(r.w)],
   's':[float(s.x),float(s.y),float(s.z)]}
 except Exception:continue
def loc(x):
 q=np.array(x['q'],float);q/=np.linalg.norm(q)
 xx,yy,zz,ww=q
 R=np.array([[1-2*(yy*yy+zz*zz),2*(xx*yy-zz*ww),2*(xx*zz+yy*ww)],
 [2*(xx*yy+zz*ww),1-2*(xx*xx+zz*zz),2*(yy*zz-xx*ww)],
 [2*(xx*zz-yy*ww),2*(yy*zz+xx*ww),1-2*(xx*xx+yy*yy)]])
 M=np.eye(4);M[:3,:3]=R@np.diag(x['s']);M[:3,3]=x['p'];return M
cache={}
def world(i):
 if i not in cache:
  x=items[i];cache[i]=(world(x['parent']) if x['parent'] in items else np.eye(4))@loc(x)
 return cache[i]
def original_root(i):
 seen=set()
 while i in items and i not in seen:
  seen.add(i)
  if items[i]['name']=='cf_j_root':return i
  i=items[i]['parent']
 raise ValueError('prefab binding missing source root')
result={}
for crc,paths in maps.items():
 matches={x['path'] for x in paths}
 if len(matches)!=1:raise RuntimeError('unexpected prefab hash collision')
 path=next(iter(matches))
 node=paths[0]['node'];root=original_root(node)
 rel=np.linalg.inv(world(root))@world(node)
 result[path]={'crc32':int(crc),'relative_prefab_world_rest':rel.tolist(),
  'native_local_rest_rotation_xyzw':items[node]['q'],
  'native_source_leaf_name':items[node]['name']}
r={'schema':'G09_KOIKATSU_AUTHENTIC_PREFAB_REST_BASIS_V1',
 'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'joints':result,'count':len(result)}
out=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016\G09_Unity_Native_Prefab_RestBasis_20261019.json')
if out.exists():raise FileExistsError(out)
out.write_text(json.dumps(r,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
print('NATIVE_REST_BASIS',len(result),hashlib.sha256(out.read_bytes()).hexdigest(),flush=True)
