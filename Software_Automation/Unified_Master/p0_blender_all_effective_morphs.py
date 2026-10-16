"""Source Blender all morph names: compute actual effective basis-to-key deltas, not only stored names."""
import bpy,pathlib,json,numpy as np,collections
out=pathlib.Path(r'D:\UnrealProjects\NW_SoftwareMaster_P0_20261016')
rows={}
for obj in bpy.data.objects:
 if obj.type!='MESH' or not obj.data.shape_keys:continue
 keys=obj.data.shape_keys.key_blocks
 verts=len(obj.data.vertices)
 basis=np.empty(verts*3,dtype=np.float32);keys[0].data.foreach_get('co',basis);basis=basis.reshape((-1,3))
 d={}
 for index,key in enumerate(keys):
  if index==0:continue
  coords=np.empty(verts*3,dtype=np.float32);key.data.foreach_get('co',coords)
  delta=(coords.reshape(-1,3)-basis)
  lengths=np.linalg.norm(delta,axis=1)
  nz=int((lengths>1e-7).sum())
  d[key.name]={'source_target_index':index-1,'effective':nz>0,'nonzero_vertices':nz,
               'max_delta':round(float(lengths.max()) if lengths.size else 0,9)}
 rows[obj.name]={'vertices':verts,'count':len(d),'effective_count':sum(x['effective'] for x in d.values()),'morphs':d}
 print('SOURCE_EFFECTIVE',obj.name,'vertices',verts,'names',len(d),'nonzero',rows[obj.name]['effective_count'],'examples',[(n,v['nonzero_vertices']) for n,v in d.items() if v['nonzero_vertices']][:8],flush=True)
state=next((s for s in ('CLOTHED','UNDERWEAR','NUDE') if '_'+s+'_' in pathlib.Path(bpy.data.filepath).name),'CLOTHED')
out.joinpath('P0_BLEND_SOURCE_EFFECTIVE_MORPHS_'+state+'.json').write_text(json.dumps(rows,indent=2))
print('SOURCE_EFFECTIVE_TOTAL',sum(x['effective_count'] for x in rows.values()),flush=True)
