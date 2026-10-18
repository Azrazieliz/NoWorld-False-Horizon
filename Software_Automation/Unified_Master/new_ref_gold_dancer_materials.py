"""Replace Dancer costume atlas colors on isolated Koikatsu runtime.
Preserve UV, alpha and source atlas; no arbitrary new mesh/polygon generation.
"""
from PIL import Image
from pathlib import Path
import numpy as np,hashlib,json
from scipy.ndimage import gaussian_filter
s=Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014\BepInEx\SM01_SANDBOX')
paths=[('skirt','DNC_TEX_SKIRT_20261017'),('top','DNC_TEX_TOP_20261017')]
receipt=[]
for k,tag in paths:
 src=s/'surface_sources'/('surface_'+tag+'.png')
 a=np.array(Image.open(src).convert('RGBA'));b=a.copy()
 rgb=a[...,:3].astype(float);r=rgb[:,:,0];g=rgb[:,:,1];bl=rgb[:,:,2]
 lum=.3*r+.59*g+.11*bl
 if k=='skirt':
  # Deep-red tartan fabric -> warm sheer ivory. Blur only color folds, not alpha or trim.
  red=(r>g*1.32)&(r>bl*1.25)&(r>60)
  blurred=gaussian_filter(lum.astype(np.float32),sigma=15)
  v=np.clip(223+(blurred-100)*.16,205,242)
  for ix,offset in enumerate((2,0,-9)):b[:,:,ix][red]=np.clip(v+offset,0,255)[red]
  # Embedded red gems -> celadon/turquoise, not red.
  red_jewel=red & (np.indices(red.shape)[1]>790)
  for ix,value in enumerate([28,169,153]):b[:,:,ix][red_jewel]=value
 else:
  # Gray blue halter bust -> ivory silk; keep turquoise gemstones/creases.
  blue=(bl>r*1.06)&(g>=r*.85)&(r<165)&(bl<215)
  blue=blue & (r>40)
  v=np.clip(219+(lum-115)*.30,188,241)
  for ix,offset in enumerate((13,8,-2)):b[:,:,ix][blue]=np.clip(v+offset,0,255)[blue]
  # preserve turquoise teardrop (blue-green but more saturated)
  teal=(g>r*1.38)&(bl>r*1.26)&(g>85)
  for ix,value in enumerate([29,173,168]):b[:,:,ix][teal]=value
 file='T06_'+k+'_ivory_gold_native_uv.png'
 dest=s/'surface_edits'/file;dest.parent.mkdir(parents=True,exist_ok=True)
 Image.fromarray(b,'RGBA').save(dest,optimize=True)
 changed=int(np.any(a!=b,axis=2).sum())
 receipt.append({'source':str(src),'copy':str(dest),'changed_pixels':changed,
  'sha256_original':hashlib.sha256(src.read_bytes()).hexdigest(),
  'sha256_variant':hashlib.sha256(dest.read_bytes()).hexdigest()})
 print('ATLAS',k,'modified_pixels',changed,'original unchanged',flush=True)
out=Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST06_WHITE_GOLD_DANCER')
(out/'T06_UV_COLOR_VARIANTS.json').write_text(json.dumps(receipt,indent=2))
