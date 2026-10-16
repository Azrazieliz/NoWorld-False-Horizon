"""Gate 09 immutable-source GLB skin/bind/influence comparison. Metadata only."""
import argparse,hashlib,json,struct,pathlib,collections,re
import numpy as np
TYPE={5120:'i1',5121:'u1',5122:'<i2',5123:'<u2',5125:'<u4',5126:'<f4'}
DIMS={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}
def load(p):
 raw=pathlib.Path(p).read_bytes()
 assert raw[:4]==b'glTF' and struct.unpack_from('<I',raw,8)[0]==len(raw)
 off=12;chunks={}
 while off<len(raw):
  n,t=struct.unpack_from('<II',raw,off);off+=8;chunks[t]=raw[off:off+n];off+=n
 return hashlib.sha256(raw).hexdigest(),json.loads(chunks[0x4E4F534A]),chunks[0x004E4942]
def acc(d,b,i):
 a=d['accessors'][i];n=a['count'];k=DIMS[a['type']];dt=np.dtype(TYPE[a['componentType']])
 out=np.zeros((n,k),dtype=dt)
 if 'bufferView' in a:
  v=d['bufferViews'][a['bufferView']]
  out=np.ndarray((n,k),dtype=dt,buffer=b,offset=v.get('byteOffset',0)+a.get('byteOffset',0),strides=(v.get('byteStride',k*dt.itemsize),dt.itemsize)).copy()
 if 'sparse' in a:
  s=a['sparse'];v=d['bufferViews'][s['indices']['bufferView']];w=d['bufferViews'][s['values']['bufferView']]
  ids=np.frombuffer(b,dtype=TYPE[s['indices']['componentType']],count=s['count'],offset=v.get('byteOffset',0)+s['indices'].get('byteOffset',0))
  vals=np.frombuffer(b,dtype=dt,count=s['count']*k,offset=w.get('byteOffset',0)+s['values'].get('byteOffset',0)).reshape((-1,k))
  out[ids]=vals
 return out
def transform(n):
 if 'matrix' in n:return np.array(n['matrix'],dtype=float).reshape(4,4,order='F')
 t=n.get('translation',[0,0,0]);sx,sy,sz=n.get('scale',[1,1,1]);x,y,z,w=n.get('rotation',[0,0,0,1])
 q=np.array([x,y,z,w],dtype=float);q/=np.linalg.norm(q);x,y,z,w=q
 m=np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w),t[0]],[2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w),t[1]],[2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y),t[2]],[0,0,0,1]],float)
 m[:3,:3]*=[sx,sy,sz];return m
def kind(s):
 # Categorize terminal bone, NOT its ancestor chain (all parts inherit Hips/Head).
 t=s.rsplit('/',1)[-1].lower()
 if re.search('hips|pelvis|spine|chest|neck|head|thigh|calf|knee|foot|toe|upperarm|lowerarm|forearm|hand|shoulder',t):return 'possible_core'
 if re.search('hair|pony|braid|skirt|dress|sleeve|cloth|accessor|ribbon|bow|sock|shoe',t):return 'possible_secondary'
 return 'unknown'
def inspect(p):
 sha,d,b=load(p);nodes=d['nodes'];parents={c:i for i,node in enumerate(nodes) for c in node.get('children',[])}
 def ident(i):
  names=[];visited=set()
  while True:
   if i in visited:raise ValueError('cycle')
   visited.add(i);names.append(nodes[i].get('name',str(i)))
   if i not in parents:break
   i=parents[i]
  return '/'.join(names[::-1])
 cache={}
 def glob(i):
  if i not in cache:cache[i]=(glob(parents[i]) if i in parents else np.eye(4))@transform(nodes[i])
  return cache[i]
 sk=d['skins'][0];ids=sk['joints'];names=[ident(i) for i in ids]
 if len(set(names))!=len(names):raise ValueError('duplicate joint path')
 weights=np.zeros(len(ids));vertices=np.zeros(len(ids),dtype=int)
 for n in nodes:
  if 'mesh' not in n or n.get('skin')!=0:continue
  for pr in d['meshes'][n['mesh']]['primitives']:
   a=pr['attributes']
   if 'JOINTS_0' not in a or 'WEIGHTS_0' not in a:continue
   ji=acc(d,b,a['JOINTS_0']).astype(int);wt=acc(d,b,a['WEIGHTS_0'])
   for col in range(ji.shape[1]):
    np.add.at(weights,ji[:,col],wt[:,col]);np.add.at(vertices,ji[:,col],wt[:,col]>1e-6)
 ib=acc(d,b,sk['inverseBindMatrices']) if 'inverseBindMatrices' in sk else None
 info={}
 for k,i in enumerate(ids):
  info[names[k]]={'name':nodes[i].get('name',''),'local':transform(nodes[i]).tolist(),'world':glob(i).tolist(),
    'ibm':ib[k].reshape(4,4,order='F').tolist() if ib is not None else None,
    'weight_sum':float(weights[k]),'vertex_influences':int(vertices[k]),'hint':kind(names[k])}
 return {'hash':sha,'paths':names,'info':info,'joints':len(ids),
   'weighted_joints':int(sum(weights>1e-6)),'animations':[{ 'name':x.get('name'),'channels':len(x.get('channels',[]))} for x in d.get('animations',[])]}
