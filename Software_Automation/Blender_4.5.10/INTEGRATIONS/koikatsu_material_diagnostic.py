"""Render a PRIVATE Koikatsu reconstruction materials diagnostic from local assembled Blender source.
This is a lighting/material audit, not proof of original Koikatsu appearance.
"""
import bpy,time,os,json
from mathutils import Vector
T=time.perf_counter()
OUT=r"D:\UnrealProjects\NW_Blender_Private_Review_20261015"
bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT,"NW_KK_ChikaHaruno_AssemblyReview_NONCANONICAL.blend"))
scene=bpy.context.scene
scene.render.engine="CYCLES"
scene.cycles.device="CPU"
scene.cycles.samples=8
scene.cycles.use_denoising=True
scene.render.threads_mode='FIXED';scene.render.threads=2
scene.render.resolution_x=450;scene.render.resolution_y=640
scene.render.resolution_percentage=100
world=bpy.data.worlds.new("NW_Neutral_StudioWorld")
scene.world=world;world.use_nodes=True
bg=world.node_tree.nodes.get("Background")
bg.inputs["Color"].default_value=(.26,.29,.35,1)
bg.inputs["Strength"].default_value=.60
def area(n,loc,power,color,size):
 data=bpy.data.lights.new(n,'AREA');data.energy=power;data.color=color
 data.shape="DISK";data.size=size
 ob=bpy.data.objects.new(n,data);scene.collection.objects.link(ob)
 ob.location=loc
 target=Vector((0,0,.9))
 ob.rotation_euler=(target-ob.location).to_track_quat('-Z','Y').to_euler()
area("NW_Studio_Key",(1.8,-2.2,2.9),130,(1,.92,.82),2.6)
area("NW_Studio_Fill",(-2,-.6,1.7),70,(.65,.83,1),3.3)
area("NW_Studio_Rim",(0.7,1.4,2.2),95,(1,.94,1),2.0)
# actual imported materials (not Workbench diffuse colors)
scene.view_settings.view_transform='AgX'
scene.render.image_settings.file_format='PNG'
for screen in bpy.data.screens:
 for area in screen.areas:
  if area.type=='VIEW_3D':
   area.spaces.active.region_3d.view_perspective='CAMERA'
lit_blend=os.path.join(OUT,'NW_KK_ChikaHaruno_LitReview_NONCANONICAL.blend')
bpy.ops.wm.save_as_mainfile(filepath=lit_blend,compress=True)
path=os.path.join(OUT,"NW_KK_ChikaHaruno_MaterialDiagnostic_Cycles.png")
scene.render.filepath=path
bpy.ops.render.render(write_still=True)
result={"blender":bpy.app.version_string,"status":"RENDERED_MATERIAL_AUDIT",
 "engine":"Cycles CPU","samples":8,"resolution":[450,640],
 "render":path,"bytes":os.path.getsize(path),
 "seconds":round(time.perf_counter()-T,2),
 "shader_parity_approved":False,"artist_accepted":False}
with open(os.path.join(OUT,"private_material_diagnostic.json"),"w") as f:json.dump(result,f,indent=2)
print("NW_KK_MATERIAL="+json.dumps(result))
