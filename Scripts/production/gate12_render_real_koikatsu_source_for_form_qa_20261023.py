"""Render existing authentic Koikatsu-derived geometry for human visual comparison.
Does not edit the frozen .blend or change any asset/authoritative state.
"""
import bpy,math,pathlib,sys,json,traceback
from mathutils import Vector
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
form=sys.argv[sys.argv.index("--")+1]
src=R/"SourceAssets/Gate12/LuminousForms20261023"/form/"CANDIDATE_NATIVE_FROM_EXISTING_KKBP_PMX"
meshes=[x for x in bpy.data.objects if x.type=="MESH"]
bounds=[x.matrix_world @ Vector(c) for x in meshes for c in x.bound_box]
minimum=Vector(tuple(min(v[i] for v in bounds)for i in range(3)))
maximum=Vector(tuple(max(v[i] for v in bounds)for i in range(3)))
mid=(minimum+maximum)*0.5
height=maximum.z-minimum.z
print("NATIVE_BOUNDS",list(minimum),list(maximum),height,flush=True)
scene=bpy.context.scene
camera_data=bpy.data.cameras.new("NativeSourceQA_Camera")
camera=bpy.data.objects.new("NativeSourceQA_Camera",camera_data)
scene.collection.objects.link(camera)
distance=max(2.,height*2.8)
camera.location=(mid.x,mid.y-distance,mid.z+height*.12)
aim=mid+Vector((0,0,height*.13))
camera.rotation_euler=(aim-camera.location).to_track_quat("-Z","Y").to_euler()
camera_data.type="ORTHO"
camera_data.ortho_scale=height*1.28
scene.camera=camera
def light(name,x,y,z,energy,size):
 data=bpy.data.lights.new(name,"AREA")
 data.energy=energy
 data.shape="DISK"
 data.size=size
 obj=bpy.data.objects.new(name,data)
 scene.collection.objects.link(obj)
 obj.location=(mid.x+x,mid.y+y,mid.z+z)
 obj.rotation_euler=(aim-obj.location).to_track_quat("-Z","Y").to_euler()
light("OriginalQA_Key",height,-height*1.6,height*1.3,850,max(1.,height))
light("OriginalQA_Fill",-height*1.3,-height*1.2,height*.8,420,max(1.,height))
light("OriginalQA_Rim",height*.8,height*1.4,height*1.1,650,max(1.,height))
scene.render.engine="BLENDER_EEVEE"
scene.render.resolution_x=600
scene.render.resolution_y=940
scene.render.resolution_percentage=100
scene.world.color=(.13,.13,.13)
scene.render.image_settings.file_format="PNG"
scene.render.film_transparent=False
out=src/"ORIGINAL_NATIVE_MODEL_FRONT_VISUAL_QA_20261023.png"
scene.render.filepath=str(out)
try:
 bpy.ops.render.render(write_still=True)
 print("ORIGINAL_QA_RENDER",out,out.stat().st_size,flush=True)
except Exception:
 print("ORIGINAL_QA_RENDER_ERROR",traceback.format_exc()[-1100:],flush=True)
