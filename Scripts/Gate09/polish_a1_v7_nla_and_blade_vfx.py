"""Blender 5.2 A1 v7: original-kit three-contact choreography via full-pose
NLA-style quaternion interpolation; re-time real source thrust at exact hit frame.
Sword stays rigidly bone-parented, VFX authored IN BLADE LOCAL SPACE:
no detached white static bars, no per-bone hand reposition.
"""
import bpy,pathlib,math,json,hashlib
from mathutils import Vector
P=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
S=P/'Luminous_A1_ScarletCadence_ConstraintDriven_v6.blend'
B=P/'Luminous_A1_ScarletCadence_ConstraintDriven_v7.blend'
R=P/'Luminous_A1_ScarletCadence_ConstraintDriven_v7.json'
D=P/'Luminous_A1_ConstraintDriven_v7_ContactRenders'
for x in (B,R,D):
 if x.exists():raise FileExistsError(x)
D.mkdir()
bpy.ops.wm.open_mainfile(filepath=str(S),load_ui=False)
s=bpy.context.scene
rig=next(x for x in bpy.data.objects if x.type=='ARMATURE' and len(x.pose.bones)==975)
sword=bpy.data.objects['G09_A1_PhysicalSword_GripFollowingRightWrist']
assert sword.parent==rig and sword.parent_type=='BONE' and sword.parent_bone=='Right wrist'
bones=[p.name for p in rig.pose.bones if p.rotation_mode=='QUATERNION']
old=rig.animation_data.action;cache={}
for f in range(1,34):
 s.frame_set(f);cache[f]={n:rig.pose.bones[n].rotation_quaternion.copy() for n in bones}
active=[n for n in bones if any(cache[f][n].rotation_difference(cache[1][n]).angle>.0005 for f in range(2,34))]
# Choose contact 3 authored source pose from later true Thrust Slash action
# (previous v6 source frame24) to physically extend blade at the actual 650ms hit.
anchor={1:1,6:6,12:12,20:24,33:33}
new=bpy.data.actions.new('G09_A1_V7_NLAStyle_Interpolated_DharmaOriginalSources')
slot=new.slots.new('OBJECT',rig.name)
rig.animation_data.action=new;rig.animation_data.action_slot=slot
for f in range(1,34):
 lo=max(i for i in anchor if i<=f)
 hi=min(i for i in anchor if i>=f)
 ratio=(f-lo)/(hi-lo) if hi>lo else 0
 # Distinct biomechanical rhythm: direct start/finish at exact contact frames;
 # ease after initial clash, accelerate toward next attack.
 ease=ratio*ratio*(3-2*ratio) if lo>=20 else ratio**1.40
 for n in active:
  a=cache[anchor[lo]][n];b=cache[anchor[hi]][n]
  if a.dot(b)<0:b=-b
  calculated=a.slerp(b,ease)
  # 12% native center-clip nuance remains, but each hit anchor is exact.
  if f not in anchor:
   orig=cache[f][n]
   if calculated.dot(orig)<0:orig=-orig
   calculated=calculated.slerp(orig,.12*math.sin(math.pi*ratio))
  pb=rig.pose.bones[n];pb.rotation_quaternion=calculated
  pb.keyframe_insert('rotation_quaternion',frame=f,group='Blender5_BatchNLAPoseBlends')
print('V7_SOURCE_POSES_CURVE_AUTO_INTERPOLATED',len(active),
 'CONTACT_SOURCE_ANCHORS',anchor,flush=True)
# Remove old scene-space tip ribbons. They did NOT follow the sword and failed visual review.
removed=[]
for ob in list(bpy.data.objects):
 if ob.name.startswith('G09_A1_RealBlade_Contact'):
  removed.append(ob.name);bpy.data.objects.remove(ob,do_unlink=True)
