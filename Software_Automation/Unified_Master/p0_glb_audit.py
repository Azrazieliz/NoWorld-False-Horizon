"""Read-only GLB 2.0 mesh/skin/texture counts and bounding information by accessor."""
import pathlib,struct,json,hashlib,collections
ROOT=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011')
DATA={}
def sha(p):
 h=hashlib.sha256()
 with open(p,'rb') as f:
  for x in iter(lambda:f.read(1<<20),b''):h.update(x)
 return h.hexdigest()
def analyze(path):
 b=path.read_bytes()
 magic,version,total=struct.unpack_from('<4sII',b,0)
 if magic!=b'glTF' or version!=2 or total!=len(b):raise ValueError('Bad GLB header '+str(path))
 off=12
 length,tag=struct.unpack_from('<I4s',b,off);off+=8
 if tag!=b'JSON':raise ValueError('Missing GLB JSON')
 data=json.loads(b[off:off+length])
 acc=data.get('accessors',[])
 meshes=data.get('meshes',[])
 primitive_total=sum(len(m.get('primitives',[])) for m in meshes)
 vertex=sum(acc[x['attributes']['POSITION']]['count'] for m in meshes for x in m.get('primitives',[]) if 'POSITION' in x.get('attributes',{}))
 tri=sum(acc[p['indices']]['count']//3 if 'indices' in p else acc[p['attributes']['POSITION']]['count']//3 for m in meshes for p in m['primitives'] if p.get('mode',4)==4)
 morphs=sum(len(p.get('targets',[])) for m in meshes for p in m['primitives'])
 uv_prims=sum('TEXCOORD_0' in p.get('attributes',{}) for m in meshes for p in m['primitives'])
 skin_prims=sum('JOINTS_0' in p.get('attributes',{}) and 'WEIGHTS_0' in p.get('attributes',{}) for m in meshes for p in m['primitives'])
 bounds=[]
 for m in meshes:
  for p in m['primitives']:
   pos=acc[p['attributes']['POSITION']]
   if 'min' in pos and 'max' in pos:bounds.append([pos['min'],pos['max']])
 images=data.get('images',[])
 materials=data.get('materials',[])
 mat_albedo=sum(bool(m.get('pbrMetallicRoughness',{}).get('baseColorTexture')) for m in materials)
 return {'file':str(path),'bytes':len(b),'sha256':sha(path),
 'mesh_count':len(meshes),'primitive_count':primitive_total,'vertices_per_primitive_sum':vertex,'triangle_count':tri,'morph_targets_across_primitives':morphs,
 'skinned_primitives':skin_prims,'uv0_primitives':uv_prims,'armature_skin_count':len(data.get('skins',[])),
 'joint_counts':[len(s.get('joints',[])) for s in data.get('skins',[])],
 'image_count':len(images),'image_embedded_buffer_views':sum('bufferView' in i for i in images),
 'texture_count':len(data.get('textures',[])),'material_count':len(materials),
 'materials_base_color_texture':mat_albedo,
 'accessor_primitive_bounds':bounds[:6],
 'total_nodes':len(data.get('nodes',[])), 'total_animations':len(data.get('animations',[]))}
for state in ('CLOTHED','UNDERWEAR','NUDE'):
 path=ROOT/'BLENDER_FRESH'/state/('LUMINOUS_V3_'+state+'_skinned.glb')
 DATA[state]=analyze(path)
out=pathlib.Path(r'D:\UnrealProjects\NW_SoftwareMaster_P0_20261016')
out.mkdir(parents=True,exist_ok=True)
p=out/'A_LUMINOUS_GLB_STAGE_AUDIT.json'
p.write_text(json.dumps(DATA,indent=2))
for state,row in DATA.items():
 print('A_GLB',state,'vertices',row['vertices_per_primitive_sum'],'tris',row['triangle_count'],'skins',row['joint_counts'],'uv0',row['uv0_primitives'],'morph',row['morph_targets_across_primitives'],'images',row['image_count'],'albedo_materials',row['materials_base_color_texture'],flush=True)
