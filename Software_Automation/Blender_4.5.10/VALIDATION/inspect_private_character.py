import bpy,json,os
root=r"D:\UnrealProjects\NW_Blender_Private_Review_20261015"
bpy.ops.wm.open_mainfile(filepath=os.path.join(root,"NW_KK_ChikaHaruno_LitReview_NONCANONICAL.blend"))
rig=next(o for o in bpy.data.objects if o.type=="ARMATURE")
head_bones=[b.name for b in rig.pose.bones if any(k in b.name.lower() for k in ("head","neck","eye","spine","shoulder","arm","breast","cf_j_kub","cf_j_face"))]
mesh=next(o for o in bpy.data.objects if o.name.startswith("NW_SHARD03_model"))
keys=[k.name for k in mesh.data.shape_keys.key_blocks]
s={"head_bones":head_bones,"eyes":[k for k in keys if "Eye" in k][:42],
 "mouth":[k for k in keys if k.startswith("Mouth")],
 "nodes":[n.name for n in bpy.data.materials.get("KK cf_m_face_00").node_tree.nodes] if bpy.data.materials.get("KK cf_m_face_00") else []}
with open(os.path.join(root,"animation_input_inspection.json"),"w") as f:json.dump(s,f,indent=2)
print("HEAD="+json.dumps(head_bones))
print("MOUTH="+json.dumps(s["mouth"]))