assert len(removed)==6,len(removed)
print('REMOVED_DETACHED_TIP_VFX',len(removed),flush=True)
# Original steel/crimson trail shaders, toned down and attached to real source equipment.
silver=bpy.data.materials['G09_A1_SilverSteel_SwordTrail'].copy()
silver.name='G09_A1_V7_SilverBladeEdge_Transparent'
bs=silver.node_tree.nodes.get('Principled BSDF')
bs.inputs['Emission Strength'].default_value=.45
bs.inputs['Roughness'].default_value=.27
bs.inputs['Alpha'].default_value=.53
silver.surface_render_method='DITHERED'
red=bpy.data.materials['G09_A1_Crimson_BladeTracery'].copy()
red.name='G09_A1_V7_CrimsonBladeEnchantment'
br=red.node_tree.nodes.get('Principled BSDF')
br.inputs['Emission Strength'].default_value=.62
br.inputs['Alpha'].default_value=.66
red.surface_render_method='DITHERED'
# Dynamic blade edge aura is in equipment's LOCAL COORDINATES,
# so bone parenting guarantees zero floating gap during fast contacts.
def ribbon(name,x,width,mat):
 vs=[];faces=[]
 for k in range(27):
  t=k/26;z=.35+.77*t
  # subtly follow blade geometry, fine tapered luminance; length 0.77m.
  xk=x+.016*math.sin(math.pi*t)*math.sin(2*math.pi*t)
  w=width*(.12+.88*math.sin(math.pi*t)**.58)
  vs.extend([(xk-w/2,-.012,z),(xk+w/2,-.012,z)])
 for k in range(26):faces.append((2*k,2*k+1,2*k+3,2*k+2))
 mesh=bpy.data.meshes.new(name+'_blade_hugging_mesh')
 mesh.from_pydata(vs,[],faces);mesh.update();mesh.materials.append(mat)
 ob=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(ob)
 ob.parent=sword;ob.matrix_parent_inverse.identity()
 return ob
for hi,frame in enumerate((6,12,20),1):
 for label,x,width,material in (
  ('Silver_BladeEdge',.035,.020,silver),('Crimson_EdgeTracery',-.035,.012,red)):
  ob=ribbon('G09_A1_V7_Hit%02d_%s'%(hi,label),x,width,material)
  for f,visible in [(1,False),(frame-2,False),(frame-1,True),
                    (frame+1,True),(frame+2,False),(33,False)]:
   ob.hide_render=not visible;ob.keyframe_insert('hide_render',frame=f,
    group='ContactVerifiedBladeEdgeEvent')
   ob.hide_viewport=not visible;ob.keyframe_insert('hide_viewport',frame=f,
    group='ContactVerifiedBladeEdgeEvent')
# Cinematic camera: maintain enough space for raised silver Gothic sword.
camera=s.camera;camera.data.type='ORTHO'
camera.data.ortho_scale=2.95
camera.location=Vector((1.25,-4.05,1.77))
camera.rotation_euler=(Vector((0,0,.90))-camera.location).to_track_quat('-Z','Y').to_euler()
s.render.engine='BLENDER_EEVEE'
s.render.resolution_x=1300;s.render.resolution_y=1450;s.render.resolution_percentage=100
s.render.image_settings.file_format='PNG'
s.frame_start=1;s.frame_end=33
qa=[]
for label,f in [('Hit01_Diagonal',6),('Hit02_Return',12),('Hit03_Thrust',20)]:
 s.frame_set(f);bpy.context.view_layer.update()
 wrist=(rig.matrix_world@rig.pose.bones['Right wrist'].matrix).translation
 grip=sword.matrix_world.translation
 distance=(grip-wrist).length
 assert distance<.13,(f,distance)
 dest=D/('G09_Luminous_A1_'+label+'_v7.png')
 s.render.filepath=str(dest)
 bpy.ops.render.render(write_still=True)
 qa.append({'label':label,'frame':f,'grip_distance_m':distance,'file':str(dest),
            'sha256':hashlib.sha256(dest.read_bytes()).hexdigest(),'bytes':dest.stat().st_size})
 print('ACTUAL_LUMINOUS_A1_V7_CLEAN_RENDER',label,'bytes',dest.stat().st_size,'grip_m',round(distance,3),flush=True)
s.frame_set(12)
bpy.ops.wm.save_as_mainfile(filepath=str(B),compress=True)
receipt={'schema':'G09_LUMINOUS_A1_NLA_AUTOMATED_WEAPON_FX_V7',
 'status':'SOURCE_MOTION_RENDERED_NATIVE_UE_NOT_YET_VALIDATED',
 'source_sha256':hashlib.sha256(S.read_bytes()).hexdigest(),
 'new_blend_sha256':hashlib.sha256(B.read_bytes()).hexdigest(),
 'source_real_motion_retime_anchor_frames':anchor,
 'native_hit_times_ms':[180,390,650],
 'automated_bone_curve_count':len(active),
 'actor_skeleton_original_bones':975,'sword_parent_bone':'Right wrist',
 'detached_trail_meshes_removed':len(removed),
 'blade_hugging_contact_materials':2,'contact_renders':qa,
 'material_identity_preserved':True,'gate07_sword_item_authorization':'NEEDED_FOR_CANONICAL_EQUIPMENT'}
R.write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf8')
print('G09_V7_ACTION_VISUAL_SOURCE',B,hashlib.sha256(B.read_bytes()).hexdigest(),flush=True)
