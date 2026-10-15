"""Evidence side-by-side actual native render from unchanged original and art-directed derivative."""
from PIL import Image,ImageDraw,ImageFont
import os,json
BASE=os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
V=os.path.join(BASE,"VALIDATION")
src=os.path.join(V,"phase2","NW_ParametricSanctuary.png")
new=os.path.join(V,"phase4","NW_StarfallSanctum_Beauty.png")
assert os.path.isfile(src) and os.path.isfile(new)
a=Image.open(src).convert("RGB");b=Image.open(new).convert("RGB")
ww,hh=700,444
out=Image.new("RGB",(ww*2+30,hh+75),(22,27,36))
for i,(im,label) in enumerate(((a,"BEFORE - Workbench engineering prototype"),(b,"AFTER - Cycles layered art-directed study"))):
 resized=im.resize((ww,hh),Image.Resampling.LANCZOS)
 out.paste(resized,(10+i*(ww+10),50))
 ImageDraw.Draw(out).text((14+i*(ww+10),18),label,fill=(236,239,244))
path=os.path.join(V,"phase4","NW_Sanctuary_RenderComparison.png")
out.save(path)
jsonfile=os.path.join(V,"phase4","visual_comparison.json")
with open(jsonfile,"w") as f:json.dump({"old_path":src,"new_path":new,"comparison_path":path,
"same_subject_same_legacy_base_mesh":True,"identical_camera":False,
"identical_render_engine":False,"artist_accepted":False},f,indent=2)
print("COMPARISON_IMAGE="+path)
