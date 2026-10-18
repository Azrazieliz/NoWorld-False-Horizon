from PIL import Image
from pathlib import Path
import numpy as np,json,hashlib
root=Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014\BepInEx\SM01_SANDBOX')
o=Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST06_WHITE_GOLD_DANCER')
for name,source in [('hair','DNC_TAKANE_TEX_ACTUAL_20261017'),('crown','DNC_CROWN_TEX_20261017')]:
 src=root/'surface_sources'/('surface_'+source+'.png')
 a=np.asarray(Image.open(src).convert('RGBA')).copy()
 v=(.3*a[...,0]+.59*a[...,1]+.11*a[...,2]).astype(np.float32)
 alpha=a[...,3]
 if name=='hair':
  # Preserves original realistic strand highlights, shadows and opacity.
  for k,(offset,scale) in enumerate([(58,.80),(20,.75),(4,.53)]):
   a[...,k]=np.uint8(np.clip(offset+v*scale,0,255))
 else:
  sat=a[...,:3].max(axis=2)-a[...,:3].min(axis=2)
  silver=(sat<54)&(alpha>8)
  for k,(offset,scale) in enumerate([(58,.82),(24,.70),(8,.39)]):
   a[...,k][silver]=np.uint8(np.clip(offset+v*scale,0,255))[silver]
 dest=root/'surface_edits'/('T06_'+name+'_authentic_UV_gold.png')
 dest.parent.mkdir(parents=True,exist_ok=True)
 Image.fromarray(a,'RGBA').save(dest,optimize=True)
 print('EDITED',name,dest,'bytes',dest.stat().st_size,flush=True)
