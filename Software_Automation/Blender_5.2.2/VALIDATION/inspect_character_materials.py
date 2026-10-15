import bpy,os,json
p=r"D:\UnrealProjects\NW_Blender_Private_Review_20261015\NW_KK_ChikaHaruno_LitReview_NONCANONICAL.blend"
bpy.ops.wm.open_mainfile(filepath=p,load_ui=False)
meshes=[{"object":o.name,"mats":[{"i":i,"name":s.material.name if s.material else None,"base_linked":any(n.type=="BSDF_PRINCIPLED" and n.inputs["Base Color"].is_linked for n in s.material.node_tree.nodes) if s.material and s.material.use_nodes else False} for i,s in enumerate(o.material_slots)]} for o in bpy.data.objects if o.type=="MESH"]
print("NOWORLD_PRIVATE_MESH_SLOTS="+json.dumps(meshes))
