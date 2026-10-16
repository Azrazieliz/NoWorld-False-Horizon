"""Read-only PMX 2.0/2.1 vertex/faces/bounds/vertex bone-deform completeness audit."""
import struct,pathlib,json,hashlib,collections
ROOT=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011\FRESH_PARTY_EXPORTS')
def analyze(path):
 data=path.read_bytes()
 if data[:4]!=b'PMX ':raise ValueError('Invalid PMX signature')
 version=struct.unpack_from('<f',data,4)[0]
 header_size=data[8];flags=data[9:9+header_size];encoding,additional_uv,vertex_index_size,texture_index_size,material_index_size,bone_index_size=flags[:6]
 pos=9+header_size
 def geti():
  nonlocal pos
  value=struct.unpack_from('<i',data,pos)[0];pos+=4;return value
 for _ in range(4):
  n=geti();pos+=n
 count=geti();mins=[1e9]*3;maxs=[-1e9]*3;deforms=collections.Counter()
 for _ in range(count):
  co=struct.unpack_from('<3f',data,pos);pos+=12
  for k in range(3):mins[k]=min(mins[k],co[k]);maxs[k]=max(maxs[k],co[k])
  pos+=12+8+16*additional_uv
  flag=data[pos];pos+=1
  deforms[flag]+=1
  if flag==0:pos+=bone_index_size
  elif flag==1:pos+=bone_index_size*2+4
  elif flag in (2,4):pos+=bone_index_size*4+16
  elif flag==3:pos+=bone_index_size*2+4+36
  else:raise ValueError('Unknown deformation flag %s in %s'%(flag,path))
  pos+=4
 indices=geti()
 if indices<0 or indices%3:raise ValueError('Invalid PMX triangle indices')
 tri=indices//3
 result={'path':str(path),'sha256':hashlib.sha256(data).hexdigest(),'pmx_version':round(version,3),
 'vertex_count':count,'triangles':tri,'uv_channels':1+additional_uv,
 'position_min':mins,'position_max':maxs,'local_z_height':maxs[2]-mins[2],
 'bone_deform_counts':{str(k):v for k,v in deforms.items()},
 'indexed_bone_weights_vertices':sum(v for k,v in deforms.items() if k in (0,1,2,3,4)),
 'file_size':len(data)}
 return result
rows={}
for state in ('CLOTHED','UNDERWEAR','NUDE'):
 for part,rel in [('BODY','model.pmx'),('OUTFIT','Outfit 00/model.pmx')]:
  p=ROOT/state/rel
  rows[state+'_'+part]=analyze(p)
out=pathlib.Path(r'D:\UnrealProjects\NW_SoftwareMaster_P0_20261016\A_PMX_GEOMETRY_STAGE_AUDIT.json')
out.write_text(json.dumps(rows,indent=2))
for tag,row in rows.items():print('A_PMX',tag,'vertices',row['vertex_count'],'tris',row['triangles'],'zheight_source_units',round(row['local_z_height'],5),'uvs',row['uv_channels'],'weights',row['indexed_bone_weights_vertices'],'deforms',row['bone_deform_counts'],flush=True)
