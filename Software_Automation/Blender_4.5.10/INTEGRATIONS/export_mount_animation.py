"""Export armature-only baked animation clip from an original editable source .blend.
Blender 4.5.10 FBX test; intended for isolated Unreal import, not production merge.
"""
import bpy, os, time, json
ROOT=os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
OUT=os.path.join(ROOT,"VALIDATION","phase2")
start=time.perf_counter()
bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT,"NW_ArticulatedMount.blend"))
rig=bpy.data.objects["NW_Skyrunner_Mount_Rig"]
assert rig.animation_data and rig.animation_data.action and rig.animation_data.action_slot, "Missing 4.5 action slot"
bpy.context.scene.frame_start=1;bpy.context.scene.frame_end=48
bpy.ops.object.select_all(action="DESELECT")
rig.select_set(True);bpy.context.view_layer.objects.active=rig
outfile=os.path.join(OUT,"NW_Mount_Walk_Test_AnimOnly.fbx")
bpy.ops.export_scene.fbx(filepath=outfile,use_selection=True,
  object_types={'ARMATURE'},add_leaf_bones=False,
  bake_anim=True,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,
  bake_anim_use_all_actions=False,bake_anim_force_startend_keying=True,
  bake_anim_simplify_factor=0.0,apply_unit_scale=True)
result={"file":outfile,"bytes":os.path.getsize(outfile),
 "action":rig.animation_data.action.name,
 "slot":rig.animation_data.action_slot.identifier,
 "frame_start":1,"frame_end":48,"seconds":round(time.perf_counter()-start,3),
 "unreal_import":"PENDING"}
with open(os.path.join(OUT,"animation_export.json"),"w") as f:json.dump(result,f,indent=2)
print("NW_ANIMFBX="+json.dumps(result))
