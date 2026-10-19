"""Gate09 six real-character diagnostic source motion preview renders.
Not an art-approved skin/material or finished Scarlet Cadence presentation.
"""
import bpy,pathlib,hashlib,json
from mathutils import Vector
P=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
src=P/'G09_Dharma_WeaponSourceRetarget_v6_20261019.blend'
out=P/'DharmaSourceMotionVisualQA_v6_20261019'
if out.exists():raise FileExistsError(out)
out.mkdir()
bpy.ops.wm.open_mainfile(filepath=str(src),load_ui=False)
rig=next(o for o in bpy.data.objects if o.type=='ARMATURE' and len(o.pose.bones)==975)
scene=bpy.context.scene
scene.render.engine='BLENDER_WORKBENCH'
scene.render.resolution_x=780;scene.render.resolution_y=980;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.display.shading.light='STUDIO'
scene.display.shading.show_cavity=True
scene.display.shading.color_type='MATERIAL'
meshes=[o for o in bpy.data.objects if o.type=='MESH' and not o.hide_render]
bounds=[o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
lo=Vector([min(p[i] for p in bounds) for i in range(3)])
hi=Vector([max(p[i] for p in bounds) for i in range(3)])
mid=(lo+hi)*.5
cam=bpy.data.objects.new('G09_DharmaRealCharacterReviewCam',bpy.data.cameras.new('G09_DharmaRealCharacterReviewCam'))
bpy.context.collection.objects.link(cam)
scene.camera=cam;cam.data.type='ORTHO'
cam.data.ortho_scale=max(hi.z-lo.z,1.4)*1.30
cam.location=mid+Vector((2,-3,0.8)).normalized()*3.0
cam.rotation_euler=(mid-cam.location).to_track_quat('-Z','Y').to_euler()
proof=[]
for name in ['Hit01_DiagonalDraw','Hit02_ReturnSlash','Hit03_FinishingThrust']:
 action=bpy.data.actions['G09_DHARMA_GENUINE_SOURCE_'+name]
 rig.animation_data.action=action;rig.animation_data.action_slot=action.slots[0]
 for portion in (0.32,0.69):
  frame=max(1,round(action.frame_range[0]+(action.frame_range[1]-action.frame_range[0])*portion))
  scene.frame_set(frame)
  png=out/('G09_Dharma_'+name+'_F'+str(frame)+'.png')
  scene.render.filepath=str(png)
  bpy.ops.render.render(write_still=True)
  proof.append({'action':name,'frame':frame,'filename':png.name,
    'bytes':png.stat().st_size,'sha256':hashlib.sha256(png.read_bytes()).hexdigest(),
    'preview_only':True})
  print('DHARMA_CAPTURE',png.name,png.stat().st_size,flush=True)
(out/'VISUAL_QA_MANIFEST.json').write_text(json.dumps({
  'schema':'GATE09_DHARMA_SWORD_SOURCE_VISUAL_QA_V6',
  'source_blend_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),
  'approved_visual_material_fidelity':False,
  'preview_type':'Blender workbench actual Luminous character poses; NOT a material/quality acceptance',
  'captures':proof},indent=2)+'\n',encoding='utf8')
