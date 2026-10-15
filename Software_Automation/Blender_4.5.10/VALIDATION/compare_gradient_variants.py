"""Pixel audit of subtle vs deliberately strong gradient, same resolution and camera."""
from PIL import Image, ImageChops, ImageStat
import json, os
ROOT=r"D:\UnrealProjects\NW_Blender_Private_Review_20261015"
files={
 'subtle':'NW_KK_ChikaHaruno_AdaptiveMaterials_PRIVATE.png',
 'global':'NW_KK_ChikaHaruno_GlobalGradients_PRIVATE.png',
 'strong':'NW_KK_ChikaHaruno_StrongGradient_Diagnostic.png'}
images={k:Image.open(os.path.join(ROOT,v)).convert('RGB') for k,v in files.items()}
assert len({im.size for im in images.values()})==1
regions={'hair':(190,40,320,195),'skirt':(160,275,340,405),'background':(0,0,100,100)}
result={'image_size':images['subtle'].size,'compared':{}}
for key in ['global','strong']:
 diff=ImageChops.difference(images['subtle'],images[key])
 stats=ImageStat.Stat(diff)
 region={}
 for name,box in regions.items():
  rg=ImageStat.Stat(diff.crop(box))
  region[name]={'mean_abs_rgb':[round(v,3) for v in rg.mean],'max_difference':max(e[1] for e in rg.extrema)}
 result['compared'][key]={'mean_abs_rgb':[round(v,3) for v in stats.mean],'global_max_difference':max(x[1] for x in stats.extrema),'region':region}
out=os.path.join(ROOT,'adaptive_gradient_visual_qc.json')
with open(out,'w',encoding='utf-8') as f:json.dump(result,f,indent=2)
print('NW_GRADIENT_QC='+json.dumps(result))
