import bpy
print('BLENDER_VERSION',bpy.app.version_string)
for x in bpy.data.objects:
 if x.type in ('MESH','ARMATURE'):print('OBJECT',x.name,'type',x.type,'verts',len(x.data.vertices) if x.type=='MESH' else len(x.data.bones),'location',tuple(round(v,3) for v in x.location),'scale',tuple(round(v,3) for v in x.scale),'bbox',tuple((round(min((x.matrix_world@__import__('mathutils').Vector(c))[k] for c in x.bound_box),3),round(max((x.matrix_world@__import__('mathutils').Vector(c))[k] for c in x.bound_box),3)) for k in range(3)) if x.type=='MESH' else '')
for m in list(bpy.data.materials)[:22]: print('MATERIAL',m.name,'nodes',bool(m.use_nodes),'imgs',[(n.image.name if n.image else '') for n in m.node_tree.nodes if n.type=='TEX_IMAGE'][:3] if m.use_nodes else [])
