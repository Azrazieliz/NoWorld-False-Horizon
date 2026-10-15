import bpy,json,pathlib,math
root=pathlib.Path(r"D:\UnrealProjects\NW_Blender_Mocap_Interop_20261015")
files=["NW_UE_AutoIK_Target121_Punch_ForPrivateVisualAudit.fbx","NW_CMU_Punch_1p875sec_AnimOnly.fbx"]
extra=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015\NW_KK_ChainMappedV2_ActualMotion.fbx")
if extra.exists():files.append(str(extra))
report={}
for n in files:
 bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
 bpy.ops.import_scene.fbx(filepath=str(root/n if not pathlib.Path(n).is_absolute() else pathlib.Path(n)))
 rigs=[x for x in bpy.context.scene.objects if x.type=="ARMATURE"]
 rows=[]
 for o in rigs:
  a=o.animation_data.action if o.animation_data else None
  orig_action=a
  curves=[]
  if a and a.layers and a.layers[0].strips:
   bag=a.layers[0].strips[0].channelbag(o.animation_data.action_slot)
   if bag:
    for fc in bag.fcurves:
     values=[v.co.y for v in fc.keyframe_points]
     if len(values)>1:curves.append((fc.data_path,fc.array_index,round(max(values)-min(values),5)))
  curves.sort(key=lambda x:-x[2])
  bones=["Hips","LeftArm","RightArm","RightHand","head","hand_r","thigh_l"]
  sample={}
  for f in [1,14,28,44]:
   bpy.context.scene.frame_set(f)
   bpy.context.view_layer.update()
   sample[str(f)]={n:tuple(round(v,4) for v in o.pose.bones[n].matrix.to_translation()) for n in bones if n in o.pose.bones}
  rows.append({"armature":o.name,"action":a.name if a else None,"bones":len(o.data.bones),"curve_count":len(curves),"top_curves":curves[:16],"sample_bones":sample})
 report[n]=rows
path=root/"ue_retarget_animation_content_audit_v3.json"
path.write_text(json.dumps(report,indent=2))
print("NW_ANIM_FIDELITY_AUDIT="+json.dumps({k:[{"rig":x["armature"],"curves":x["curve_count"],"top":x["top_curves"][:3]} for x in v] for k,v in report.items()}))
