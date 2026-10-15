"""Editable Test03 Blender pose and visual review; source remains untouched."""
import bpy,pathlib,math,json,time
from mathutils import Vector
t0=time.perf_counter()
out=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261016\TEST03_ORANGE_BRAID')
arms=[x for x in bpy.data.objects if x.type=='ARMATURE']
if len(arms)!=2:raise RuntimeError('Both original KKBP rigs required')
for rig in arms:
 poses={
  'cf_j_spine01':('X',.12),'cf_j_spine02':('X',.12),
  'cf_j_arm00_L':('Y',-1.38),'cf_j_arm00_R':('Y',1.20),
  'cf_j_forearm01_L':('X',.26),'cf_j_forearm01_R':('X',-.26),
  'cf_j_hand_L':('Z',-.12),'cf_j_hand_R':('Z',.12)}
 for name,(axis,angle) in poses.items():
  b=rig.pose.bones.get(name)
  if b:
   for con in b.constraints:con.mute=True
   b.rotation_mode='XYZ';b.rotation_euler['XYZ'.index(axis)]=angle
mesh=bpy.data.objects.get('model_mesh')
expr=[]
if mesh and mesh.data.shape_keys:
 for name,weight in [('kuti_face.f00_bisyou_op',.6),('eye_face.f00_egao_op',.16)]:
  b=mesh.data.shape_keys.key_blocks.get(name)
  if b:b.value=weight;expr.append(name)
# Camera from front: Blender coordinates +Z up, -Y forward.
cam_data=bpy.data.cameras.new('Test03_Inspection_Camera')
cam=bpy.data.objects.new('Test03_Inspection_Camera',cam_data)
bpy.context.collection.objects.link(cam)
cam.location=(0,-.36,.085)
direction=Vector((0,0,.081))-cam.location
cam.rotation_euler=direction.to_track_quat('-Z','Y').to_euler()
cam_data.type='ORTHO';cam_data.ortho_scale=.165
scene=bpy.context.scene
scene.camera=cam
scene.render.engine='BLENDER_WORKBENCH'
scene.display.shading.light='STUDIO'
scene.display.shading.color_type='MATERIAL'
scene.display.shading.show_cavity=True
for m in bpy.data.materials:
 n=m.name.lower()
 if 'extension.braid' in n or 'hair' in n:col=(.98,.36,.045,1)
 elif 'eye' in n or 'hitomi' in n or 'pupil' in n:col=(.14,.52,.96,1)
 elif 'face' in n or 'body' in n or 'skin' in n:col=(.94,.735,.61,1)
 elif 'shirtdress' in n or 'top' in n or 'socks' in n or 'panst' in n:col=(.98,.98,.99,1)
 elif 'metal' in n:col=(.76,.78,.82,1)
 else:col=(.9,.85,.82,1)
 m.diffuse_color=col
scene.render.resolution_x=780;scene.render.resolution_y=1100
scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.render.film_transparent=False
if scene.world is None:scene.world=bpy.data.worlds.new('Test03_NeutralStudio')
scene.world.color=(.18,.20,.23)
scene.render.filepath=str(out/'Test03_ShirtPose_BlenderReview.png')
for name,position,power,size in [
 ('KeyLight',(-.10,-.10,.24),200,0.22),
 ('FillLight',(.18,-.04,.14),120,0.24),
 ('RimLight',(0,.13,.22),250,0.20)]:
 obj=bpy.data.objects.new(name,bpy.data.lights.new(name,'AREA'))
 bpy.context.collection.objects.link(obj)
 obj.location=position;obj.data.energy=power;obj.data.shape='DISK';obj.data.size=size
 obj.rotation_euler=(Vector((0,0,.08))-obj.location).to_track_quat('-Z','Y').to_euler()
scene.render.image_settings.color_mode='RGBA'
blend=out/'Test03_OrangeBraid_ShirtPolished_Posed_Editable.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(blend))
bpy.ops.render.render(write_still=True)
status={'schema':'NW_TEST03_BLENDER_POSE_REVIEW_V1','status':'PASS_EDITABLE_POSE_AND_RENDER',
 'arms':len(arms),'face_morphs_enabled':expr,'blender':str(blend),
 'render':scene.render.filepath,'seconds':round(time.perf_counter()-t0,2),
 'reference_fidelity_approved':False}
(out/'PosePreviewReceipt.json').write_text(json.dumps(status,indent=2))
print('TEST03_POSED_RENDER='+json.dumps(status),flush=True)
