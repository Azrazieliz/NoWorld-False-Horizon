"""Non-destructive modification of copied Test 03 Blender scene.
Resize only dress's lower panel into a short fitted shirt, retain rig, morphs, geometry source.
"""
import bpy,pathlib,json,time,math
start=time.perf_counter()
out=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261016\TEST03_ORANGE_BRAID')
body=bpy.data.objects.get('model.001_mesh')
assert body and body.type=='MESH'
mesh=body.data
target_mats=[i for i,m in enumerate(mesh.materials) if m and 'cf_m_top_shirtdress_ss_d' in m.name]
if len(target_mats)!=2:raise RuntimeError('Expected exactly two ShirtDress materials, got '+str(target_mats))
lower=min(target_mats)
affected=set()
for p in mesh.polygons:
 if p.material_index==lower:affected.update(p.vertices)
assert 1000<len(affected)<25000,('Invalid garment edit vertex count',len(affected))
keys=mesh.shape_keys.key_blocks if mesh.shape_keys else []
basis=keys[0] if keys else None
def src(vi):return (basis.data[vi].co.copy() if basis else mesh.vertices[vi].co.copy())
edits={}
for i in affected:
 coord=src(i);z=coord.z
 u=max(0,min(1,(z-.044)/(.092-.044)))
 factor=.30+.70*u
 new_z=.072+(z-.044)*(.092-.072)/(.092-.044)
 if z>.092:new_z=z
 delta=(coord.x*(factor-1),coord.y*(factor-1),new_z-z)
 edits[i]=delta
if keys:
 for key in keys:
  for i,delta in edits.items():
   pos=key.data[i].co
   pos.x+=delta[0];pos.y+=delta[1];pos.z+=delta[2]
else:
 for i,delta in edits.items():
  pos=mesh.vertices[i].co;pos.x+=delta[0];pos.y+=delta[1];pos.z+=delta[2]
mesh.update()
for idx in target_mats:
 m=mesh.materials[idx];m.diffuse_color=(.96,.968,.99,1.)
 m.use_nodes=True
 for node in m.node_tree.nodes:
  if node.type=='BSDF_PRINCIPLED':
   node.inputs['Base Color'].default_value=(.958,.969,.993,1.)
   node.inputs['Roughness'].default_value=.23
   node.inputs['Metallic'].default_value=.0
   node.inputs['Alpha'].default_value=.94
 try:m.surface_render_method='DITHERED'
 except:pass
# White thigh-highs, preserve existing masks/morphs.
for m in mesh.materials:
 if m and ('cf_m_socks_' in m.name or 'cf_m_panst_' in m.name):
  m.diffuse_color=(.965,.975,.997,1.)
# MMD skeleton bones are preserved for downstream runtime retarget.
arms=[a for a in bpy.data.objects if a.type=='ARMATURE']
print('ARMATURES',[(a.name,len(a.data.bones)) for a in arms],flush=True)
for a in arms:
 print('BONES',a.name,[b.name for b in list(a.data.bones)[:65]],flush=True)
blend=out/'Test03_OrangeBraid_ShirtPolished_Editable.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(blend))
result={'schema':'NW_TEST03_EDITABLE_SHIRT_GEOMETRY_POLISH_V1',
 'status':'PASS_EDITABLE_SHIRT_GEOMETRY_RESIZED',
 'source_blend':str(out/'Test03_OrangeBraid_NativeKKBP_Editable.blend'),
 'polished_blend':str(blend),'bytes':blend.stat().st_size,
 'shirt_materials':[mesh.materials[i].name for i in target_mats],
 'edited_verts':len(affected),'shape_keys_preserved':len(keys),
 'armature_count':len(arms),'elapsed_seconds':round(time.perf_counter()-start,2),
 'reference_fidelity_approved':False,'arm_pose_approved':False}
(out/'PolishReceipt.json').write_text(json.dumps(result,indent=2))
print('TEST03_BLENDER_POLISH='+json.dumps(result),flush=True)
