"""READ ONLY original Luminous V3 source .blend: texture/UV/shape/rig baseline report."""
import bpy,json,pathlib,hashlib,collections
src=pathlib.Path(bpy.data.filepath)
out=pathlib.Path(r'D:\UnrealProjects\NW_SoftwareMaster_P0_20261016')
out.mkdir(parents=True,exist_ok=True)
meshes=[x for x in bpy.data.objects if x.type=='MESH']
rigs=[x for x in bpy.data.objects if x.type=='ARMATURE']
materials=list(bpy.data.materials)
images=[x for x in bpy.data.images if x.source=='FILE' and x.type=='IMAGE']
props=[]
for mat in materials:
 imgs=[]
 groups=[]
 if mat.use_nodes and mat.node_tree:
  for node in mat.node_tree.nodes:
   if node.type=='TEX_IMAGE':imgs.append({'image':node.image.name if node.image else None,'file':node.image.filepath if node.image else None})
   if node.type=='GROUP':groups.append(node.node_tree.name if node.node_tree else None)
 props.append({'name':mat.name,'images':imgs,'groups':groups[:9], 'alpha':mat.blend_method if hasattr(mat,'blend_method') else str(getattr(mat,'surface_render_method',None))})
shape=sum(max(0,len(m.data.shape_keys.key_blocks)-1) for m in meshes if m.data.shape_keys)
meshrows=[]
for m in meshes:
 meshrows.append({'name':m.name,'verts':len(m.data.vertices),'triangles':sum(max(0,len(f.vertices)-2) for f in m.data.polygons),
 'uv_layers':len(m.data.uv_layers),'shapekeys':len(m.data.shape_keys.key_blocks)-1 if m.data.shape_keys else 0,
 'mods':[x.type for x in m.modifiers],
 'material_slots':[x.name if x else None for x in m.data.materials[:8]],
 'bounds_local': [[round(min(v.co[i] for v in m.data.vertices),6),round(max(v.co[i] for v in m.data.vertices),6)] for i in range(3)] if m.data.vertices else []})
res={'source':str(src),'state':'CLOTHED','blender_version':bpy.app.version_string,'meshes':len(meshes),'vertices':sum(x['verts'] for x in meshrows),'triangles':sum(x['triangles'] for x in meshrows),'armatures':[(x.name,len(x.data.bones)) for x in rigs],'shapekeys':shape,'images_loaded':len(images),'images_count_total':len(bpy.data.images),'image_nodes_total':sum(len(x['images']) for x in props),'image_nodes_resolved':sum(sum(bool(n['image']) for n in x['images']) for x in props),'materials':len(materials),'meshes_detail':meshrows,'material_details':props}
(out/'A_LUMINOUS_BASE_BLEND_BEFORE.json').write_text(json.dumps(res,indent=2))
print('P0_BASE_BLEND_INSPECT='+json.dumps({k:v for k,v in res.items() if k not in ('meshes_detail','material_details')}),flush=True)
for mat in props[:14]:print('MAT',mat['name'],'image_nodes',len(mat['images']),'groups',mat['groups'][:3],flush=True)
