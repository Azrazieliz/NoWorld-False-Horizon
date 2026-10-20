"""Gate09: final source review v7 -> genuine native Unreal 5.8 animation-only FBX.
Never mutates approved character cards or prior v6 FBX.
"""
import bpy,pathlib,hashlib,json
P=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
src=P/'Luminous_A1_ScarletCadence_GroundedRootMotion_v9.blend'
out=P/'A1_ActualGrounded_UnrealExports_v9'
if out.exists():raise FileExistsError(out)
out.mkdir()
bpy.ops.wm.open_mainfile(filepath=str(src),load_ui=False)
rig=next(x for x in bpy.data.objects if x.type=='ARMATURE' and len(x.pose.bones)==975)
act=rig.animation_data.action
assert 'GroundedRootSourceFullPose' in act.name
scene=bpy.context.scene;scene.frame_start=1;scene.frame_end=33;scene.render.fps=30
bpy.ops.object.select_all(action='DESELECT')
rig.select_set(True);bpy.context.view_layer.objects.active=rig
fbx=out/'G09_LuminousV3_A1_ScarletCadence_RealSword_Grounded_v9.fbx'
bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'ARMATURE'},
 add_leaf_bones=False,axis_forward='-Z',axis_up='Y',
 bake_anim=True,bake_anim_use_all_bones=True,
 bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,
 bake_anim_simplify_factor=0.0)
assert fbx.stat().st_size>100000
m={'status':'GATE09_V9_PHYSICALLY_GROUNDED_BLENDER_EXPORTED_UNREAL_COLD_PENDING',
'fps':30,'frames':[1,33],'native_hit_event_seconds':[.18,.39,.65],
'cancel_seconds':.52,'sha256':hashlib.sha256(fbx.read_bytes()).hexdigest(),
'source_blend_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),
'sword_is_bone_parented_in_blend':True,
'equipment_gothic_model_not_canonical_Dharma':True,'fbx':str(fbx)}
(out/'INTERCHANGE.json').write_text(json.dumps(m,indent=2)+'\n',encoding='utf8')
print('G09_A1_NLA_REAL_SWORD_V7_FBX',fbx,fbx.stat().st_size,flush=True)
