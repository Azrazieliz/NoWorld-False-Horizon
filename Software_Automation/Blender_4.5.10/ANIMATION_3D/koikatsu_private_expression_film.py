"""REAL private 121-bone KK-derived character: 72-frame facial/pose/camera keyframed cinematic.
No source GLB changed and no private character assets distributed in Git.
Must review for unintended face/stocking shader defects; artistic acceptance false.
"""
import bpy,math,os,json,random,time
from mathutils import Vector
ROOT=r"D:\UnrealProjects\NW_Blender_Private_Review_20261015"
T=time.perf_counter()
bpy.ops.wm.open_mainfile(filepath=os.path.join(ROOT,"NW_KK_ChikaHaruno_LitReview_NONCANONICAL.blend"))
scene=bpy.context.scene
scene.frame_start=1;scene.frame_end=72;scene.render.fps=24
rig=bpy.data.objects["NW_KK_INTENDED_SKELETON_121"]
mesh=next(o for o in bpy.data.objects if o.name.startswith("NW_SHARD03_model"))
kb=mesh.data.shape_keys.key_blocks
assert all(name in kb for name in ("Mouth Happy S","Mouth Smiling","Mouth Surprised S"))
# Maintain exclusive mouth choices; each expression is a separate channel, avoid overdriven simultaneous combinations.
poses=[(1,0,0,0), (14,0,.45,0), (26,0,.78,0),
       (35,0,0,.42),(43,.54,0,0),(54,.78,0,0),(66,.20,.24,0),(72,0,.2,0)]
for f,h,sm,sur in poses:
 for name,value in (("Mouth Happy S",h),("Mouth Smiling",sm),("Mouth Surprised S",sur)):
  b=kb[name];b.value=value;b.keyframe_insert(data_path="value",frame=f,group="NW_Expression")
# Animated head turn, neck lean, chest breath, asymmetrical shoulder gesture.
for f in (1,12,24,36,48,60,72):
 t=(f-1)/71
 for name,axis,v in [
  ("head",1,.09*math.sin(2*math.pi*t-.3)),
  ("head",2,.065*math.sin(2*math.pi*t+.3)),
  ("neck",0,.045*math.sin(2*math.pi*t)),
  ("spine_03",0,.023*math.sin(4*math.pi*t)),
  ("upperarm_l",0,-.16*max(0,math.sin(math.pi*t))),
  ("upperarm_r",0,.12*max(0,math.sin(math.pi*t+.3)))
 ]:
  b=rig.pose.bones[name];b.rotation_mode="XYZ";b.rotation_euler[axis]=v
  b.keyframe_insert(data_path="rotation_euler",frame=f,group="NW_CinematicActing")
# A coherent moving camera: full figure to head closeup and return to medium portrait.
cam=scene.camera;assert cam
cam.data.type="ORTHO"
for f,loc,focus,scale in [
 (1,(.13,-3.7,1.18),(0,0,.86),1.95),
 (20,(.04,-3.2,1.21),(0,0,1.15),1.45),
 (40,(.10,-2.65,1.32),(0,0,1.36),.75),
 (57,(-.15,-2.8,1.34),(0,0,1.34),.75),
 (72,(0,-3.1,1.29),(0,0,1.20),1.36)]:
 cam.location=loc
 cam.rotation_euler=(Vector(focus)-cam.location).to_track_quat("-Z","Y").to_euler()
 cam.data.ortho_scale=scale
 cam.keyframe_insert(data_path="location",frame=f,group="NW_PortraitCamera")
 cam.keyframe_insert(data_path="rotation_euler",frame=f,group="NW_PortraitCamera")
 cam.data.keyframe_insert(data_path="ortho_scale",frame=f)
# Orbiting luminous motes around character, visual accent but not canonical character geometry.
m=bpy.data.materials.new("NW_FacialPortrait_SoftCyanMotes");m.use_nodes=True
p=m.node_tree.nodes.get("Principled BSDF")
p.inputs["Base Color"].default_value=(.35,.8,1,1)
p.inputs["Emission Color"].default_value=(.12,.7,1,1)
p.inputs["Emission Strength"].default_value=1.6
rng=random.Random(1422);parts=[]
for i in range(18):
 a=math.tau*i/18;radius=.60+rng.random()*.25
 pos0=Vector((math.cos(a)*radius,.23+rng.uniform(-.03,.17),.80+1.0*rng.random()))
 bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=.016+rng.random()*.020,location=pos0)
 o=bpy.context.object;o.name="NW_AnimatedBlueMote_%02d"%i;o.data.materials.append(m)
 parts.append(o)
 for f in (1,25,49,72):
  angle=a+.6*(f-1)/71
  o.location=pos0+Vector((.08*math.cos(angle),0,.09*math.sin(angle)))
  o.keyframe_insert(data_path="location",frame=f,group="NW_MagicSecondary")
# Saved pose and framing: chosen expressive close-up frame.
scene.frame_set(50)
scene.render.engine="CYCLES";scene.cycles.device="CPU";scene.cycles.samples=18
scene.cycles.use_denoising=True;scene.render.threads_mode="FIXED";scene.render.threads=2
scene.render.resolution_x=600;scene.render.resolution_y=760
scene.render.resolution_percentage=100
scene.render.image_settings.file_format="PNG"
scene.render.filepath=os.path.join(ROOT,"NW_CharacterExpression_Closeup.png")
for screen in bpy.data.screens:
 for area in screen.areas:
  if area.type=="VIEW_3D":area.spaces.active.region_3d.view_perspective="CAMERA"
blend=os.path.join(ROOT,"NW_ChikaHaruno_Acting72Frames_NONCANONICAL.blend")
bpy.ops.wm.save_as_mainfile(filepath=blend,compress=True)
bpy.ops.render.render(write_still=True)
res={"blender":bpy.app.version_string,"frames":72,"fps":24,
 "duration":3,"rig_bones":len(rig.data.bones),
 "mesh_expression_count":len(kb)-1,"animated_mouth_channels":3,
 "face_key_pose_events":len(poses),"actual_pose_bones":len(set(["head","neck","spine_03","upperarm_l","upperarm_r"])),
 "camera_keys":5,"animated_secondary_props":len(parts),
 "image":scene.render.filepath,"blend":blend,
 "elapsed_seconds":round(time.perf_counter()-T,2),
 "original_koikatsu_sources_modified":False,
 "source_redistribution_permitted":False,
 "quality_accepted":False,
 "android_import_validated":False}
with open(os.path.join(ROOT,"private_character_acting_results.json"),"w",encoding="utf-8") as f:json.dump(res,f,indent=2)
print("NW_ACTING="+json.dumps(res))
