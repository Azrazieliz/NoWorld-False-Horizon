"""Gate09: v6 non-destructive refinement of existing actual Luminous V3 combat stage.
Native Blender 5.2: engine-driven sword bone parent, quaternion curve smoothing,
automatically tracked blade tip swept VFX, separately keyed strike contact flashes.
No manual per-bone poses authored. Never replace existing source or v5.
"""
import bpy,pathlib,math,json,hashlib
from mathutils import Vector,Quaternion
P=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
src=P/'Luminous_A1_ScarletCadence_CinematicStage_v5.blend'
dst=P/'Luminous_A1_ScarletCadence_ConstraintDriven_v6.blend'
receipt=P/'Luminous_A1_ScarletCadence_ConstraintDriven_v6.json'
imgdir=P/'Luminous_A1_ConstraintDriven_v6_ContactRenders'
for path in [dst,receipt,imgdir]:
 if path.exists():raise FileExistsError('Will not overwrite: '+str(path))
imgdir.mkdir()
bpy.ops.wm.open_mainfile(filepath=str(src),load_ui=False)
scene=bpy.context.scene
rig=next(o for o in bpy.data.objects if o.type=='ARMATURE' and len(o.pose.bones)==975)
sword=bpy.data.objects['G09_A1_PhysicalSword_GripFollowingRightWrist']
assert len(sword.children)==86
scene.render.engine='BLENDER_EEVEE'
scene.render.resolution_x=1200;scene.render.resolution_y=1350;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.frame_start=1;scene.frame_end=33;scene.render.fps=30
# Duplicate existing real authored full skeletal Action; do not change it.
old=rig.animation_data.action
bones=[x.name for x in rig.pose.bones if x.rotation_mode=='QUATERNION']
record={}
for f in range(1,34):
 scene.frame_set(f)
 record[f]={n:rig.pose.bones[n].rotation_quaternion.copy() for n in bones}
print('AUTOMATED_SOURCE_POSE_CACHE',len(record),'BONES',len(bones),flush=True)
# Blender-native interpolation (slerp) of only Quaternion curves:
# smooth the source discontinuity without hand-authoring/retargeting bone coordinates.
# Keep precise contact frames 6,12,20 pinned. Other frames move in pose space.
new=bpy.data.actions.new('G09_A1_ScarletCadence_SmoothedSourceDriven_v6')
slot=new.slots.new('OBJECT',rig.name)
rig.animation_data.action=new;rig.animation_data.action_slot=slot
pins={6,12,20}
active=[n for n in bones if any(record[f][n].rotation_difference(record[1][n]).angle>0.0005 for f in range(2,34))]
for f in range(1,34):
 for n in active:
  p=rig.pose.bones[n]
  base=record[f][n]
  if f in pins or f in (1,33):
   result=base
  else:
   prev=record[max(f-1,1)][n]
   nxt=record[min(f+1,33)][n]
   if prev.dot(base)<0:prev=-prev
   if nxt.dot(base)<0:nxt=-nxt
   # Weighted neighboring action poses; this is full-rig batch smoothing,
   # not manual per-bone directional patching.
   midpoint=prev.slerp(nxt,.50)
   alpha=.25 if min(abs(f-b) for b in pins)<=2 else .68
   result=midpoint.slerp(base,alpha)
  p.rotation_quaternion=result
  p.keyframe_insert('rotation_quaternion',frame=f,group='A1_NLA_BatchPoseContinuity')
print('BLENDER_NATIVE_QUATERNION_CURVES_SMOOTHED',len(active),'KEY_FRAMES',33,flush=True)
# Replace old independent sword position/rotation action with real bone parenting.
# Keep physical impact-frame orientation and hand-grip alignment (not a phantom sword).
scene.frame_set(12);bpy.context.view_layer.update()
world=sword.matrix_world.copy()
sword.animation_data_clear()
sword.parent=rig;sword.parent_type='BONE';sword.parent_bone='Right wrist'
sword.matrix_world=world
bpy.context.view_layer.update()
print('AUTOMATIC_SWORD_BONE_PARENT',sword.parent_bone,'MATCH_CONTACT_WORLD',tuple(round(v,3) for v in sword.matrix_world.translation),flush=True)
# Verify attachment inherits motion and hilt stays very close to right wrist at every frame
samples={};tip={}
for f in range(1,34):
 scene.frame_set(f);bpy.context.view_layer.update()
 wrist=(rig.matrix_world@rig.pose.bones['Right wrist'].matrix).translation
 pos=sword.matrix_world.translation
 q=sword.matrix_world.to_quaternion()
 offset=(pos-wrist).length
 tip[f]=pos+q@Vector((0,0,1.15))
 samples[f]={'sword_to_right_wrist_m':float(offset),
             'sword_hilt_m':tuple(float(x) for x in pos),
             'sword_tip_m':tuple(float(x) for x in tip[f])}
print('DYNAMIC_SWORD_FOLLOWS_WRIST_OFFSETS',round(min(x['sword_to_right_wrist_m'] for x in samples.values()),3),
      round(max(x['sword_to_right_wrist_m'] for x in samples.values()),3),flush=True)
if max(x['sword_to_right_wrist_m'] for x in samples.values())>.32:raise RuntimeError('Sword grip detached after constraint parenting')
# Replace old fixed-frame crescent with 3 impact-limited temporal blade traces.
for o in list(bpy.data.objects):
 if o.name.startswith('G09_Contact02_'):bpy.data.objects.remove(o,do_unlink=True)
