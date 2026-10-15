"""Produce isolated skeletal source+target Unreal retarget test fixtures.
Blender 5.2.2. Target mesh is private, derived read-only from existing KK source;
none of source imagery or resulting FBX is permitted in the public repository.
Source uses CMU female performance 144_20, full 31-bone hierarchy preserved.
"""
import bpy,os,json,time
OUT=r"D:\UnrealProjects\NW_Blender_Mocap_Interop_20261015"
PRIVATE=r"D:\UnrealProjects\NW_Blender_Private_Review_20261015"
T=time.perf_counter()
bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
sc=bpy.context.scene;sc.render.fps=24
bpy.ops.import_anim.bvh(filepath=os.path.join(OUT,"144_20.bvh"),update_scene_fps=False,use_fps_scale=True)
rig=next(o for o in sc.objects if o.type=="ARMATURE")
assert len(rig.data.bones)==31
rig.name="NW_CMU_Female_Punch_Rig"
# Minimal source-mesh: Unreal IK rig requires skeletal mesh, not armature-only.
# This cube has no artistic role; its single bone weighting allows preserving real CMU motion.
bpy.ops.mesh.primitive_cube_add(size=.25)
o=bpy.context.object
o.name="NW_MocapCalibrationMarker"
o.location=rig.pose.bones["Hips"].head
vg=o.vertex_groups.new(name="Hips");vg.add(list(range(len(o.data.vertices))),1,"REPLACE")
mod=o.modifiers.new("NW_CMU_SourceDeform","ARMATURE");mod.object=rig
o.parent=rig
bpy.ops.object.select_all(action="DESELECT")
rig.select_set(True);o.select_set(True);bpy.context.view_layer.objects.active=rig
src_path=os.path.join(OUT,"NW_CMU_FemalePunch_SourceSkeleton.fbx")
bpy.ops.export_scene.fbx(filepath=src_path,use_selection=True,object_types={'ARMATURE','MESH'},
 add_leaf_bones=False,bake_anim=True,bake_anim_use_all_bones=True,
 bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0)
# Read actual imported KK Blender scene. In-memory morph removal on exported derivative only.
bpy.ops.wm.open_mainfile(filepath=os.path.join(PRIVATE,"NW_KK_ChikaHaruno_LitReview_NONCANONICAL.blend"))
target=bpy.data.objects["NW_KK_INTENDED_SKELETON_121"]
meshes=[o for o in bpy.data.objects if o.type=="MESH"]
assert len(meshes)==6 and len(target.data.bones)==121
old_keys=sum(max(0,len(o.data.shape_keys.key_blocks)-1) for o in meshes if o.data.shape_keys)
for o in meshes:
 bpy.ops.object.select_all(action="DESELECT")
 o.select_set(True);bpy.context.view_layer.objects.active=o
 if o.data.shape_keys:o.shape_key_clear()
bpy.ops.object.select_all(action="DESELECT")
for o in meshes:o.select_set(True)
target.select_set(True);bpy.context.view_layer.objects.active=target
trg_path=os.path.join(OUT,"NW_KK_121Bone_TargetSkeletal_Private.fbx")
bpy.ops.export_scene.fbx(filepath=trg_path,use_selection=True,object_types={'ARMATURE','MESH'},
 add_leaf_bones=False,bake_anim=False,apply_unit_scale=True)
result={"blender":bpy.app.version_string,"source_skeleton_bones":31,
 "source_motion_id":"CMU 144_20 punching female","source_fbx":src_path,
 "source_fbx_size":os.path.getsize(src_path),
 "target_skeleton_bones":len(target.data.bones),"target_mesh_parts":len(meshes),
 "target_morph_channels_temporarily_skipped":old_keys,
 "target_fbx":trg_path,"target_fbx_size":os.path.getsize(trg_path),
 "original_kk_files_modified":False,"source_bvh_files_modified":False,
 "commercial_use_mocap_permitted":True,"public_asset_redistribution":False,
 "elapsed_sec":round(time.perf_counter()-T,3),
 "unreal_import_tested":False,"visual_art_accepted":False}
with open(os.path.join(OUT,"ue_retarget_fixture_export.json"),"w") as f:json.dump(result,f,indent=2)
print("NW_UE_RETARGET_FIXTURES="+json.dumps(result))
