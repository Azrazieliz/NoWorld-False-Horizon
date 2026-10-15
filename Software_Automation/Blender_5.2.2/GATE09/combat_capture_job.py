"""Actual licensed actor-performance segmentation, not hand-keyed or camera animation.
Automatic max acceleration/velocity window chooses striking arm or kicking leg
and extracts full-body CMU Action F-Curves as native reusable Blender + FBX.
"""
import bpy,sys,json,pathlib,hashlib,math
from mathutils import Vector
def pathnew(s):
 p=pathlib.Path(s).resolve()
 if p.exists():raise FileExistsError(p)
 p.parent.mkdir(parents=True,exist_ok=True);return p
def sha(p):return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
def run(job):
 src=pathlib.Path(job["bvh"]).resolve()
 if not src.is_file():raise FileNotFoundError(src)
 out=pathnew(job["blend"]);fbx=pathnew(job["fbx"]);report=pathnew(job["report"])
 input_hash=sha(src)
 bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
 sc=bpy.context.scene;sc.render.fps=24
 bpy.ops.import_anim.bvh(filepath=str(src),update_scene_fps=False,use_fps_scale=True,rotate_mode="NATIVE")
 rig=next(o for o in sc.objects if o.type=="ARMATURE")
 if not (rig.animation_data and rig.animation_data.action and rig.animation_data.action_slot):
  raise RuntimeError("Imported mocap has no Blender Action+Slot")
 limb=job.get("limb","RightHand")
 if limb not in rig.pose.bones:raise ValueError("No performance joint "+limb)
 max_f=int(rig.animation_data.action.frame_range[1])
 mode=job.get("peak_type","velocity")
 centers=[]
 last=None
 for f in range(3,max_f,2):
  sc.frame_set(f);bpy.context.view_layer.update()
  p=rig.pose.bones[limb].matrix.translation-rig.pose.bones["Hips"].matrix.translation
  if last is not None:
   speed=(p-last).length
   centers.append((f,speed))
  last=p.copy()
 legal=[x for x in centers if x[0]>25 and x[0]<max_f-25]
 if not legal:raise RuntimeError("Too short to sample performance")
 peak,value=max(legal,key=lambda x:x[1])
 start=max(1,peak-int(job.get("before_frames",18)))
 end=min(max_f,peak+int(job.get("after_frames",26)))
 action=rig.animation_data.action;slot=rig.animation_data.action_slot
 bag=action.layers[0].strips[0].channelbag(slot)
 assert bag
 keys=0
 for curve in bag.fcurves:
  kp=curve.keyframe_points
  for i in range(len(kp)-1,-1,-1):
   frame=kp[i].co.x
   if frame<start or frame>end:kp.remove(kp[i],fast=True)
  for p in kp:
   p.co.x-=start-1
   p.handle_left.x-=start-1;p.handle_right.x-=start-1
   keys+=1
  curve.update()
 assert keys>=20,keys
 sc.frame_start=1;sc.frame_end=end-start+1
 action.name="NW_AuthoredMocap_"+str(job.get("id","Strike"))
 action.asset_mark()
 action.asset_data.description="Commercial-usage-permitted CMU mocap / derivative full-body combat"
 sc.frame_set(1)
 bpy.ops.wm.save_as_mainfile(filepath=str(out),compress=True)
 bpy.ops.object.select_all(action="DESELECT")
 rig.select_set(True);bpy.context.view_layer.objects.active=rig
 bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,
  object_types={"ARMATURE"},add_leaf_bones=False,
  bake_anim=True,bake_anim_use_all_bones=True,bake_anim_use_all_actions=False,
  bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0)
 assert sha(src)==input_hash
 r={"blender":bpy.app.version_string,"performance_source":str(src),
 "bvh_sha256":input_hash,"source_preserved":True,
 "actual_mocap_bones":len(rig.data.bones),
 "automatically_detected_peak":peak,"limb":limb,"score":value,
 "original_frames":max_f,"cropped_range":[start,end],
 "generated_game_frames":end-start+1,"keyframes_of_authored_performance":keys,
 "edited_per_bone_manually":False,
 "editable_blend":str(out),"animation_only_fbx":str(fbx),
 "fbx_sha256":sha(fbx),"artist_quality_accepted":False,
 "unreal_ik_retarget_verified":False}
 report.write_text(json.dumps(r,indent=2),encoding="utf-8")
 print("NW_GATE09_REAL_FIGHT="+json.dumps(r))
if __name__=="__main__":
 a=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
 assert len(a)==1
 run(json.loads(pathlib.Path(a[0]).read_text(encoding="utf-8-sig")))
