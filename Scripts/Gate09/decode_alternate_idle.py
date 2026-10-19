"""Reconstruct authentic Unity 5.x sampled transform curves for three Koikatsu idles.
No guessing bones: hash must resolve to Unity prefab path. Source stored protected.
"""
import pathlib,sys,json,gzip,struct,math,bisect,hashlib,collections
import numpy as np
root=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty')
sys.path.insert(0,str(root/'pipeline/vendor_unitypy'))
import UnityPy
protected=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
src=protected/'koikatsu_idle_alternates_20261019'
mapping=json.loads((protected/'G09_Unity_Native_Transform_Hash_Map_20261019.json').read_text(encoding='utf8'))['resolved']
prefab=pathlib.Path(r'D:\Tools\Steam\steamapps\common\Koikatsu Party\abdata\chara\oo_base.unity3d')
env=UnityPy.load(str(prefab))
rotation={}
for o in env.objects:
 if o.type.name=='Transform':
  try:
   t=o.read();r=t.m_LocalRotation
   rotation[int(o.path_id)]=[float(r.x),float(r.y),float(r.z),float(r.w)]
  except (AttributeError,ValueError):pass
def decode(frames):
 a=frames['packed_u32'];i=0;result=[]
 while i<len(a):
  t=struct.unpack('<f',struct.pack('<I',a[i]))[0];n=a[i+1];i+=2
  if n>1000:raise RuntimeError('invalid stream key count')
  keys={}
  for j in range(n):
   idx=a[i];coeff=struct.unpack('<ffff',struct.pack('<IIII',*a[i+1:i+5]));i+=5
   keys[idx]=float(coeff[3])
  result.append((t,keys))
 assert i==len(a)
 return result
def hash_file(p):
 return hashlib.sha256(p.read_bytes()).hexdigest()
for p in sorted(src.glob('*.gz')):
 with gzip.open(p,'rt',encoding='utf8') as f:d=json.load(f)
 binds=d['bindings'];dense=d['dense'];n_stream=d['streamed']['count'];n_dense=dense['count'];const=np.array(d['constant'],dtype=float)
 samples=np.array(dense['samples'],dtype=float).reshape((dense['frames'],n_dense))
 events=decode(d['streamed'])
 num=int(d['duration_seconds']*d['fps'])+1
 if num>900:raise ValueError('unreasonable duration')
 values=np.zeros(n_stream+n_dense+len(const),dtype=float)
 if values.size!=sum(3 if b['attribute'] in (1,3) else 4 if b['attribute']==2 else 1 for b in binds):raise RuntimeError('not matching clip channels')
 values[n_stream+n_dense:]=const
 frames=[];cur_keys={};j=0
 for f in range(num):
  t=f/float(d['fps'])
  while j<len(events) and events[j][0]<=t+0.000001:
   cur_keys.update(events[j][1]);j+=1
  for index,v in cur_keys.items():
   if index>=n_stream:raise ValueError('stream index overflow')
   values[index]=v
  k=min(dense['frames']-1,max(0,round((t-dense['start'])*dense['rate'])))
  values[n_stream:n_stream+n_dense]=samples[k]
  row={};off=0
  for b in binds:
   length={1:3,2:4,3:3}.get(b['attribute'])
   if length is None:raise ValueError('unexpected Unity transform attribute')
   hh=str(b['path_crc32'])
   if hh not in mapping:raise RuntimeError('no authentic prefab binding '+hh)
   vals=values[off:off+length];off+=length
   key=next(iter({m['path'] for m in mapping[hh]}))
   piece=row.setdefault(key,{})
   piece[{1:'local_position',2:'local_rotation_xyzw',3:'local_scale'}[b['attribute']]]=vals.tolist()
  if off!=len(values):raise ValueError('clip access error')
  frames.append(row)
 discrepancies=[];rest={}
 for h,items in mapping.items():
  ps={x['path'] for x in items}
  if len(ps)!=1:raise RuntimeError('native hash collision')
  nam=next(iter(ps))
  rots=[rotation[x['node']] for x in items if x['node'] in rotation]
  if not rots:continue
  med=np.median(np.array(rots),axis=0).tolist()
  diff=max(np.linalg.norm(np.array(v)-np.array(med)) for v in rots)
  rest[nam]=med
  if diff>0.01:discrepancies.append({'path':nam,'rest_variant_max_quat_component_euclid':float(diff)})
 quats=[]
 for f in frames:
  for nm,row in f.items():
   if 'local_rotation_xyzw' in row:
    q=row['local_rotation_xyzw'];quats.append(sum(x*x for x in q)**0.5)
 # Record genuine motion, not an artistic success
 record={'schema':'GATE09_DECODED_KOIKATSU_UNITY_NATIVE_TRANSFORM_SAMPLES_V1',
 'source_sha256':hash_file(p),'source_clip_name':d['clip_name'],'fps':d['fps'],
 'frames':frames,'native_prefab_rest_quaternions_xyzw':rest,'native_rest_conflicts':discrepancies,
 'minimum_sampled_quaternion_norm':min(quats),'maximum_sampled_quaternion_norm':max(quats),
 'unity_prefab_source':str(prefab),'unity_prefab_sha256':hash_file(prefab),
 'all_hashes_resolved':True,'blender_bake':'NOT_YET','unreal_test':'NOT_YET'}
 out=protected/(d['clip_name']+'_G09_Native_30FPS_Samples.json.gz')
 if out.exists():raise FileExistsError(out)
 with gzip.open(out,'wt',encoding='utf8',compresslevel=5) as f:json.dump(record,f,separators=(',',':'))
 print('SAMPLED',p.name,'frames',len(frames),'rest_conflicts',len(discrepancies),
       'quaternion_norm',round(min(quats),4),round(max(quats),4),'sha256',hash_file(out),flush=True)
