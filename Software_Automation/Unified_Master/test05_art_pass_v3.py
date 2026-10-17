"""Test05 corrective art pass: restore subject visibility, realistic PBR contrast and wing anatomy.
Works only on saved isolated QA v2 .blend; does NOT modify originals or canonical content.
"""
import bpy,math,pathlib,random,json
from mathutils import Vector
root=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST05_CRYSTAL_SERAPH')
def mat(name):
 return bpy.data.materials['T05 '+name]
def tune(name,c,metal=None,rough=None,alpha=None):
 m=mat(name)
 p=m.node_tree.nodes.get('Principled BSDF')
 if p:
  p.inputs['Base Color'].default_value=(*c,1)
  if metal is not None:p.inputs['Metallic'].default_value=metal
  if rough is not None:p.inputs['Roughness'].default_value=rough
  if alpha is not None:p.inputs['Alpha'].default_value=alpha
 m.diffuse_color=(*c,alpha if alpha is not None else 1)
tune('Plum Root Hair',(.19,.045,.085),.0,.45)
tune('Plum Highlight Hair',(.37,.13,.25),.03,.48)
tune('Lavender Hair Tips',(.57,.34,.48),.04,.51)
tune('Smoky Rose Wing Membranes',(.11,.045,.082),.11,.49,.89)
tune('Frosted Pearlescent Wing Veil',(.57,.44,.57),.17,.38,.78)
tune('Frosted Ivory Armor',(.68,.65,.70),.52,.31)
tune('Polished Silver Scale Tips',(.84,.84,.88),.59,.27)
tune('Smoky Graphite Armor',(.066,.058,.071),.46,.4)
tune('Aged Dark Lilac Steel',(.12,.088,.115),.61,.36)
tune('Amethyst Cosmic Void',(.008,.003,.017),.08,.2)
tune('Inner Violet Core',(.34,.11,.46),.10,.25)
# Hairstyle reference: heavy windswept plum locks framing but NEVER concealing eyes/face.
removed=0;relocated=0
for o in list(bpy.data.objects):
 name=o.name
 if o.type!='CURVE':continue
 if name.startswith('Flowing orchid hair'):
  # Too many reference-discordant straight wisps -> keep coherent 3-dimensional clumps.
  ix=int(name.rsplit('.',1)[-1])
  layer=int(name.rsplit('.',2)[-2])
  if layer in (2,3) and ix%2==0:
   o.hide_render=True;o.hide_viewport=True;removed+=1;continue
  for spline in o.data.splines:
   for point in spline.bezier_points:
    # Base hair source on side/back, free the central facial surface.
    point.co.y+=.17
    if point.co.z>1.46 and abs(point.co.x)<.175:
     point.co.x+=.08 if point.co.x>=0 else -.08
    relocated+=1
 if name.startswith('Cascade hair ribbon'):
  ix=int(name.split()[-1])
  if ix%3==0:
   o.hide_render=True;o.hide_viewport=True;removed+=1;continue
  for spline in o.data.splines:
   for pt in spline.bezier_points:pt.co.y+=.13;relocated+=1
 if name.startswith('Face silhouette fringe'):
  ix=int(name.split()[-1])
  if ix%3!=0:
   o.hide_render=True;o.hide_viewport=True;removed+=1;continue
  for spline in o.data.splines:
   for pt in spline.bezier_points:
    if pt.co.z<1.6:pt.co.z+=.11
    if pt.co.z<1.67:pt.co.y+=.052
# Lower wings should be overlapping large individual lamella-feathers, not a white wire brush.
for o in bpy.data.objects:
 if not o.name.startswith('L Secondary flight feather') and not o.name.startswith('R Secondary flight feather'):continue
 if o.type!='MESH':continue
 indices=o.name.split('feather ')[-1].split('.')
 if len(indices)!=2:continue
 layer=int(indices[0]);ix=int(indices[1])
 if layer>=3 or (layer==2 and ix%2==0):
  o.hide_render=True;o.hide_viewport=True;removed+=1
  continue
 mesh=o.data
 for r in range(len(mesh.vertices)//5):
  center=mesh.vertices[r*5+2].co.copy()
  for j in range(5):
   v=mesh.vertices[r*5+j]
   v.co=center+(v.co-center)*1.33
# Avoid tail looking like a trail of disconnected giant rocks.
for o in bpy.data.objects:
 if o.name.startswith('Tail bony segment') or o.name.startswith('Tail razor spines'):
  o.hide_render=True;o.hide_viewport=True;removed+=1
# Add continuous 3D tapering underlying tail tube, with dorsal crystalline ridge.
tail= bpy.data.collections['T05 | Rigged Scaled Dragon Tail']
coords=[(0,.18,.71),(.08,.27,.60),(.22,.30,.40),(.42,.33,.23),(.61,.31,.13),(.73,.29,.15),(.79,.28,.28),(.88,.29,.35),(1.05,.29,.26)]
cu=bpy.data.curves.new('Dragon tail: continuous armored vertebral tube','CURVE');cu.dimensions='3D'
cu.bevel_depth=.061;cu.resolution_u=18;cu.bevel_resolution=5
sp=cu.splines.new('BEZIER');sp.bezier_points.add(len(coords)-1)
for i,(pt,loc) in enumerate(zip(sp.bezier_points,coords)):
 pt.co=loc;pt.handle_left_type='AUTO';pt.handle_right_type='AUTO';pt.radius=max(.12,1-i/(len(coords)-1))
o=bpy.data.objects.new('TEST05 continuous curved tail anatomy',cu);tail.objects.link(o)
cu.materials.append(mat('Smoky Graphite Armor'))
for name in ('Soft Key + Facial Detail','Silver Wing Rim','Rose Hair Bounce','Warm Gold Facets'):
 if name in bpy.data.objects:
  ob=bpy.data.objects[name];ob.data.energy*=.19
scene=bpy.context.scene
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.42
scene.view_settings.exposure=-.50
scene.camera.data.ortho_scale=3.05
scene.render.resolution_percentage=67
scene.render.filepath=str(root/'TEST05_PlumCrystalDragon_ArtPass_v3.png')
destination=root/'TEST05_CrystallineDragonSeraph_Editable_v3.blend'
if destination.exists():raise FileExistsError(destination)
bpy.ops.wm.save_as_mainfile(filepath=str(destination))
receipt={'status':'ART_PASS_PENDING_VISUAL_REVIEW','v3':str(destination),
  'objects_hidden_or_simplified':removed,'hair_control_points_repositioned':relocated,
  'native_body_original_rig_and_shape_keys_preserved':True,
  'old_scenes_unchanged':True,'output_image':scene.render.filepath,'jewelry_omitted':True}
(root/'TEST05_ART_PASS_v3_receipt.json').write_text(json.dumps(receipt,indent=2))
print('TEST05_ART_PASS_SAVED',destination,'hidden',removed,'hair_points',relocated,flush=True)
bpy.ops.render.render(write_still=True)
print('TEST05_ART_PASS_RENDERED',scene.render.filepath,flush=True)
