"""One native Maker session: choose best illustrated witch bust/waist geometry by previews."""
import pathlib,json,shutil
from PIL import Image,ImageDraw
import kk_native_visual_search as s
import kk_one_session as m
ROOT=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\WITCH_20261018_NATIVE_BUILD')
submit=s.client_factory();m.profile_valid();m.game_guards(stage=False)
candidates=[('Yelan_dark_cutout','Kraitx.Yelan.Mod',130732),
 ('Ningguang_long','com.nep.ningguangstuff',19511508),
 ('Magilou_witch','com.gf.clothes',8205575),
 ('Dragonic_corset','[Ω-G16] HS2-KK Set',15410),
 ('Lisa_witch','Kraitx.Lisa.Mod',130539),
 ('VoidQueen_dress','[Darobgy,Darobulance]VoidQueen_C2',52228)]
cards=[];events=[]
for label,guid,oid in candidates:
 try:
  s.invoke(submit,{"op":"set_clothes_top","args":{"guid":guid,"category":105,"original_id":oid}},timeout=12)
  n,_,_=s.invoke(submit,{"op":"render_camera_view","args":{"group":"front"}},timeout=14)
  path=m.RUNTIME/"camera_bank"/f"SM01_CAM_{n}_front.png"
  im=Image.open(path).convert("RGB").crop((250,370,670,765)).resize((315,300))
  cards.append((label,im))
  source=ROOT/(label+"_top_front.png")
  shutil.copy2(path,source)
  events.append({"label":label,"source":str(source),"success":True})
  print('TOP_RENDER',label,flush=True)
 except Exception as exc:
  print('TOP_FAIL',label,str(exc)[:180],flush=True)
  events.append({"label":label,"success":False,"error":str(exc)[:350]})
n=3;rows=(len(cards)+n-1)//n
image=Image.new('RGB',(n*319,rows*330),'white');draw=ImageDraw.Draw(image)
for j,(label,im) in enumerate(cards):
 x=(j%n)*319;y=(j//n)*330
 image.paste(im,(x,y+23));draw.text((x+5,y+5),label,fill=(0,0,0))
target=ROOT/'TOP_SIX_NATIVE_WITCH_OUTFIT_VISUALS.png'
if target.exists():raise FileExistsError(target)
image.save(target)
(ROOT/'TOP_NATIVE_WITCH_CANDIDATES.json').write_text(json.dumps(events,indent=2))
print('TOP_CONTACT',str(target),flush=True)
