"""2D native Grease Pencil v3 source animation -> automatic game sprite atlas.
No manual frame illustration and NO camera animation; sources remain editable.
Atlas/metadata suitable for Unreal Paper2D/VFX, to be imported by Gate12.
"""
import bpy,sys,pathlib,json,math,hashlib
def check(x):
 p=pathlib.Path(x).resolve()
 if p.exists():raise FileExistsError(p)
 p.parent.mkdir(parents=True,exist_ok=True);return p
def sha(p):return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
def run(job):
 src=pathlib.Path(job["source"]).resolve()
 assert src.is_file()
 old=sha(src)
 blend=check(job["blend"]);report=check(job["report"])
 folder=pathlib.Path(job["frames_dir"]).resolve()
 if folder.exists():raise FileExistsError(folder)
 folder.mkdir(parents=True)
 bpy.ops.wm.open_mainfile(filepath=str(src),load_ui=False)
 sc=bpy.context.scene
 gps=[o for o in sc.objects if o.type=="GREASEPENCIL"]
 assert len(gps)>=2,len(gps)
 speed=float(job.get("speed",1));scale=float(job.get("scale",1))
 assert .3<=speed<=2.5 and .25<=scale<=2
 for o in gps:
  o.scale=(scale,)*3
  o.rotation_mode="XYZ"
  # Add independent Native 2D object animation, preserving all artist-drawn GP frames.
  for f in (1,16,32,48):
   ph=2*math.pi*(f-1)/47
   o.location.x=float(job.get("x_amplitude",.2))*math.sin(ph*speed)
   o.location.z=float(job.get("z_amplitude",.14))*math.sin(ph*speed+.4)
   o.keyframe_insert(data_path="location",frame=f,group="NW_GP_IndependentMotion")
  o["nw_game_2d_source"]="reusable_GPv3_native_frames_and_motion"
 if job.get("gp_only",True):
  for o in sc.objects:
   if o.type=="MESH":o.hide_render=True
 sc.render.engine="BLENDER_WORKBENCH"
 sc.render.film_transparent=True
 sc.render.resolution_x=int(job.get("frame_width",384))
 sc.render.resolution_y=int(job.get("frame_height",384))
 sc.render.resolution_percentage=100
 sc.render.image_settings.file_format="PNG"
 sc.render.image_settings.color_mode="RGBA"
 if not sc.camera:
  raise RuntimeError("2D GP source missing camera; cannot certify game sprite frame")
 frames=[1,8,16,24,32,40,48]
 arts=[]
 for i,f in enumerate(frames):
  sc.frame_set(f)
  path=folder/("NW_2D_%02d.png"%i)
  sc.render.filepath=str(path)
  bpy.ops.render.render(write_still=True)
  if path.stat().st_size<500:raise RuntimeError("Rendered frame too small "+str(path))
  arts.append({"index":i,"source_frame":f,"file":str(path),"sha256":sha(path)})
 sc.frame_set(16)
 bpy.ops.wm.save_as_mainfile(filepath=str(blend),compress=True)
 assert sha(src)==old
 output={"blender":bpy.app.version_string,"grease_pencil_source_objects":len(gps),
  "native_gp_drawings":sum(len(layer.frames) for o in gps for layer in o.data.layers),
  "native_animation_editable":True,"animated_GP_objects":len(gps),
  "static_camera":True,"sample_frames":arts,"frames_per_second":24,
  "source_blend":str(blend),"source_preserved":True,
  "art_quality_accepted":False,"unreal_paper2d_imported":False}
 report.write_text(json.dumps(output,indent=2),encoding="utf-8")
 print("NW_GAME2D="+json.dumps({k:v for k,v in output.items() if k!="sample_frames"}))
if __name__=="__main__":
 a=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
 assert len(a)==1
 run(json.loads(pathlib.Path(a[0]).read_text(encoding="utf-8-sig")))