def pair(a,b):
 aa=a['info'];bb=b['info'];shared=set(aa)&set(bb);left=set(aa)-set(bb);right=set(bb)-set(aa)
 mismatch=[]
 for p in sorted(shared):
  x,y=aa[p],bb[p];ld=float(np.max(np.abs(np.array(x['local'])-np.array(y['local']))))
  wd=float(np.max(np.abs(np.array(x['world'])-np.array(y['world']))))
  bd=float(np.max(np.abs(np.array(x['ibm'])-np.array(y['ibm'])))) if x['ibm'] is not None and y['ibm'] is not None else None
  if ld>1e-6:mismatch.append({'path':p,'rest_local_max_delta':ld,'rest_world_max_delta':wd,'inverse_bind_max_delta':bd,
    'state_a_skin_weight':x['weight_sum'],'state_b_skin_weight':y['weight_sum'],'family_hint':x['hint']})
 return {'shared':len(shared),'only_a':len(left),'only_b':len(right),'identical_order':a['paths']==b['paths'],
  'shared_rest_mismatches':len(mismatch),'mismatch_affects_weighted_joint_count':sum(1 for m in mismatch if m['state_a_skin_weight']>1e-6 or m['state_b_skin_weight']>1e-6),
  'mismatched_possible_core':sum(1 for m in mismatch if m['family_hint']=='possible_core'),
  'a_unique_weighted_joints':sum(aa[p]['weight_sum']>1e-6 for p in left),'b_unique_weighted_joints':sum(bb[p]['weight_sum']>1e-6 for p in right),
  'a_unique_hints':dict(collections.Counter(aa[p]['hint'] for p in left)),
  'b_unique_hints':dict(collections.Counter(bb[p]['hint'] for p in right)),
  'mismatched_detail':mismatch,'a_only_paths':sorted(left),'b_only_paths':sorted(right),
  'same_skeleton_accepted':False}
def main():
 arg=argparse.ArgumentParser()
 for k in ['clothed','underwear','nude','output']:arg.add_argument('--'+k,required=True)
 a=arg.parse_args();data={k:inspect(getattr(a,k)) for k in ['clothed','underwear','nude']}
 pairs={x+'_'+y:pair(data[x],data[y]) for x,y in [('clothed','underwear'),('clothed','nude'),('underwear','nude')]}
 public={'schema':'G09_READONLY_SKIN_COMPAT_V1','states':{k:{'sha256':v['hash'],'joints':v['joints'],'weighted_joints':v['weighted_joints'],'animations':v['animations']} for k,v in data.items()},'pairs':pairs,'not_accepted':['No identical rig acceptance','No observed posed-deformation proof','Bone family regex only a hint','Gate12 owns Unreal retarget']}
 out=pathlib.Path(a.output)
 if out.exists():raise FileExistsError(out)
 out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(public,indent=2)+'\n',encoding='utf-8')
 print(json.dumps({'states':public['states'],'pairs':{k:{x:y for x,y in p.items() if x not in ('mismatched_detail','a_only_paths','b_only_paths')} for k,p in pairs.items()}},indent=2))
if __name__=='__main__':main()
