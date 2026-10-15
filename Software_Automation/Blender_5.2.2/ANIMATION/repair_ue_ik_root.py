"""Post-UE IK retarget visual fidelity repair: eliminate pathological millions-unit
pelvis translation while preserving engine-created per-bone animation.
Genuine source mocap -> Unreal 5.8.3 mapped 10 IK chains -> Blender 5.2 edited Action -> FBX.
Private character never added to Git or overwritten. Fails if bone action is static.
"""
import bpy,os,json,pathlib,math,hashlib
from mathutils import Vector
P=pathlib.Path(r"D:\UnrealProjects\NW_Blender_Private_Review_20261015")
Q=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015")
original=P/"NW_KK_ChikaHaruno_LitReview_NONCANONICAL.blend"
target=Q/"NW_KK_ChainMappedV2_ActualMotion.fbx"
derived=Q/"NW_KK_Mocap_RootNormalized_PRIVATE.blend"
clip=Q/"NW_KK_Mocap_RootNormalized_AnimOnly.fbx"
report=Q/"NW_KK_Mocap_RootNormalized.json"
if any(x.exists() for x in (derived,clip,report)):raise FileExistsError("Do not overwrite prior animation tests")
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
before=sha(original)
bpy.ops.wm.open_mainfile(filepath=str(original),load_ui=False)
rig=bpy.data.objects["NW_KK_INTENDED_SKELETON_121"]
scene=bpy.context.scene;scene.render.fps=24
bpy.ops.import_scene.fbx(filepath=str(target))
source=next(o for o in scene.objects if o.type=="ARMATURE" and o!=rig)
anim=source.animation_data
assert anim and anim.action and anim.action_slot
action=anim.action.copy();action.name="NW_FemalePunch_CMU_IK10_RootCorrected"
slot=action.slots[0]
bag=action.layers[0].strips[0].channelbag(slot)
assert bag
root_curves=[fc for fc in bag.fcurves if 'pose.bones["pelvis"].location' in fc.data_path]
assert len(root_curves)==3,len(root_curves)
before_root=[]
for fc in root_curves:
 if len(fc.keyframe_points)<2:raise RuntimeError("Pelvis curve static")
 values=[float(x.co.y) for x in fc.keyframe_points]
 before_root.append(max(values)-min(values))
orig_range=max(before_root)
assert orig_range>1000,orig_range
# Rig-size-relative root-motion normalization, cap one action to 1/3 character height,
# preserve captured root trajectory SHAPE, no hand-authored bone keys.
meshlist=[o for o in bpy.data.objects if o.type=="MESH"]
allpoints=[]
for o in meshlist:
 for corner in o.bound_box:allpoints.append(o.matrix_world@Vector(corner))
mins=[min(p[i] for p in allpoints) for i in range(3)]
maxs=[max(p[i] for p in allpoints) for i in range(3)]
char_height=max(.2,maxs[2]-mins[2])
scale=(char_height*.32)/orig_range
for fc in root_curves:
 baseline=fc.evaluate(1)
 for k in fc.keyframe_points:
  k.co.y=(k.co.y-baseline)*scale
  k.handle_left.y=(k.handle_left.y-baseline)*scale
  k.handle_right.y=(k.handle_right.y-baseline)*scale
 fc.update()
rig.animation_data_create()
rig.animation_data.action=action;rig.animation_data.action_slot=slot
source.hide_render=True;source.hide_set(True)
scene.frame_start=1;scene.frame_end=44
watch=("pelvis","upperarm_r","lowerarm_r","hand_r","head","thigh_l")
poses={}
for f in (1,12,23,34,44):
 scene.frame_set(f);bpy.context.view_layer.update()
 poses[f]={k:tuple(round(v,6) for v in rig.pose.bones[k].matrix.to_translation()) for k in watch}
def delt(b):return max(math.dist(poses[1][b],poses[f][b]) for f in poses if f!=1)
deltas={n:delt(n) for n in watch}
assert deltas["hand_r"]>0.05 and deltas["upperarm_r"]>.03,deltas
# Material retaining private review source, cinematic pose QA source native action editable.
camera=bpy.data.cameras.new("NW_AutoFixedPoseCamera")
view=bpy.data.objects.new("NW_AutoFixedPoseCamera",camera);scene.collection.objects.link(view)
center=Vector((sum(v)/2 for v in zip(mins,maxs)))
# -Y front view, +Z tilt, calibrated to measured character bounds rather than arbitrary camera.
span=max(maxs[2]-mins[2],maxs[0]-mins[0],.5)
view.location=center+Vector((0,-span*2.5,span*.25))
view.rotation_euler=(center-view.location).to_track_quat("-Z","Y").to_euler()
camera.type="ORTHO";camera.ortho_scale=span*1.45;scene.camera=view
scene.render.engine="BLENDER_WORKBENCH"
scene.display.shading.color_type="MATERIAL"
scene.render.resolution_x=580;scene.render.resolution_y=720;scene.render.resolution_percentage=100
scene.render.image_settings.file_format="PNG"
pics=[]
for f in (1,23,44):
 scene.frame_set(f)
 p=Q/("NW_KK_MocapRootFixed_Frame%02d.png"%f)
 scene.render.filepath=str(p);bpy.ops.render.render(write_still=True)
 pics.append(str(p))
scene.frame_set(23)
bpy.ops.wm.save_as_mainfile(filepath=str(derived),compress=True)
bpy.ops.object.select_all(action="DESELECT")
rig.select_set(True);bpy.context.view_layer.objects.active=rig
bpy.ops.export_scene.fbx(filepath=str(clip),use_selection=True,object_types={"ARMATURE"},
 add_leaf_bones=False,bake_anim=True,bake_anim_use_all_bones=True,
 bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_simplify_factor=0)
assert sha(original)==before
res={"action":action.name,"source_action":"CMU female punch native UE5.8 IK retarget",
 "mapped_bone_rig_count":len(rig.data.bones),"frame_range":[1,44],"fps":24,
 "source_unusable_root_max_units":orig_range,
 "normalized_root_travel_max_units":max(before_root)*scale,
 "measured_character_height_units":char_height,
 "bone_joint_motion_span":deltas,"sampled_frames":poses,
 "material_original_unchanged":True,"source_hash_unchanged":True,
 "editable_private_source":str(derived),"private_clip":str(clip),
 "pose_images":pics,"native_unreal_reimported":False,
 "professional_motion_accepted":False}
report.write_text(json.dumps(res,indent=2))
print("NOWORLD_RETARGET_ROOT_REPAIRED="+json.dumps({k:v for k,v in res.items() if k not in ("sampled_frames","pose_images")}))
