"""Unreal 5.8.3 native Paper2D 2D animation asset creation using Blender GP frames.
Existing transparent PNG samples -> real Texture2D + PaperSprite + PaperFlipbook.
No camera movement substitute; actual 2D frame images and GameRuntime flipbook.
"""
import unreal,json,pathlib,traceback,os
QA=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015")
job=json.loads(os.environ.get("NW_PAPER2D_JOB_PARAMS","{}"))
frames=pathlib.Path(job.get("frames_dir",str(QA/"NW_GP_2D_TransparentFrames"))).resolve()
out=pathlib.Path(job.get("receipt",str(QA/"UE58_Paper2D_GP_NativeFlipbook_v2.json"))).resolve()
prefix=job.get("destination","/Game/NW_MasterHub_QA/NativeGP2D_20261015")
if not prefix.startswith("/Game/NW_MasterHub_QA/"):
 raise ValueError("Paper2D source must import only into sandbox")
fps=int(job.get("fps",24))
run=int(job.get("frame_run",8))
if fps<1 or fps>120 or run<1 or run>120:raise ValueError("Invalid 2D timing")
if out.exists():raise FileExistsError("Refuse overwrite evidence: "+str(out))
report={"engine":unreal.SystemLibrary.get_engine_version(),"status":"START","sprites":[]}
try:
 images=sorted(frames.glob("NW_2D_*.png"))
 assert 2<=len(images)<=100,len(images)
 unreal.EditorAssetLibrary.make_directory(prefix)
 tools=unreal.AssetToolsHelpers.get_asset_tools()
 sprites=[]
 for i,path in enumerate(images):
  texture_path=prefix+"/"+path.stem
  tex=unreal.load_asset(texture_path)
  if tex is None:
   task=unreal.AssetImportTask()
   task.filename=str(path);task.destination_path=prefix;task.automated=True
   task.save=True;task.replace_existing=False
   opts=unreal.TextureFactory();task.options=opts
   tools.import_asset_tasks([task])
   tex=next((unreal.load_asset(x) for x in task.imported_object_paths if unreal.load_asset(x)),None)
  if not tex or tex.get_class().get_name()!="Texture2D":
   raise RuntimeError("Texture import failed "+str(path))
  asset_path=prefix+"/NW_GP_KeySprite_"+str(i).zfill(2)
  sprite=unreal.load_asset(asset_path)
  if sprite is None:
   sprite=tools.create_asset("NW_GP_KeySprite_"+str(i).zfill(2),prefix,unreal.PaperSprite,unreal.PaperSpriteFactory())
   assert sprite
   sprite.set_editor_property("source_texture",tex)
   sprite.set_editor_property("source_uv",unreal.Vector2D(0,0))
   sprite.set_editor_property("source_dimension",unreal.Vector2D(384,384))
   unreal.EditorAssetLibrary.save_loaded_asset(sprite)
  reloaded=unreal.load_asset(sprite.get_path_name())
  assert reloaded and reloaded.get_editor_property("source_texture").get_path_name()==tex.get_path_name()
  sprites.append(reloaded)
  report["sprites"].append({"source_png":str(path),"sprite":reloaded.get_path_name(),"texture":tex.get_path_name()})
 if unreal.EditorAssetLibrary.does_asset_exist(prefix+"/NW_GP_2D_KeyframeFlipbook"):raise FileExistsError
 flip=tools.create_asset("NW_GP_2D_KeyframeFlipbook",prefix,unreal.PaperFlipbook,unreal.PaperFlipbookFactory())
 assert flip
 keys=[]
 for sprite in sprites:
  key=unreal.PaperFlipbookKeyFrame()
  key.set_editor_property("sprite",sprite)
  key.set_editor_property("frame_run",run)
  keys.append(key)
 flip.set_editor_property("frames_per_second",fps)
 flip.set_editor_property("key_frames",keys)
 unreal.EditorAssetLibrary.save_loaded_asset(flip)
 reopened=unreal.load_asset(flip.get_path_name())
 framecount=reopened.get_num_frames()
 assert framecount==len(images)*run,framecount
 rkeys=reopened.get_editor_property("key_frames")
 assert len(rkeys)==len(images)
 report.update(status="PASS_NATIVE_PAPER2D_FLIPBOOK",flipbook=reopened.get_path_name(),
  native_key_sprites=len(rkeys),num_animation_frames=framecount,
  frames_per_second=fps,source_type="Blender Grease Pencil v3",
  production_game_modified=False,art_approval=False)
except Exception:
 report["status"]="ERROR"
 report["error"]=traceback.format_exc()
out.write_text(json.dumps(report,indent=2))
print("NW_PAPER2D_REAL="+json.dumps({k:v for k,v in report.items() if k not in ("sprites","error")}))
