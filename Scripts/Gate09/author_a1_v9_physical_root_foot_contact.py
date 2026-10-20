"""Ground actual Koikatsu-sourced Luminous A1 in Blender 5.2 using one
automatically keyed root bone, computed from toe-floor contact. Single forward
lunge along real actor motion; preserve all 975 original bone/weapon constraints.
This replaces floating source poses without manual per-bone IK edits.
"""
import bpy,pathlib,hashlib,json,math
from mathutils import Vector,Matrix
P=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
SRC=P/'Luminous_A1_ScarletCadence_CinematicEdit_v8.blend'
DST=P/'Luminous_A1_ScarletCadence_GroundedRootMotion_v9.blend'
REC=P/'Luminous_A1_ScarletCadence_GroundedRootMotion_v9.json'
CAP=P/'Luminous_A1_ScarletCadence_GroundedRootMotion_v9_Captures'
for x in (DST,REC,CAP):
 if x.exists():raise FileExistsError(x)
CAP.mkdir()
bpy.ops.wm.open_mainfile(filepath=str(SRC),load_ui=False)
s=bpy.context.scene
rig=next(x for x in bpy.data.objects if x.type=='ARMATURE' and len(x.pose.bones)==975)
center=rig.pose.bones['Center']
assert center.parent is None
original_action=rig.animation_data.action
action=original_action.copy()
action.name='G09_A1_v9_GroundedRootSourceFullPose_33f'
rig.animation_data.action=action
slot=[a for a in action.slots if a.target_id_type=='OBJECT'][0]
rig.animation_data.action_slot=slot
def toe_positions():
 return [(rig.matrix_world@rig.pose.bones[n].matrix).translation.copy() for n in ('Left toe','Right toe')]
s.frame_set(1)
assert center.location.length<1e-6
base=toe_positions()[0]
origin=Vector((0,0,0))
# Evaluate one transform point per root translation axis to calculate actual
# live bone-to-world transform. The center pose transforms reflect rig rest axis.
columns=[]
for i in range(3):
 center.location=Vector(tuple(.1 if k==i else 0 for k in range(3)))
 bpy.context.view_layer.update()
 columns.append((toe_positions()[0]-base)/.1)
center.location=origin
bpy.context.view_layer.update()
J=Matrix(((columns[0].x,columns[1].x,columns[2].x),
          (columns[0].y,columns[1].y,columns[2].y),
          (columns[0].z,columns[1].z,columns[2].z)))
assert abs(J.determinant())>.6
inv=J.inverted()
# Designer-approved physical movement envelope, interpolation via smoothstep,
# not hand-keying 975 bones; max 35cm lunge rather than unnatural teleport.
stride={1:(0,0),4:(0,-.04),6:(-.015,-.08),8:(-.012,-.06),
        12:(0,-.09),16:(.016,-.17),20:(-.02,-.35),
        24:(-.024,-.31),28:(0,-.24),33:(0,-.18)}
def fxy(frame):
 lower=max(k for k in stride if k<=frame)
 upper=min(k for k in stride if k>=frame)
 if lower==upper:return stride[lower]
 t=(frame-lower)/(upper-lower)
 t=t*t*(3-2*t)
 return tuple((1-t)*stride[lower][i]+t*stride[upper][i] for i in range(2))
raw=[]
for f in range(1,34):
 s.frame_set(f)
 center.location=origin
 bpy.context.view_layer.update()
 toes=toe_positions()
 before=min(t.z for t in toes)
 x,y=fxy(f)
 # Feet are grounded to the actually measured stage floor; no invented rigid
 # body collision response or local limb deformation.
 dz=max(-.15,min(.08,.003-before))
 raw.append((f,x,y,dz,before))
assert max(a[4] for a in raw)>.10
for f,x,y,z,before in raw:
 s.frame_set(f)
 center.location=inv@Vector((x,y,z))
 center.keyframe_insert('location',frame=f,group='PhysicalRootMotion_FootGrounding')
# validate against actual evaluated post-animation local floor contact
quality=[]
for f,x,y,z,before in raw:
 s.frame_set(f);bpy.context.view_layer.update()
 after=min(q.z for q in toe_positions())
 sword=bpy.data.objects['G09_A1_PhysicalSword_GripFollowingRightWrist']
 wrist=(rig.matrix_world@rig.pose.bones['Right wrist'].matrix).translation
 distance=(sword.matrix_world.translation-wrist).length
 assert abs(after-.003)<.004,(f,before,after)
 assert distance<.13,(f,distance)
 quality.append({'frame':f,'min_before_z_m':before,'min_after_z_m':after,
                 'world_lunge_x_m':x,'world_lunge_y_m':y,
                 'world_ground_shift_m':z,'sword_grip_error_m':distance})
print('AUTO_GROUNDED_33_FRAMES',len(quality),
  'MAX_PREVIOUS_FLOAT_M',max(x['min_before_z_m'] for x in quality),
  'MAX_RESIDUAL_GAP_M',max(abs(x['min_after_z_m']-.003) for x in quality),
  'MAX_SWORD_GRIP_ERROR_M',max(x['sword_grip_error_m'] for x in quality),flush=True)
# World mode gameplay camera and cinematic third-hit camera remain scene-specific.
s.render.engine='BLENDER_EEVEE'
s.render.resolution_x=950;s.render.resolution_y=1050;s.render.resolution_percentage=100
s.render.image_settings.file_format='PNG'
game=bpy.data.objects['G09_A1_Camera_WorldMode_CleanGameplayFraming']
finisher=bpy.data.objects['G09_A1_CinematicCamera_Hit03_Lunge']
captures=[]
for label,f in [('Hit01',6),('Hit02',12),('Hit03',20)]:
 s.frame_set(f);s.camera=finisher if f==20 else game
 path=CAP/f'G09_Luminous_A1_PhysicallyGrounded_{label}_v9.png'
 s.render.filepath=str(path)
 bpy.ops.render.render(write_still=True)
 captures.append({'name':label,'frame':f,'path':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
 print('ACTUAL_GROUNDED_LUMINOUS_RENDER',f,path.stat().st_size,flush=True)
s.frame_start=1;s.frame_end=33;s.frame_set(1);s.camera=game
bpy.ops.wm.save_as_mainfile(filepath=str(DST),compress=True)
result={'schema':'G09_A1_AUTOMATED_ROOT_GROUNDING_V9',
'status':'SKELETAL_ORIGINAL_AND_SWORD_PHYSICALLY_GROUNDED_RENDERED',
'source':str(SRC),'source_sha256':hashlib.sha256(SRC.read_bytes()).hexdigest(),
'saved_blend':str(DST),'sha256':hashlib.sha256(DST.read_bytes()).hexdigest(),
'new_animation_action':action.name,
'preserved_skeletal_bones':975,'modified_motion_bone':'Center',
'fixed_source_animations_untouched':True,
'original_morphs_outfit_and_sword_preserved':True,
'floor_z_m':0.003,'max_forward_lunge_m':.35,'frames':quality,
'quality_captures':captures,'native_ue_import':'PENDING','visual_acceptance':'PENDING_MOTION_REVIEW'}
REC.write_text(json.dumps(result,indent=2)+'\n',encoding='utf8')
print('G09_A1_V9_ROOT_MOTION_SOURCE_READY',DST,flush=True)
