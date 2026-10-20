"""Render 12 alpha FX contact frames independently from physically recorded
sword motion in Blender5.2; do NOT rerender original Luminous mesh per FX tweak.
"""
import bpy,pathlib,sys
P=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
SRC=P/'Luminous_A1_ScarletCadence_ActualSwordSweptFX_v10.blend'
OUT=P/'Luminous_A1_ScarletCadence_v10_CleanSwordFX_AlphaFrames'
if OUT.exists():raise FileExistsError('Cannot overwrite previous VFX alpha frame pass')
OUT.mkdir()
bpy.ops.wm.open_mainfile(filepath=str(SRC),load_ui=False)
s=bpy.context.scene
s.render.engine='BLENDER_EEVEE';s.render.film_transparent=True
s.render.image_settings.file_format='PNG';s.render.image_settings.color_mode='RGBA'
s.render.resolution_x=640;s.render.resolution_y=720;s.render.resolution_percentage=100
game=bpy.data.objects['G09_A1_Camera_WorldMode_CleanGameplayFraming']
finisher=bpy.data.objects['G09_A1_CinematicCamera_Hit03_Lunge']
active=sorted({f for hit in (6,12,20) for f in range(hit-2,hit+2)})
for f in active:
 s.frame_set(f);s.camera=game if f<20 else finisher
 for obj in bpy.data.objects:
  if obj.type in ('MESH','CURVE','SURFACE','FONT','META') and not obj.name.startswith('FX_A1_RealSwordMotion_'):
   obj.hide_render=True
 p=OUT/f'fx_{f:04d}.png'
 s.render.filepath=str(p)
 bpy.ops.render.render(write_still=True)
 assert p.stat().st_size>3000
 print('PHYSICALLY_TRACED_SWORD_FX_ALPHA',f,p.stat().st_size,flush=True)
print('ACTUAL_SWORD_MOTION_SILVER_BLOOD_FX_ALPHA_PASS_12_COMPLETE')
