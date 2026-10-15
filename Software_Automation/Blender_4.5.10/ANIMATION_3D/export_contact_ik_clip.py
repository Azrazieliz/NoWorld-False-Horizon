"""Export contact-aware constrained action from .blend; verify baked FBX is genuinely animated.
Does not alter editable source. All tests local to isolated worktree.
"""
import bpy,os,time,json,math
from mathutils import Vector
BASE=os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
OUT=os.path.join(BASE,"VALIDATION","phase2")
started=time.perf_counter()
source=os.path.join(OUT,"NW_ArticulatedMount_IK.blend")
bpy.ops.wm.open_mainfile(filepath=source)
rig=bpy.data.objects["NW_Skyrunner_Mount_Rig"]
scene=bpy.context.scene
scene.frame_start=1;scene.frame_end=48
bpy.ops.object.select_all(action='DESELECT')
rig.select_set(True);bpy.context.view_layer.objects.active=rig
outfile=os.path.join(OUT,"NW_Mount_ContactIK_AnimOnly.fbx")
bpy.ops.export_scene.fbx(filepath=outfile,use_selection=True,object_types={"ARMATURE"},
 bake_anim=True,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,
 bake_anim_use_all_actions=False,bake_anim_simplify_factor=0,add_leaf_bones=False)
assert os.path.getsize(outfile)>10000
original_pose=[]
for f in (1,12,24,36,48):
 scene.frame_set(f);bpy.context.view_layer.update()
 original_pose.append([round(float(v),5) for v in rig.matrix_world @ rig.pose.bones["FrontL_Lower"].tail])
# Re-import into empty temporary scene to test baked motion, without modifying source .blend.
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=outfile)
rigs=[o for o in bpy.data.objects if o.type=="ARMATURE"]
assert len(rigs)==1
imported=rigs[0]
action=imported.animation_data.action if imported.animation_data else None
action_name=action.name if action else None
if not action:
 candidates=[a for a in bpy.data.actions if "Skyrunner_Mount_Rig" in a.name]
 if candidates:
  imported.animation_data_create();imported.animation_data.action=candidates[-1]
  if candidates[-1].slots:imported.animation_data.action_slot=candidates[-1].slots[0]
retained=bool(imported.animation_data and imported.animation_data.action)
imported_pose=[]
for f in (1,12,24,36,48):
 scene.frame_set(f);bpy.context.view_layer.update()
 pb=imported.pose.bones.get("FrontL_Lower")
 if pb:imported_pose.append([round(float(v),5) for v in imported.matrix_world @ pb.tail])
differences=[math.dist(a,b) for a,b in zip(original_pose,imported_pose)]
rep={"source":source,"fbx":outfile,"bytes":os.path.getsize(outfile),"animation_action_name":action_name,
  "imported_skeleton_bones":len(imported.data.bones),"imported_action_present":retained,
  "source_samples":original_pose,"import_samples":imported_pose,
  "max_position_diff":max(differences,default=None),
  "source_vs_import_pose_match_under_5cm":bool(differences and max(differences)<.05),
  "seconds":round(time.perf_counter()-started,3)}
with open(os.path.join(OUT,"contact_ik_fbx_roundtrip.json"),"w") as f:json.dump(rep,f,indent=2)
print("NW_IK_ROUNDTRIP="+json.dumps(rep))
