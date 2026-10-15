"""Render a real 72-frame camera/ornament animation from saved art directed Blender scene."""
import bpy,os,json,time
ROOT=os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
OUT=os.path.join(ROOT,"VALIDATION","phase4")
t=time.perf_counter()
bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT,"NW_StarfallSanctum_ArtDirected.blend"))
s=bpy.context.scene
s.frame_start=1;s.frame_end=72;s.render.fps=24
s.render.engine="CYCLES";s.cycles.samples=5;s.cycles.use_denoising=True;s.cycles.device="CPU"
s.render.threads_mode="FIXED";s.render.threads=2
s.render.resolution_x=640;s.render.resolution_y=404;s.render.resolution_percentage=100
s.render.image_settings.file_format="FFMPEG"
s.render.ffmpeg.format="MPEG4";s.render.ffmpeg.codec="H264"
s.render.ffmpeg.constant_rate_factor="MEDIUM"
s.render.ffmpeg.ffmpeg_preset="GOOD"
out=os.path.join(OUT,"NW_StarfallSanctum_CameraTour.mp4")
s.render.filepath=out
bpy.ops.render.render(animation=True)
r={"blender":bpy.app.version_string,"source":"NW_StarfallSanctum_ArtDirected.blend",
 "video":out,"bytes":os.path.getsize(out),"frames":72,"fps":24,
 "resolution":[640,404],"cycles_samples":5,"duration_seconds":3,
 "seconds_elapsed":round(time.perf_counter()-t,2),"art_quality_accepted":False}
with open(os.path.join(OUT,"artdirected_animation_results.json"),"w") as f:json.dump(r,f,indent=2)
print("NW_STARFALL_ANIMATION="+json.dumps(r))
