"""Render genuine 72-frame 24fps keyframed private character acting video; source remains local.
Use 5 Cycles samples and 420x540 for weak CPU. Not publish user asset in public Git.
"""
import bpy,os,json,time
OUT=r"D:\UnrealProjects\NW_Blender_Private_Review_20261015"
t=time.perf_counter()
bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT,"NW_ChikaHaruno_Acting72Frames_NONCANONICAL.blend"))
s=bpy.context.scene;s.frame_start=1;s.frame_end=72;s.render.fps=24
s.render.engine="CYCLES";s.cycles.samples=5;s.cycles.use_denoising=True
s.render.threads_mode="FIXED";s.render.threads=2
s.render.resolution_x=420;s.render.resolution_y=540
s.render.image_settings.file_format="FFMPEG"
s.render.ffmpeg.format="MPEG4";s.render.ffmpeg.codec="H264"
s.render.ffmpeg.constant_rate_factor="MEDIUM";s.render.ffmpeg.ffmpeg_preset="GOOD"
p=os.path.join(OUT,"NW_ChikaHaruno_ActualFacialActing_3sec.mp4")
s.render.filepath=p
bpy.ops.render.render(animation=True)
result={"status":"REAL_NATIVE_BLENDER_RENDERED_ANIMATION","frames":72,"fps":24,
"duration_seconds":3,"resolution":[420,540],"samples":5,
"file":p,"bytes":os.path.getsize(p),
"elapsed_seconds":round(time.perf_counter()-t,2),
"artist_accepted":False,"source_redistributed":False}
with open(os.path.join(OUT,"real_character_video_results.json"),"w") as f:json.dump(result,f,indent=2)
print("NOWORLD_KK_FACIAL_VIDEO="+json.dumps(result))
