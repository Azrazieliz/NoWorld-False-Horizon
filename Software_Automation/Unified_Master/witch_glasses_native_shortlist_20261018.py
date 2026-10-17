"""One-session actual Koikatsu rim-glasses visual shortlist, no save."""
import pathlib,io,json,contextlib,shutil
from PIL import Image,ImageDraw
import kk_native_visual_search as s
import kk_one_session as m
ROOT=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\WITCH_20261018_NATIVE_BUILD')
submit=s.client_factory();m.profile_valid();m.game_guards(stage=False)
pics=[];info=[]
for i in [1,3,5,7,9,12,14,18,22,26,30]:
 try:
  s.invoke(submit,{"op":"apply_accessory_native","args":{"slot":1,"effective_id":i,
      "category":123,"parent":"a_n_head"}},timeout=10)
  n,_,_=s.invoke(submit,{"op":"render_camera_view","args":{"group":"face"}},timeout=15)
  path=m.RUNTIME/"camera_bank"/f"SM01_CAM_{n}_face.png"
  im=Image.open(path).convert('RGB')
  # Eye+face central ROI is fixed for all candidates; avoid single global image score.
  crop=im.crop((165,530,615,900)).resize((270,222),Image.Resampling.LANCZOS)
  pics.append((i,crop));info.append({"native_id":i,"image":str(path),"success":True})
  print('GLASSES_RENDERED',i,flush=True)
 except Exception as exc:
  info.append({"native_id":i,"success":False,"error":str(exc)[:150]})
if not pics:raise SystemExit(2)
columns=4;rows=(len(pics)+columns-1)//columns
out=Image.new('RGB',(columns*274,rows*248),'white');d=ImageDraw.Draw(out)
for j,(i,im) in enumerate(pics):
 x=(j%columns)*274;y=(j//columns)*248
 out.paste(im,(x,y+20));d.text((x+8,y+2),f'Native eyeglasses {i}',fill=(0,0,0))
target=ROOT/'GLASSES_NATIVE_11_VISUAL_OPTIONS.png'
if target.exists():raise FileExistsError(target)
out.save(target)
(ROOT/'GLASSES_NATIVE_OPTIONS.json').write_text(json.dumps(info,indent=2))
print('GLASSES_CONTACT_SHEET',target,flush=True)
