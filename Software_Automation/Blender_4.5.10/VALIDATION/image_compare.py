"""Compare actual source vs batched export preview pixels; exclude unverifiable visual parity claims."""
import os,sys,json,time
from PIL import Image,ImageChops,ImageStat
path=os.path.join(os.path.dirname(__file__),"phase2")
a=Image.open(os.path.join(path,"NW_ParametricSanctuary.png")).convert("RGB")
b=Image.open(os.path.join(path,"NW_ParametricSanctuary_Batched.png")).convert("RGB")
assert a.size==b.size
delta=ImageChops.difference(a,b)
stat=ImageStat.Stat(delta)
box=delta.getbbox()
res={"resolution":list(a.size),
  "mean_abs_rgb":[round(v,5) for v in stat.mean],
  "max_abs_difference":max(channel[1] for channel in stat.extrema),
  "pixel_identical":not bool(box),
  "changed_bounding_box":box,"artistic_equality_accepted":False}
with open(os.path.join(path,'batched_visual_diff.json'),'w') as f:json.dump(res,f,indent=2)
print('NW_VISUAL_DIFF='+json.dumps(res))
