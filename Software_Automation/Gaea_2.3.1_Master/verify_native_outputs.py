from pathlib import Path
from PIL import Image,ImageChops,ImageStat
import json,struct,hashlib,statistics
r=Path(r"D:\UnrealProjects\NW_Gaea_Terrain_Sandbox_20261015\Unified_Master_Run_20261015")
source=r/"NW_Gaea_Input_Height16.png"
native=r/"exports"/"NW_Height.png"
for p in (source,native,r/"exports"/"NW_Color.png"):
 img=Image.open(p); print("PNG",p.name,"mode",img.mode,"dimensions",img.size,
  "png_bitdepth",p.read_bytes()[24],"range",img.getextrema(),"sha",hashlib.sha256(p.read_bytes()).hexdigest()[:16])
im0=Image.open(source).convert("I");im1=Image.open(native).convert("I")
if im0.size==im1.size:
 import numpy as np
 a=np.asarray(im0,dtype=np.int64);b=np.asarray(im1,dtype=np.int64)
 delta=np.abs(a-b)
 print("NATIVE_DIFFERENCE","changed_pixels",int(np.count_nonzero(delta)),
  "max_difference",int(delta.max()),"mean_difference",round(float(delta.mean()),3),
  "native_equal_source",source.read_bytes()==native.read_bytes())
for name in ("Out","Flow","Wear","Deposits"):
 p=r/"exports"/("NaturalHydraulicErosion_"+name+".exr")
 print("EXR",name,"bytes",p.stat().st_size,"magic",p.read_bytes()[:4].hex())
report=json.loads((r/"exports"/"report.json").read_text())
print("GAEA_RESULT",report["Result"],"NODE_COUNT",report["NodeCount"])
