"""Render three independently animated genuine Koikatsu standing actions on real Luminous."""
import bpy,pathlib,json,hashlib
from mathutils import Vector
out=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
src=out/'G09_AuthenticIdleAndBow_Source_Review_v4.blend'
bpy.ops.wm.open_mainfile(filepath=str(src),load_ui=False)
rig=next(o for o in bpy.data.objects if o.type=='ARMATURE' and len(o.pose.bones)==975)
scene=bpy.context.scene;scene.render.engine='BLENDER_WORKBENCH'
scene.render.resolution_x=640;scene.render.resolution_y=920;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.display.shading.light='STUDIO'
scene.display.shading.show_cavity=True
scene.display.shading.color_type='MATERIAL'
meshes=[o for o in bpy.data.objects if o.type=='MESH' and not o.hide_render]
pts=[o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
lo=Vector([min(p[i] for p in pts) for i in range(3)])
hi=Vector([max(p[i] for p in pts) for i in range(3)])
mid=(lo+hi)*.5
cam=bpy.data.objects.new('G09_Visual_ReviewCamera',bpy.data.cameras.new('G09_Visual_ReviewCamera'))
bpy.context.collection.objects.link(cam);scene.camera=cam
cam.data.type='ORTHO';cam.data.ortho_scale=max(hi.z-lo.z,1.4)*1.25
cam.location=mid+Vector((2,-3,0.8)).normalized()*3.0
cam.rotation_euler=(mid-cam.location).to_track_quat('-Z','Y').to_euler()
rec=[]
for a in sorted((x for x in bpy.data.actions if x.name.startswith('G09_REAL_KK_')),key=lambda x:x.name):
 rig.animation_data.action=a
 rig.animation_data.action_slot=a.slots[0]
 scene.frame_set(int(a.frame_range[0]+(a.frame_range[1]-a.frame_range[0])*0.38))
 png=out/(a.name+'_FrameReview_v4.png')
 if png.exists():raise FileExistsError(png)
 scene.render.filepath=str(png);bpy.ops.render.render(write_still=True)
 rec.append({'action':a.name,'path':str(png),'sha256':hashlib.sha256(png.read_bytes()).hexdigest(),'frame':scene.frame_current})
 print('RENDERED',png.name,flush=True)
manifest=out/'G09_Koikatsu_IdlesAndBow_VisualSourceReview_v4.json'
if manifest.exists():raise FileExistsError(manifest)
manifest.write_text(json.dumps({'schema':'G09_REAL_KOIKATSU_THREE_IDLE_BLENDER_VISUAL_QA','source_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),
'captures':rec,'render':'Blender Workbench untextured geometry-only, not accepted full-shader artistic quality'},indent=2)+'\n',encoding='utf8')
