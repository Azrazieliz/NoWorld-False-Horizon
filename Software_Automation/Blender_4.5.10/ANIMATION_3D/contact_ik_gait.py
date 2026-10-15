"""Contact-aware, in-motion quadruped IK gait test (Blender 4.5.10).
Edits an isolated COPY of the already generated mount; original source stays untouched.
End effector foot targets lock in stance while rig translates; evaluated foot drift measured.
"""
import bpy,math,os,time,json
from mathutils import Vector
BASE=os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
OUT=os.path.join(BASE,"VALIDATION","phase2")
t0=time.perf_counter()
bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT,"NW_ArticulatedMount.blend"))
rig=bpy.data.objects["NW_Skyrunner_Mount_Rig"]
scene=bpy.context.scene;scene.frame_start=1;scene.frame_end=48;scene.render.fps=24
T=24;STRIDE=1.05;STANCE=.63;STEP_HEIGHT=.27
names=["FrontL","FrontR","HindL","HindR"]
targets={};samples={}
for name in names:
 src=rig.pose.bones[name+"_Lower"];upper=rig.pose.bones[name+"_Upper"]
 # Unconstrained controller empty stores world-space independent footplant.
 target=bpy.data.objects.new("NW_IK_TARGET_"+name,None)
 scene.collection.objects.link(target)
 target.empty_display_type="CUBE";target.empty_display_size=.085
 ik=src.constraints.new("IK");ik.name="NW_ContactLockedIK"
 ik.target=target;ik.chain_count=2;ik.use_stretch=False
 ik.iterations=96
 targets[name]=target
 samples[name]=[]
phase={"FrontL":0,"HindR":0,"FrontR":.5,"HindL":.5}
for i in range(48):
 frame=i+1;scene.frame_set(frame)
 root_y=STRIDE*i/T
 # Lower hips slightly to avoid fully extended singular two-bone IK when body travels.
 rig.location=(0,root_y,-.16)
 rig.keyframe_insert(data_path="location",frame=frame,group="NW_RootMotion")
 for name in names:
  side=-1 if name.endswith("L") else 1
  front=-1 if name.startswith("Front") else 1
  p=phase[name]
  u=(i/T+p)%1.0
  n=math.floor(i/T+p)
  anchor=(n-p)*STRIDE
  if u<=STANCE:
   foot_y=anchor
   foot_z=.20
   active=True
  else:
   a=(u-STANCE)/(1.0-STANCE)
   eased=a*a*(3-2*a)
   foot_y=anchor+STRIDE*eased
   foot_z=.20+STEP_HEIGHT*math.sin(math.pi*a)
   active=False
  target=targets[name]
  target.location=(side*.75,front*.92+foot_y,foot_z)
  target.keyframe_insert(data_path="location",frame=frame,group="NW_Contact_Swing")
  samples[name].append({"f":frame,"state":"STANCE" if active else "SWING",
                        "target":[float(q) for q in target.location]})
# Refresh scene and compute world-space contact continuity after constraints.
metrics={}
for name in names:
 coords=[]
 for f in range(1,49):
  scene.frame_set(f);bpy.context.view_layer.update()
  src=rig.pose.bones[name+"_Lower"]
  world=rig.matrix_world @ src.tail
  coords.append(tuple(world))
 diffs=[]
 errors=[]
 for i in range(47):
  if samples[name][i]["state"]=="STANCE" and samples[name][i+1]["state"]=="STANCE":
   diffs.append(math.dist(coords[i][:2],coords[i+1][:2]))
 for i in range(48):
  if samples[name][i]["state"]=="STANCE":
   errors.append(math.dist(coords[i],samples[name][i]["target"]))
 metrics[name]={"num_plant_steps":len(diffs),"max_plant_xy_drift":round(max(diffs,default=0),6),
 "mean_plant_xy_drift":round(sum(diffs)/len(diffs),6) if diffs else None,
 "max_contact_target_3d_error":round(max(errors,default=0),6),
 "mean_contact_target_3d_error":round(sum(errors)/len(errors),6) if errors else None}
scene.frame_set(11)
blend=os.path.join(OUT,"NW_ArticulatedMount_IK.blend")
bpy.ops.wm.save_as_mainfile(filepath=blend,compress=True)
scene.render.engine="BLENDER_WORKBENCH";scene.display.shading.color_type="MATERIAL";scene.display.shading.show_cavity=True
scene.render.resolution_x=900;scene.render.resolution_y=600
png=os.path.join(OUT,"NW_ArticulatedMount_IK.png");scene.render.filepath=png
scene.render.image_settings.file_format="PNG";bpy.ops.render.render(write_still=True)
# Constraints and root object animation are baked during exporter. Control empties unselected.
bpy.ops.object.select_all(action="DESELECT")
for o in bpy.data.objects:
 if o.get("nw_source_part"):o.select_set(True)
rig.select_set(True);bpy.context.view_layer.objects.active=rig
fbx=os.path.join(OUT,"NW_Mount_ContactIK_Skeletal.fbx")
bpy.ops.export_scene.fbx(filepath=fbx,use_selection=True,object_types={'MESH','ARMATURE'},
 add_leaf_bones=False,apply_unit_scale=True,bake_anim=True,
 bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,
 bake_anim_use_all_actions=False,bake_anim_simplify_factor=0)
mx=max((m["max_plant_xy_drift"] for m in metrics.values()),default=999)
err=max((m["max_contact_target_3d_error"] for m in metrics.values()),default=999)
report={"blender":bpy.app.version_string,"in_place_original_has_sliding":True,"movement":"translated root object + world-locked IK stance",
 "stride_per_cycle":STRIDE,"cycle_frames":T,"stance_fraction":STANCE,
 "metrics":metrics,"max_plant_xy_drift":mx,"max_target_error":err,
 "numerical_contact_check_pass":mx<.02 and err<.04,
 "blend_file":blend,"fbx_file":fbx,"seconds":round(time.perf_counter()-t0,3),
 "artist_quality_approved":False,"unreal_animation_import_validated":False}
with open(os.path.join(OUT,"mount_contact_ik_results.json"),"w") as f:json.dump(report,f,indent=2)
print("NW_CONTACT_IK="+json.dumps(report))
