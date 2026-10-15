"""Automated clip segmentation using a measured mocap strike peak.
No manually set bone rotations or poses; edits a local action copy from the original CMU BVH.
Output a 1.875-second 24fps reusable animation-only source FBX.
"""
import bpy,os,json,time
from mathutils import Vector
M=r"D:\UnrealProjects\NW_Blender_Mocap_Interop_20261015"
T=time.perf_counter()
bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene;scene.render.fps=24
bpy.ops.import_anim.bvh(filepath=os.path.join(M,"144_20.bvh"),update_scene_fps=False,use_fps_scale=True)
rig=next(o for o in scene.objects if o.type=="ARMATURE")
action=rig.animation_data.action
slot=rig.animation_data.action_slot
assert slot
start,end=203,247
# Blender 5.2 layered animation actions, not old Action.fcurves.
assert action.layers and action.layers[0].strips
bag=action.layers[0].strips[0].channelbag(slot)
assert bag is not None
num=0
for fc in bag.fcurves:
 ks=fc.keyframe_points
 for idx in range(len(ks)-1,-1,-1):
  frame=float(ks[idx].co.x)
  if frame<start or frame>end:ks.remove(ks[idx],fast=True)
 for k in ks:
  k.co.x-=start-1
  k.handle_left.x-=start-1
  k.handle_right.x-=start-1
 num+=len(ks)
 fc.update()
assert num>30,num
scene.frame_start=1;scene.frame_end=end-start+1
scene.frame_set(1)
bpy.ops.object.select_all(action="DESELECT")
rig.select_set(True);bpy.context.view_layer.objects.active=rig
fbx=os.path.join(M,"NW_CMU_Punch_1p875sec_AnimOnly.fbx")
bpy.ops.export_scene.fbx(filepath=fbx,use_selection=True,object_types={'ARMATURE'},
 add_leaf_bones=False,bake_anim=True,bake_anim_use_all_bones=True,
 bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_simplify_factor=0)
# Reimport into fresh scene and inspect count and native Action Slot, rotations at 1,23,45
bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=fbx)
rr=next(o for o in bpy.context.scene.objects if o.type=="ARMATURE")
if not (rr.animation_data and rr.animation_data.action):
 candidate=next(a for a in bpy.data.actions if a!=action and a.slots)
 rr.animation_data_create();rr.animation_data.action=candidate
 rr.animation_data.action_slot=candidate.slots[0]
samples=[]
for f in (1,23,45):
 scene.frame_set(f);bpy.context.view_layer.update()
 samples.append([round(x,5) for x in rr.pose.bones["RightArm"].matrix.to_quaternion()])
rotation_motion=max(sum(abs(a-b) for a,b in zip(samples[0],s)) for s in samples[1:])
r={"blender":bpy.app.version_string,"source":"144_20 BVH","recording_license":"CMU allows commercial use; no raw resale",
 "source_frames_selected":[start,end],"target_frames":[1,45],"fps":24,
 "action_5_2_keyframes_remaining":num,"reimport_bones":len(rr.data.bones),
 "reimport_has_action":bool(rr.animation_data and rr.animation_data.action),
 "rotation_difference":rotation_motion,"fbx":fbx,"bytes":os.path.getsize(fbx),
 "seconds":round(time.perf_counter()-T,3),
 "unreal_native_accepted":False,"artist_accepted":False}
with open(os.path.join(M,"NW_CMU_ShortPunch_SourceClip.json"),"w") as f:json.dump(r,f,indent=2)
print("NW_SHORT_CMUMOCAP="+json.dumps(r))
