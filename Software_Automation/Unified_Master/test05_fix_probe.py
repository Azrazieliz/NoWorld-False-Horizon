import bpy,math
from mathutils import Vector
for o in bpy.data.objects:
 if o.type in ('MESH','ARMATURE') and ('KKBP' in o.name or 'model_arm' in o.name or 'model.001_mesh' in o.name):
  print('KK_SOURCE',o.name,o.type,'hide',o.hide_render,o.hide_viewport,'scale',tuple(round(x,2) for x in o.scale),'mod',[(m.type,m.object.name if m.type=='ARMATURE' and m.object else '') for m in o.modifiers][:5],flush=True)
  if o.type=='MESH':
   ev=o.evaluated_get(bpy.context.evaluated_depsgraph_get())
   mesh=ev.to_mesh();coords=[ev.matrix_world@v.co for v in mesh.vertices]
   print('EVALUATED_BOUNDS',[(round(min(v[i] for v in coords),3),round(max(v[i] for v in coords),3)) for i in range(3)])
   ev.to_mesh_clear()
print('ORTHO',bpy.context.scene.camera.data.ortho_scale,'CAMLOC',bpy.context.scene.camera.location[:])
