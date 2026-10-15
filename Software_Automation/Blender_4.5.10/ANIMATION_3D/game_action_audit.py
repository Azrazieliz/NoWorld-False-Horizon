"""Gatekeeper: distinguish reusable skeletal game actions from camera/object animation.
Native Blender 4.5.10 read-only pose analysis. No artistic acceptance from this tool.

Example:
 blender --background --factory-startup --python game_action_audit.py -- SOURCE.blend OUTPUT.json
"""
import bpy,sys,os,json,math
from mathutils import Vector
args=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
if len(args)!=2:raise SystemExit("Usage: -- SOURCE.blend RESULT.json")
source,result=map(os.path.abspath,args)
assert os.path.isfile(source),source
bpy.ops.wm.open_mainfile(filepath=source)
scene=bpy.context.scene
frames=sorted(set([int(round(scene.frame_start+(scene.frame_end-scene.frame_start)*f)) for f in (0,.15,.30,.50,.70,.85,1)]))
meshes=[o for o in bpy.data.objects if o.type=="MESH"]
rigs=[o for o in bpy.data.objects if o.type=="ARMATURE"]
report={"blender":bpy.app.version_string,"source":os.path.basename(source),
 "source_read_only":True,"frames_sampled":frames,
 "camera_motion_not_counted_as_skeletal_animation":True,
 "rigs":[],"camera_action_count":sum(bool(o.animation_data and o.animation_data.action) for o in bpy.data.objects if o.type=="CAMERA"),
 "mesh_shape_key_animated_count":sum(bool(o.data.shape_keys and o.data.shape_keys.animation_data and o.data.shape_keys.animation_data.action) for o in meshes),
 "game_animation_accepted":False,
 "unreal_native_animation_import_verified_by_this_test":False,
 "motion_quality_review_completed":False}
for rig in rigs:
 base={p.name:[] for p in rig.pose.bones}
 for f in frames:
  scene.frame_set(f);bpy.context.view_layer.update()
  for p in rig.pose.bones:base[p.name].append(p.matrix.copy())
 animated=[]
 for name,mats in base.items():
  a=mats[0]
  motion=max(((a.inverted_safe() @ other).translation.length for other in mats[1:]),default=0)
  rota=max((a.to_quaternion().rotation_difference(other.to_quaternion()).angle for other in mats[1:]),default=0)
  if motion>.00005 or rota>.00005:animated.append({"bone":name,"max_relative_translation":round(motion,6),"max_rotation_radians":round(rota,6)})
 action=rig.animation_data.action if rig.animation_data else None
 slot=rig.animation_data.action_slot if rig.animation_data else None
 deformed=[m.name for m in meshes if any(md.type=="ARMATURE" and md.object is rig for md in m.modifiers)]
 driven_constraints=sum(len(pb.constraints) for pb in rig.pose.bones)
 strict={"assigned_action":action is not None,"valid_45_action_slot":slot is not None,
     "skeletal_motion_present":len(animated)>0,
     "deforms_actual_mesh":len(deformed)>0,
     "no_unbaked_pose_constraints":driven_constraints==0}
 report["rigs"].append({"name":rig.name,"bones":len(rig.data.bones),
     "animation_action":action.name if action else None,
     "animation_slot_identifier":slot.identifier if slot else None,
     "deformed_mesh_count":len(deformed),
     "animated_pose_bones_count":len(animated),
     "animated_pose_bones":animated,
     "pose_constraints":driven_constraints,
     "structural_checks":strict,
     "structural_acceptance":all(strict.values())})
report["any_skeletal_action"]=any(r["animation_action"] is not None for r in report["rigs"])
report["any_evaluated_skeletal_motion"]=any(r["animated_pose_bones_count"]>0 for r in report["rigs"])
report["any_structurally_valid"]=any(r["structural_acceptance"] for r in report["rigs"])
report["explicit_outstanding"]=[
 "Animation must be compatible with intended target Unreal Skeleton and retarget policy",
 "Independent motion/audio-visual artistic review remains required",
 "Attack/contact/recovery markers and AnimNotifies must be authored and validated as game events",
 "Frame-by-frame bone-world comparison with imported Unreal AnimSequence not performed",
 "Root motion/in-place variant and contact/physics behavior not audited by this probe",
 "An animated camera never counts as reusable character animation"]
os.makedirs(os.path.dirname(result),exist_ok=True)
with open(result,"w",encoding="utf-8") as f:json.dump(report,f,indent=2)
print("NW_ACTION_AUDIT="+json.dumps({k:v for k,v in report.items() if k!="rigs"}))
for r in report["rigs"]:
 print("RIG="+json.dumps({k:v for k,v in r.items() if k!="animated_pose_bones"}))
