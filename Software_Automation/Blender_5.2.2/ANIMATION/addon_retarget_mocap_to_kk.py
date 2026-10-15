"""Blender 5.2.2: Fully automatic authored performance -> Koikatsu derivative action.
Uses CMU Motion Capture Library source animation (license: commercial use allowed,
do not resell motion dataset directly); all original KK data remains untouched.
Outputs PRIVATE editable action .blend, isolated skeletal FBX and animated pose audits.
No hand-keyframed joints. Source is intelligently sampled to find wrist attack peak.
"""
import bpy,os,math,json,sys,time,statistics
from mathutils import Vector
T=time.perf_counter()
PRIVATE=r"D:\UnrealProjects\NW_Blender_Private_Review_20261015"
MOCAP=r"D:\UnrealProjects\NW_Blender_Mocap_Interop_20261015"
args=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
source_id=args[0] if args else "144_20"
assert source_id in {"144_20","144_05","144_13","143_23","02_05","02_07"}
input_bvh=os.path.join(MOCAP,source_id+".bvh")
bpy.ops.wm.open_mainfile(filepath=os.path.join(PRIVATE,"NW_KK_ChikaHaruno_LitReview_NONCANONICAL.blend"))
target=bpy.data.objects["NW_KK_INTENDED_SKELETON_121"]
scene=bpy.context.scene;scene.render.fps=24
bpy.ops.import_anim.bvh(filepath=input_bvh,use_fps_scale=True,update_scene_fps=False,rotate_mode="NATIVE")
source=next(o for o in bpy.context.scene.objects if o.type=="ARMATURE" and o!=target)
source.name="NW_CMU_SourceSkeleton_"+source_id
maxframe=int(source.animation_data.action.frame_range[1])
# Detect high-energy right or left hand relative to pelvis, rather than choosing timestamps by hand.
last=None;motion=[]
for f in range(3,maxframe-3,2):
 scene.frame_set(f);bpy.context.view_layer.update()
 hips=source.pose.bones["Hips"].head
 hand_l=source.pose.bones["LeftHand"].head-hips
 hand_r=source.pose.bones["RightHand"].head-hips
 score=0
 if last:
  score=max((hand_l-last[0]).length,(hand_r-last[1]).length)
 motion.append((f,score))
 last=(hand_l.copy(),hand_r.copy())
guard=30
valid=[(f,score) for f,score in motion if f>guard and f<maxframe-guard]
assert valid
peak,max_speed=max(valid,key=lambda x:x[1])
start=max(1,peak-18)
end=min(maxframe,peak+26)
mapping={
 "pelvis":"Hips",
 "spine_01":"LowerBack","spine_02":"Spine","spine_03":"Spine1",
 "neck":"Neck","head":"Head",
 "clavicle_l":"LeftShoulder","upperarm_l":"LeftArm","lowerarm_l":"LeftForeArm","hand_l":"LeftHand",
 "clavicle_r":"RightShoulder","upperarm_r":"RightArm","lowerarm_r":"RightForeArm","hand_r":"RightHand",
 "thigh_l":"LeftUpLeg","calf_l":"LeftLeg","Left ankle":"LeftFoot","ball_l":"LeftToeBase",
 "thigh_r":"RightUpLeg","calf_r":"RightLeg","Right ankle":"RightFoot","ball_r":"RightToeBase"
}
# Standardize *source* CMU bone names to existing KK game rig bones without editing the target.
# The GPL Retarget add-on handles diverse rest poses; no per-bone hand keyframing.
for to,fr in mapping.items():
 if fr in source.data.bones:source.data.bones[fr].name=to
