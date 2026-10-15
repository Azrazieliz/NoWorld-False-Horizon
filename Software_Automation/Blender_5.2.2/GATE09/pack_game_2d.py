"""Pack native Grease Pencil key drawings into game-ready RGBA spritesheet.
Creates 7 key sprite samples + frame timing metadata; NOT manual painting 24fps.
"""
from PIL import Image,ImageDraw,ImageChops
from pathlib import Path
import json,hashlib
ROOT=Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015")
frames=ROOT/"NW_GP_2D_TransparentFrames"
paths=sorted(frames.glob("NW_2D_*.png"))
assert len(paths)==7,len(paths)
imgs=[Image.open(p).convert("RGBA") for p in paths]
size=imgs[0].size
assert all(im.size==size for im in imgs)
w,h=size
cols=4;rows=2
atlas=Image.new("RGBA",(w*cols,h*rows),(0,0,0,0))
data=[]
source_frames=[1,8,16,24,32,40,48]
for i,(p,im) in enumerate(zip(paths,imgs)):
 assert im.getchannel("A").getextrema()[0]==0,"Transparent background is required"
 # Trim only transparent outer part? Keep uniform frames to avoid shifting effect pose.
 xy=(i%cols*w,i//cols*h)
 atlas.alpha_composite(im,xy)
 data.append({"i":i,"source_frame":source_frames[i],
   "rect":list(xy)+[w,h],"offset_seconds":(source_frames[i]-1)/24,
   "key_pose_source":p.name})
out=ROOT/"NW_GP_Game_2D_SpriteAtlas.png"
if out.exists():raise FileExistsError(out)
atlas.save(out)
meta={"format":"RGBA PNG","source":"Grease Pencil v3 editable .blend",
 "atlas":str(out),"width":w*cols,"height":h*rows,"sprite_width":w,
 "sprite_height":h,"keyframes":data,"sampled_frames":len(data),
 "source_duration_seconds":2.0,"no_manually_painted_intermediate_frames":True,
 "runtime_target":"Unreal Paper2D flipbook / Niagara sprite subimages",
 "pixel_alpha_range":atlas.getchannel("A").getextrema(),
 "hash_sha256":hashlib.sha256(out.read_bytes()).hexdigest(),
 "native_unreal_imported":False,"art_quality_accepted":False}
(ROOT/"NW_GP_Game_2D_SpriteAtlas.json").write_text(json.dumps(meta,indent=2))
print("NW_2D_ATLAS="+json.dumps(meta))
