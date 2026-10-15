"""Bake evaluated 4-leg IK into a purely keyframed deformation skeleton in isolated derivative.
Tests positional equivalence before/after baking on integer frames.
"""
import bpy, os, math, json, time
ROOT=os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
OUT=os.path.join(ROOT,"VALIDATION","phase2")
start=time.perf_counter()
bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT,"NW_ArticulatedMount_IK.blend"))
scene=bpy.context.scene
rig=bpy.data.objects["NW_Skyrunner_Mount_Rig"]
samples=(1,12,24,36,48)
def points():
 out={}
 for f in samples:
  scene.frame_set(f);bpy.context.view_layer.update()
  out[str(f)]=[round(float(v),6) for v in rig.matrix_world @ rig.pose.bones["FrontL_Lower"].tail]
 return out
before=points()
bpy.ops.object.select_all(action="DESELECT")
rig.select_set(True);bpy.context.view_layer.objects.active=rig
bpy.ops.object.mode_set(mode="POSE")
bpy.ops.pose.select_all(action="SELECT")
res=bpy.ops.nla.bake(frame_start=1,frame_end=48,step=1,only_selected=False,
  visual_keying=True,clear_constraints=True,clear_parents=False,use_current_action=False,
  clean_curves=False,bake_types={'POSE'})
bpy.ops.object.mode_set(mode="OBJECT")
# nla.bake(use_current_action=False) creates a new pose action and drops the object-location
# root translation stored in the prior action. Restore explicit root motion on the new action.
for frame in range(1,49):
 scene.frame_set(frame)
 rig.location=(0,1.05*(frame-1)/24,-.16)
 rig.keyframe_insert(data_path="location",frame=frame,group="NW_RootMotion")
after=points()
diff={k:round(math.dist(before[k],after[k]),6) for k in before}
remaining={pb.name:len(pb.constraints) for pb in rig.pose.bones if pb.name.endswith("_Lower")}
scene.frame_set(1)
blend=os.path.join(OUT,"NW_ArticulatedMount_IK_Baked.blend")
bpy.ops.wm.save_as_mainfile(filepath=blend,compress=True)
bpy.ops.object.select_all(action="DESELECT")
rig.select_set(True);bpy.context.view_layer.objects.active=rig
fbx=os.path.join(OUT,"NW_ContactIK_Baked_AnimOnly.fbx")
bpy.ops.export_scene.fbx(filepath=fbx,use_selection=True,object_types={'ARMATURE'},
 bake_anim=True,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,
 bake_anim_use_all_actions=False,bake_anim_simplify_factor=0,add_leaf_bones=False)
report={"blender":bpy.app.version_string,"bake_operator":str(res),
 "sample_source_world":before,"sample_baked_world":after,
 "sample_differences":diff,"max_diff":max(diff.values()),"lower_constraints_remaining":remaining,
 "same_pose_under_1mm":max(diff.values())<.001,
 "blend":blend,"fbx":fbx,"bytes":os.path.getsize(fbx),
 "seconds":round(time.perf_counter()-start,3),"unreal_accepted":False}
with open(os.path.join(OUT,"contact_ik_manual_bake.json"),"w") as f:json.dump(report,f,indent=2)
print("NW_MANUAL_BAKE="+json.dumps(report))
