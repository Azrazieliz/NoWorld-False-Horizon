import bpy,json
p=r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015\NW_KK_Mocap_RootNormalized_PRIVATE.blend"
bpy.ops.wm.open_mainfile(filepath=p,load_ui=False)
for o in bpy.data.objects:
 if o.type=="MESH":
  print("NW_VIS_MESH",o.name,"hide_render",o.hide_render,"hide_viewport",o.hide_viewport,"hide_set",o.hide_get(),"dimensions",tuple(round(x,3) for x in o.dimensions),"location",tuple(round(x,3) for x in o.location),"materials",len(o.material_slots),"verts",len(o.data.vertices))
print("NW_CAM",bpy.context.scene.camera.name,tuple(bpy.context.scene.camera.location),bpy.context.scene.camera.data.ortho_scale)