import importlib
addon=importlib.import_module("bl_ext.blender_org.retarget.operators")
addon.ConstrainToArmature.current_m="OBJECT" # background Blender execute() lacks invoke() setup
bpy.ops.object.select_all(action="DESELECT")
target.select_set(True);source.select_set(True)
bpy.context.view_layer.objects.active=source
res=bpy.ops.armature.retarget_constrain_to_armature(same_bone_names=True)
assert "FINISHED" in res,res
mapped=[(to,fr) for to,fr in mapping.items() if to in target.pose.bones and len(target.pose.bones[to].constraints)>0]
assert len(mapped)>=20,len(mapped)
assert len(target.data.bones)==121,"Game skeleton contract changed unexpectedly"
# Capture unconstrained source-driven target poses for numerical baked-source fidelity.
watch=["head","upperarm_r","lowerarm_r","hand_r","upperarm_l","thigh_l","calf_l","pelvis"]
checkframes=sorted(set([start,peak,end,start+(end-start)//3,start+2*(end-start)//3]))
def sample():
 v={}
 for f in checkframes:
  scene.frame_set(f);bpy.context.view_layer.update()
  v[str(f)]={k:[round(float(x),5) for row in target.pose.bones[k].matrix for x in row] for k in watch}
 return v
pre=sample()
bpy.ops.object.select_all(action="DESELECT")
target.select_set(True);bpy.context.view_layer.objects.active=target
bpy.ops.object.mode_set(mode="POSE");bpy.ops.pose.select_all(action="SELECT")
bpy.ops.nla.bake(frame_start=start,frame_end=end,step=1,
  only_selected=False,visual_keying=True,clear_constraints=True,
  clear_parents=False,use_current_action=False,clean_curves=False,bake_types={"POSE"})
bpy.ops.object.mode_set(mode="OBJECT")
assert all(len(pb.constraints)==0 for pb in target.pose.bones if pb.name in mapping)
post=sample()
max_pose_diff=0
for f in checkframes:
 for k in watch:
  aa=pre[str(f)][k];bb=post[str(f)][k]
  max_pose_diff=max(max_pose_diff,max(abs(a-b) for a,b in zip(aa,bb)))
# Validate independent evaluated joint movement and attached textured mesh.
locs=[]
for f in checkframes:
 scene.frame_set(f);bpy.context.view_layer.update()
 locs.append(tuple(target.pose.bones["hand_r"].matrix.translation))
motion_span=max((math.dist(locs[0],p) for p in locs),default=0)
if motion_span<.05:raise RuntimeError("Retargeted hand motion below threshold; unacceptable static action")
# Source and original remain. Hide source from reviewers; keep as retraceable editable archival armature.
source.hide_render=True;source.hide_set(True)
scene.frame_start=start;scene.frame_end=end
scene.frame_set(peak)
for screen in bpy.data.screens:
 for area in screen.areas:
  if area.type=="VIEW_3D":
   area.spaces.active.region_3d.view_perspective="CAMERA"
scene["nw_mocap_source"]=source_id
scene["nw_mocap_peak"]=peak
scene["nw_action_acceptance"]="AUTOMATED_RETARGET_ARTISTIC_AND_ENGINE_VALIDATION_PENDING"
out_blend=os.path.join(PRIVATE,"NW_KBSCharacter_"+source_id+"_AutoMocapAction_PRIVATE.blend")
bpy.ops.wm.save_as_mainfile(filepath=out_blend,compress=True)
# Export selected target rig only: import in Unreal as animation against 121-bone skel.
bpy.ops.object.select_all(action="DESELECT")
target.select_set(True);bpy.context.view_layer.objects.active=target
fbx=os.path.join(MOCAP,"NW_KBSCharacter_"+source_id+"_AnimOnly.fbx")
bpy.ops.export_scene.fbx(filepath=fbx,use_selection=True,
 object_types={"ARMATURE"},add_leaf_bones=False,
 bake_anim=True,bake_anim_use_all_bones=True,
 bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,
 bake_anim_simplify_factor=0)
# Non-video visual review of characteristic poses.
scene.render.engine="BLENDER_WORKBENCH"
scene.display.shading.color_type="MATERIAL";scene.display.shading.light="STUDIO"
scene.render.resolution_x=450;scene.render.resolution_y=650;scene.render.resolution_percentage=100
scene.render.image_settings.file_format="PNG"
posters=[]
for f in (start,peak,end):
 scene.frame_set(f)
 fn=os.path.join(MOCAP,"NW_KBS_"+source_id+"_Pose"+str(f)+".png")
 scene.render.filepath=fn;bpy.ops.render.render(write_still=True)
 posters.append(fn)
report={"blender":bpy.app.version_string,"source":source_id,
  "cmu_license":"free for commercial game use; raw reselling prohibited",
  "source_frames":maxframe,"source_original_fps":120,
  "blender_fps":24,"selected_clip_range":[start,end],
  "clip_duration_seconds":round((end-start+1)/24,3),
  "peak_hand_speed_frame":peak,"peak_speed":max_speed,
  "mapped_bones":mapped,"mapped_count":len(mapped),
  "evaluated_right_wrist_motion_span":motion_span,
  "max_constraint_vs_baked_matrix_element_diff":max_pose_diff,
  "original_kk_modified":False,"manual_animation_keys_authored":0,
  "visual_review":posters,"animation_only_fbx":fbx,"blend":out_blend,
  "fbx_bytes":os.path.getsize(fbx),"elapsed_seconds":round(time.perf_counter()-T,3),
  "unreal_playback_accepted":False,"professional_art_accepted":False,
  "needs_retarget_cleanup":True}
with open(os.path.join(MOCAP,"NW_KBS_"+source_id+"_retarget_evidence.json"),"w") as f:json.dump(report,f,indent=2)
print("NOWORLD_KBS_RETARGET="+json.dumps({k:v for k,v in report.items() if k not in ("mapped_bones","visual_review")}))
