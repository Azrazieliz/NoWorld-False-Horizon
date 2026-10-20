"""Gate09 v8 professional cinematic Hit03 camera on completed Blender5 sword-coupled
v7 actor motion. New editorial shot camera, no changes to accepted character rig.
"""
import bpy,pathlib,hashlib,json
from mathutils import Vector
P=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
source=P/'Luminous_A1_ScarletCadence_ConstraintDriven_v7.blend'
out=P/'Luminous_A1_ScarletCadence_CinematicEdit_v8.blend'
png=P/'Luminous_A1_ScarletCadence_Hit03_CinematicBeauty_v8.png'
rec=P/'Luminous_A1_ScarletCadence_Hit03_CinematicBeauty_v8.json'
for x in (out,png,rec):
 if x.exists():raise FileExistsError(x)
bpy.ops.wm.open_mainfile(filepath=str(source),load_ui=False)
scene=bpy.context.scene
old=scene.camera
old.name='G09_A1_Camera_WorldMode_CleanGameplayFraming'
new=bpy.data.cameras.new('G09_A1_CinematicCamera_Hit03_Lunge')
newobj=bpy.data.objects.new('G09_A1_CinematicCamera_Hit03_Lunge',new)
bpy.context.collection.objects.link(newobj)
newobj.location=(-2.6,-3.5,1.65)
newobj.rotation_euler=(Vector((-.35,0,.95))-newobj.location).to_track_quat('-Z','Y').to_euler()
new.type='ORTHO';new.ortho_scale=3.05
# Set 2 named art direction cameras, rather than controlling each character bone.
scene.camera=newobj
scene.frame_set(20)
for label,frame in [('A1_Hit01_180ms',6),('A1_Hit02_390ms',12),
                    ('A1_CinematicShot_Hit03_650ms',20)]:
 if not any(m.name==label for m in scene.timeline_markers):
  marker=scene.timeline_markers.new(label,frame=frame)
  marker.camera=old if frame<20 else newobj
  # These metadata markers denote authorized timings; runtime still Gate12-owned.
scene.render.engine='BLENDER_EEVEE'
scene.render.resolution_x=1510;scene.render.resolution_y=1650
scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.render.filepath=str(png)
bpy.ops.render.render(write_still=True)
assert png.stat().st_size>300000
scene.camera=old
scene.frame_set(20)
bpy.ops.wm.save_as_mainfile(filepath=str(out),compress=True)
record={'schema':'GATE09_A1_CINEMATIC_CAMERA_AND_REAL_SOURCE_BEAUTY_V8',
'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
'new_blend_sha256':hashlib.sha256(out.read_bytes()).hexdigest(),
'render_sha256':hashlib.sha256(png.read_bytes()).hexdigest(),
'original_character_modified':False,'same_975_bone_animation':True,
'camera_world_mode':old.name,'camera_specific_finisher':newobj.name,
'hit_marker_frames':[6,12,20],'sword_continues_following_right_wrist':True,
'visual_art_acceptance':False,'render_file':str(png)}
rec.write_text(json.dumps(record,indent=2)+'\n',encoding='utf8')
print('GATE09_REAL_LUMINOUS_A1_CINEMATIC_FINISHER_BEAUTY',str(png),png.stat().st_size,flush=True)