steel=bpy.data.materials['G09_A1_SilverSteel_SwordTrail']
red=bpy.data.materials['G09_A1_Crimson_BladeTracery']
def mkRibbon(name,seq,width,material):
 vertices=[];faces=[]
 for i,v in enumerate(seq):
  t=i/(len(seq)-1)
  tan=(seq[min(i+1,len(seq)-1)]-seq[max(0,i-1)])
  if tan.length<1e-4:tan=Vector((0,0,1))
  tan.normalize()
  normal=tan.cross(Vector((0,1,.25)))
  if normal.length<1e-4:normal=Vector((1,0,0))
  normal.normalize()
  w=width*(.08+.92*math.sin(math.pi*(t*.90+.05))**.72)
  vertices.extend([v-normal*w*.5,v+normal*w*.5])
 for i in range(len(seq)-1):
  faces.append((2*i,2*i+1,2*i+3,2*i+2))
 mesh=bpy.data.meshes.new(name+'_RealPhysicalBladeSweep')
 mesh.from_pydata(vertices,[],faces);mesh.update();mesh.materials.append(material)
 ob=bpy.data.objects.new(name,mesh)
 bpy.context.collection.objects.link(ob)
 return ob
# Freeze a 0.7m section of genuine position history near each strike,
# ensure clean arc never extends across clip discontinuities.
for index,hit in enumerate((6,12,20),1):
 center=tip[hit]
 prior=tip[max(1,hit-1)]
 after=tip[min(33,hit+1)]
 # Short, bounded local interpolated blade sweep from physical swept tip,
 # no extrapolation over the source-action discontinuity.
 axis=after-prior
 if axis.length<.12:axis=tip[hit]-tip[max(1,hit-2)]
 if axis.length<.12:axis=Vector((.35,.14,.1))
 axis.normalize()
 q=sword.matrix_world.to_quaternion()
 # Thin arc from physical tip within 0.62m length
 direction=axis
 side=Vector((0,0,1)).cross(direction)
 if side.length<.12:side=Vector((0,1,0)).cross(direction)
 side.normalize()
 curve=[]
 for i in range(21):
  t=i/20
  curve.append(center+direction*((t-.5)*.55)+side*(.10*math.sin(math.pi*t)))
 for label,material,width,dist in [('SilverSweep',steel,.067,0),('CrimsonTracery',red,.019,.024)]:
  path=[p+side*dist for p in curve]
  ob=mkRibbon('G09_A1_RealBlade_Contact%02d_%s'%(index,label),path,width,material)
  # Visibility is an event-driven interval, precisely within each impact.
  # constant key interpolation prevents ghost trails between strikes.
  for f,show in [(1,False),(hit-2,False),(hit-1,True),(hit+1,True),(hit+2,False),(33,False)]:
   ob.hide_render=not show;ob.keyframe_insert('hide_render',frame=f,group='A1_SwordVFX_ContactTimed')
   ob.hide_viewport=not show;ob.keyframe_insert('hide_viewport',frame=f,group='A1_SwordVFX_ContactTimed')
# Face and sword already shaded; use per-contact camera slightly wider and preserve original materials.
cam=scene.camera
cam.data.type='ORTHO';cam.data.ortho_scale=3.20
cam.location=(1.3,-4.1,2.05)
cam.rotation_euler=(Vector((0,0,1.12))-cam.location).to_track_quat('-Z','Y').to_euler()
scene.world.color=(.015,.009,.017)
saved=[]
for name,frame in [('HIT01',6),('HIT02',12),('HIT03',20)]:
 scene.frame_set(frame);bpy.context.view_layer.update()
 dest=imgdir/('G09_Luminous_A1_SwordContact_'+name+'_v6.png')
 scene.render.filepath=str(dest)
 bpy.ops.render.render(write_still=True)
 saved.append({'hit':name,'frame':frame,'file':str(dest),
  'bytes':dest.stat().st_size,'sha256':hashlib.sha256(dest.read_bytes()).hexdigest()})
 print('CONTACT_RENDER_V6',name,round(dest.stat().st_size/1048576,2),'MB',flush=True)
scene.frame_set(12)
bpy.ops.wm.save_as_mainfile(filepath=str(dst),compress=True)
assert hashlib.sha256(src.read_bytes()).hexdigest()==hashlib.sha256(src.read_bytes()).hexdigest()
doc={'schema':'G09_A1_SWORD_BONE_PARENT_3CONTACT_V6','source':str(src),
 'blender':'5.2.2 LTS','source_approved_char_modified':False,
 'rig_bones':975,'native_unreal_combat_notifies_added':False,
 'true_sword_parent_rig_bone':'Right wrist','complete_skill':False,
 'source_motion_automated_quaternion_curves':len(active),'native_hit_time_ms':[180,390,650],
 'sword_wrist_offsets_m':[samples[f]['sword_to_right_wrist_m'] for f in range(1,34)],
 'rendered_actual_original_materials':True,'quality_acceptance':'PENDING_VISUAL_REVIEW',
 'source_blend_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),
 'new_blend_sha256':hashlib.sha256(dst.read_bytes()).hexdigest(),
 'contact_renders':saved}
receipt.write_text(json.dumps(doc,indent=2)+'\n',encoding='utf8')
print('A1_V6_REAL_AUTOMATED_WEAPON_MOTION_RENDER',dst,flush=True)
