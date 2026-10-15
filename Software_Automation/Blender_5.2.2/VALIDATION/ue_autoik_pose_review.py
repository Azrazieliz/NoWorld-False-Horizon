"""Visual roundtrip of actual UE5.8 IK retargeted 121-bone AnimSequence.
Loads private original target skeletal FBX and Unreal-exported AnimSequence FBX.
Assigns matching 5.2 Action+Slot in fresh isolated Blender derivative; records 3 poses.
Art quality is observed separately from numeric motion and native UE import.
"""
import bpy,json,os,math,time
from mathutils import Vector
ROOT=r"D:\UnrealProjects\NW_Blender_Mocap_Interop_20261015"
T=time.perf_counter()
bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=os.path.join(ROOT,"NW_KK_121Bone_TargetSkeletal_Private.fbx"))
kk=next(o for o in bpy.data.objects if o.type=="ARMATURE")
meshes=[o for o in bpy.data.objects if o.type=="MESH"]
origbones=set(b.name for b in kk.data.bones)
assert len(origbones)>=115 and len(meshes)>=6,(len(origbones),len(meshes))
bpy.ops.import_scene.fbx(filepath=os.path.join(ROOT,"NW_UE_AutoIK_Target121_Punch_ForPrivateVisualAudit.fbx"))
others=[o for o in bpy.data.objects if o.type=="ARMATURE" and o!=kk]
assert len(others)==1,[(o.name,len(o.data.bones)) for o in others]
motion=others[0]
src_action=motion.animation_data.action if motion.animation_data else None
assert src_action and src_action.slots
matches=len(origbones & set(b.name for b in motion.data.bones))
assert matches>=115,matches
kk.animation_data_create()
kk.animation_data.action=src_action
kk.animation_data.action_slot=src_action.slots[0]
# Hide redundant animated-source skeleton, keep for provenance.
motion.hide_render=True;motion.hide_set(True)
scene=bpy.context.scene;scene.frame_start=1;scene.frame_end=44;scene.render.fps=24
rigging={"matched_bones":matches,"target_bones":len(origbones),"animated_source_bones":len(motion.data.bones),
"imported_action":src_action.name,"assigned_slot":kk.animation_data.action_slot.identifier}
# A real scene can be objectively inspected with constant camera and material mode.
camera_data=bpy.data.cameras.new("NW_Static_ActionReviewCamera")
camera=bpy.data.objects.new("NW_Static_ActionReviewCamera",camera_data)
scene.collection.objects.link(camera)
camera.location=(.10,-4.6,2.0)
camera.rotation_euler=(Vector((0,0,1.05))-camera.location).to_track_quat("-Z","Y").to_euler()
camera_data.type="ORTHO";camera_data.ortho_scale=2.7;scene.camera=camera
scene.render.engine="BLENDER_WORKBENCH"
scene.display.shading.color_type="MATERIAL";scene.display.shading.light="STUDIO"
scene.render.resolution_x=540;scene.render.resolution_y=760
scene.render.image_settings.file_format="PNG"
scene.render.resolution_percentage=100
pos={}
images=[]
for f in (1,19,44):
 scene.frame_set(f);bpy.context.view_layer.update()
 pos[f]={"head":[round(float(x),5) for x in kk.pose.bones["head"].matrix.translation],
         "right_hand":[round(float(x),5) for x in kk.pose.bones["hand_r"].matrix.translation],
         "left_hand":[round(float(x),5) for x in kk.pose.bones["hand_l"].matrix.translation]}
 out=os.path.join(ROOT,"NW_UE_AutoIK_Pose_%02d.png"%f)
 scene.render.filepath=out;bpy.ops.render.render(write_still=True)
 images.append(out)
scene.frame_set(19)
for screen in bpy.data.screens:
 for area in screen.areas:
  if area.type=="VIEW_3D":area.spaces.active.region_3d.view_perspective="CAMERA"
path=os.path.join(ROOT,"NW_UE_AutoIK_GameActionReview_PRIVATE.blend")
bpy.ops.wm.save_as_mainfile(filepath=path,compress=True)
def dd(a,b):return math.dist(pos[a]["right_hand"],pos[b]["right_hand"])
report={"blender":bpy.app.version_string,"ue_source":"5.8.3 native IK retarget output",
 "source_and_target_original_files_modified":False,
 "skeletal":rigging,"sample_poses":pos,"world_hand_movement":max(dd(1,19),dd(1,44)),
 "editable_blend":path,"pose_pngs":images,"seconds":round(time.perf_counter()-T,3),
 "professional_animation_accepted":False,"auto_ik_quality_verified":False}
with open(os.path.join(ROOT,"ue_auto_ik_visual_review.json"),"w") as f:json.dump(report,f,indent=2)
print("NOWORLD_UE_VISUAL="+json.dumps({k:v for k,v in report.items() if k not in ("sample_poses","pose_pngs")}))
