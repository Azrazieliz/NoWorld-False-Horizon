"""Exact GLB 2.0 POSITION morph delta reader, including sparse (no dense bufferView).
Read-only. Compares effective/non-effective morphs without conflating repeated material
primitives, aliases or zero-delta placeholder key names.
"""
import pathlib,struct,json,collections,hashlib
import numpy as np
ROOT=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011')
OUT=pathlib.Path(r'D:\UnrealProjects\NW_SoftwareMaster_P0_20261016')
states={}
def get_accessor(a,views,buff):
 n=a['count'];ctype=a['componentType'];q={5121:'u1',5123:'<u2',5125:'<u4',5126:'<f4'}[ctype]
 cols={'VEC3':3,'VEC2':2,'VEC4':4,'SCALAR':1}[a['type']]
 dtype=np.dtype(q)
 if 'bufferView' in a:
  v=views[a['bufferView']]
  stride=v.get('byteStride',cols*dtype.itemsize)
  start=v.get('byteOffset',0)+a.get('byteOffset',0)
  if stride==cols*dtype.itemsize:
   result=np.frombuffer(buff,dtype=dtype,count=n*cols,offset=start).reshape(n,cols).copy()
  else:
   result=np.ndarray((n,cols),dtype=dtype,buffer=buff,offset=start,strides=(stride,dtype.itemsize)).copy()
 else:result=np.zeros((n,cols),dtype=dtype)
 if 'sparse' in a:
  s=a['sparse'];idx=s['indices'];iv=views[idx['bufferView']]
  typ=np.dtype({5121:'u1',5123:'<u2',5125:'<u4'}[idx['componentType']])
  i0=iv.get('byteOffset',0)+idx.get('byteOffset',0)
  inds=np.frombuffer(buff,dtype=typ,count=s['count'],offset=i0)
  vv=s['values'];vview=views[vv['bufferView']]
  v0=vview.get('byteOffset',0)+vv.get('byteOffset',0)
  vals=np.frombuffer(buff,dtype=dtype,count=s['count']*cols,offset=v0).reshape(s['count'],cols)
  result[inds]=vals
 return result
for state in ('CLOTHED','UNDERWEAR','NUDE'):
 path=ROOT/'BLENDER_FRESH'/state/f'LUMINOUS_V3_{state}_skinned.glb'
 raw=path.read_bytes();jn=struct.unpack_from('<I',raw,12)[0];j=json.loads(raw[20:20+jn])
 bp=(20+jn+3)&~3
 l,typ=struct.unpack_from('<I4s',raw,bp);assert typ==b'BIN\x00'
 buff=memoryview(raw)[bp+8:bp+8+l];acc=j['accessors'];views=j['bufferViews']
 metrics=[];tot=0;effective=0;unique={}
 for mi,m in enumerate(j['meshes']):
  names=m.get('extras',{}).get('targetNames',[])
  for pi,pr in enumerate(m['primitives']):
   for ti,target in enumerate(pr.get('targets',[])):
    pos=target.get('POSITION')
    if pos is None:continue
    a=acc[pos]
    data=get_accessor(a,views,buff)
    sizes=np.linalg.norm(data.astype('float64'),axis=1)
    nz=int((sizes>1e-7).sum())
    md=float(sizes.max()) if sizes.size else 0
    name=names[ti] if ti<len(names) else ''
    row={'mesh':m['name'],'mi':mi,'primitive':pi,'target_index':ti,'name':name,'accessor':pos,'nonzero_deltas':nz,
      'max_delta':round(md,8),'sparse_index_count':a.get('sparse',{}).get('count',0),
      'is_sparse':bool('sparse' in a)}
    metrics.append(row);tot+=1;effective+=nz>0
    agg=unique.setdefault((m['name'],name),{'effective_primitives':0,'nonzero_deltas_sum':0,'max_delta':0,'primitives':0})
    agg['primitives']+=1;agg['effective_primitives']+=nz>0;agg['nonzero_deltas_sum']+=nz
    agg['max_delta']=max(agg['max_delta'],md)
 # Select one meaningful body/face sample for source-to-output later.
 significant=[r for r in metrics if r['name'] in ('Eye Cartoony Crying','Eye Fiery Eyes','eye_face.f00_def_cl','eye_face.f00_egao_op','kuti_face.f00_bisyou_op','Basis.001')]
 result={'file':str(path),'sha256':hashlib.sha256(raw).hexdigest(),
 'total_position_target_accessors':tot,'nonzero_effective_accessors':effective,'zero_effective_accessors':tot-effective,
 'per_target':metrics,'examples':significant,'unique_mesh_and_names':len(unique),
 'unique_effective_mesh_and_names':sum(x['nonzero_deltas_sum']>0 for x in unique.values()),
 'unreal_effectiveness':'NOT_TESTED',
 'glb_effective_deltas_measured':True}
 states[state]=result
 print('EFFECTIVE',state,'total',tot,'nonzero',effective,'zero',tot-effective,'unique_mesh_names',len(unique),'effective_unique',result['unique_effective_mesh_and_names'],flush=True)
 print('SAMPLES',[(r['mesh'],r['name'],r['nonzero_deltas'],r['max_delta']) for r in significant if r['nonzero_deltas']][:15],flush=True)
outfile=OUT/'P0_GLB_EFFECTIVE_SPARSE_MORPH_DELTAS.json'
outfile.write_text(json.dumps(states,indent=2))
print('FILE',outfile)
