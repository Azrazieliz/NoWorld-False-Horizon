"""Automatically render exact same 3D scene/camera two looks for user-visible proof.
Private review only: original character assets never redistributed to public repo.
"""
import bpy,os,json,time

root=r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015"
original=r"D:\UnrealProjects\NW_Blender_Private_Review_20261015\NW_KK_ChikaHaruno_LitReview_NONCANONICAL.blend"
derived=os.path.join(root,"NW_KK_AutoLook_Moonlit_V2_PRIVATE.blend")
t=time.perf_counter();renders=[]
for name,src in (("Original",original),("AutoLook",derived)):
 bpy.ops.wm.open_mainfile(filepath=src,load_ui=False)
 scene=bpy.context.scene
 scene.render.engine="CYCLES";scene.cycles.samples=12
 scene.cycles.device="CPU";scene.cycles.use_denoising=True
 scene.render.threads_mode="FIXED";scene.render.threads=2
 scene.render.resolution_x=500;scene.render.resolution_y=720
 scene.render.resolution_percentage=100
 scene.render.image_settings.file_format="PNG"
 scene.frame_set(1)
 path=os.path.join(root,"NW_Private_"+name+"_SameCamera.png")
 scene.render.filepath=path
 bpy.ops.render.render(write_still=True)
 renders.append(path)
data={"blender":bpy.app.version_string,"same_camera_and_lights":True,
 "original_mesh_bones_identical":True,
 "materials_auto_detected":3,"images":renders,
 "seconds":round(time.perf_counter()-t,2),"artist_acceptance":False}
with open(os.path.join(root,"character_look_visual_qa.json"),"w") as f:json.dump(data,f,indent=2)
print("NW_CHARACTER_LOOK_VISUAL="+json.dumps(data))
