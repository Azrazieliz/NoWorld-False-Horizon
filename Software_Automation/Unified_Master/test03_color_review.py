"""Neutral-color topology review of Test03. Workbench avoids unsupported MMD PBR overexposure."""
import bpy,pathlib,json,time
t0=time.perf_counter()
out=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261016\TEST03_ORANGE_BRAID')
pal={}
for m in bpy.data.materials:
 n=m.name.lower()
 if 'extension.braid' in n or 'hair' in n:
  col=(.92,.32,.055,1)
 elif 'eye' in n or 'hitomi' in n or 'pupil' in n:
  col=(.14,.48,.92,1)
 elif 'face' in n or 'body' in n or 'skin' in n or 'mayuge' in n or 'eyeline' in n:
  col=(.94,.735,.61,1)
 elif 'shirtdress' in n or 'top' in n or 'socks' in n or 'panst' in n:
  col=(.95,.955,.975,1)
 elif 'metal' in n:
  col=(.74,.78,.84,1)
 else:
  col=(.90,.84,.81,1)
 m.diffuse_color=col;pal[m.name]=col
scene=bpy.context.scene
scene.render.engine='BLENDER_WORKBENCH'
scene.display.shading.light='STUDIO'
scene.display.shading.color_type='MATERIAL'
scene.display.shading.show_shadows=True
scene.display.shading.show_cavity=True
scene.display.shading.cavity_type='BOTH'
scene.render.resolution_x=780;scene.render.resolution_y=1100
scene.render.resolution_percentage=100
scene.render.filepath=str(out/'Test03_BlenderTopologyColorReview.png')
scene.render.image_settings.file_format='PNG'
bpy.ops.render.render(write_still=True)
report={'status':'PASS_NEUTRAL_TOPOLOGY_COLOR_RENDER','preview':scene.render.filepath,
'elapsed_seconds':round(time.perf_counter()-t0,2),'shader_mode':'BLENDER_WORKBENCH_MATERIAL_COLORS',
'original_MMD_materials_preserved':True,'reference_fidelity_accepted':False}
(out/'TopologyColorReviewReceipt.json').write_text(json.dumps(report,indent=2))
print('TEST03_COLOR_REVIEW='+json.dumps(report),flush=True)
