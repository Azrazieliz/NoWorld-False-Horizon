"""Gate09 A1 v8 exact Blender 5.2 real-asset render; resume exact 11-frame chunks.
World camera for first two strikes and dedicated cinematic for 3rd.
Read existing authored v8 source ONLY; no substitute/lower-detail model.
"""
import bpy,sys,pathlib,time
P=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
src=P/'Luminous_A1_ScarletCadence_GroundedRootMotion_v9.blend'
d=P/'Luminous_A1_GroundedRootMotion_v9_ActualFrames'
args=sys.argv[sys.argv.index('--')+1:]
assert len(args)==2
first,last=map(int,args)
assert 1<=first<=last<=33
if not d.exists():d.mkdir()
bpy.ops.wm.open_mainfile(filepath=str(src),load_ui=False)
s=bpy.context.scene
game=bpy.data.objects['G09_A1_Camera_WorldMode_CleanGameplayFraming']
finisher=bpy.data.objects['G09_A1_CinematicCamera_Hit03_Lunge']
s.render.engine='BLENDER_EEVEE'
s.render.resolution_x=640
s.render.resolution_y=720
s.render.resolution_percentage=100
s.render.image_settings.file_format='PNG'
s.render.image_settings.color_mode='RGB'
assert len([o for o in bpy.data.objects if o.type=='ARMATURE' and len(o.pose.bones)==975])==1
for f in range(first,last+1):
 out=d/('frame_%04d.png'%f)
 if out.exists():
  print('PRESERVED_EXISTING_FRAME',out.name,flush=True)
  continue
 s.frame_set(f)
 s.camera=game if f<20 else finisher
 s.render.filepath=str(out)
 bpy.ops.render.render(write_still=True)
 assert out.stat().st_size>100000
 print('ACTUAL_LUMINOUS_A1_V9_GROUNDED_FRAME',f,out.stat().st_size,flush=True)
print('A1_V9_REAL_GROUNDED_RENDER_CHUNK_PASS',first,last,flush=True)
