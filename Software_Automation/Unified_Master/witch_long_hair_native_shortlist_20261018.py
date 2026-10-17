"""Three actual native long-hair accessory alternatives for witch identity."""
import pathlib,json,shutil
from PIL import Image,ImageDraw
import kk_native_visual_search as s
import kk_one_session as m
ROOT=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\WITCH_20261018_NATIVE_BUILD')
submit=s.client_factory()
choices=[('Kushina_long','kushinahairpack.mod',221100),
         ('Minami_long','lrdj.minami_ssr_outfits',2),
         ('Anne_long','anne001',84210)]
imgs=[];rows=[]
for label,guid,oid in choices:
 try:
  s.invoke(submit,{"op":"apply_accessory","args":{"slot":4,"guid":guid,
      "category":122,"original_id":oid,"parent":"a_n_headtop"}},timeout=9)
  # New imported hair often starts white; tint base if supported.
  try:
   _,_,q=s.invoke(submit,{"op":"inspect_surface_targets","args":{"group":"accessory","slot":4}})
   targets=[x.split('\t') for x in q['data'].splitlines() if x.startswith('target\t')]
   for t in targets:
    if len(t)<3:continue
    try:s.invoke(submit,{"op":"set_surface_color","args":{"group":"accessory","slot":4,
          "file_name":t[1],"parent":t[2].split(' (Instance)')[0],
          "value":.05,"v2":.38,"v3":.49,"v4":1}},timeout=7)
    except Exception:pass
  except Exception:pass
  n,_,_=s.invoke(submit,{"op":"render_camera_view","args":{"group":"front"}},timeout=12)
  source=m.RUNTIME/"camera_bank"/f"SM01_CAM_{n}_front.png"
  raw=ROOT/(label+"_front.png")
  shutil.copy2(source,raw)
  tile=Image.open(raw).convert('RGB').crop((160,180,710,850)).resize((385,469))
  imgs.append((label,tile));rows.append({"name":label,"guid":guid,"id":oid,"capture":str(raw)})
  print('HAIR_CANDIDATE',label,flush=True)
 except Exception as exc:
  rows.append({"name":label,"error":str(exc)[:250]})
if imgs:
 out=Image.new('RGB',(len(imgs)*390,505),'white');d=ImageDraw.Draw(out)
 for j,(label,tile) in enumerate(imgs):
  out.paste(tile,(j*390,25));d.text((j*390+12,7),label,fill=(5,5,5))
 path=ROOT/'LONG_HAIR_REAL_VISUAL_CHOICES.png'
 if path.exists():raise FileExistsError(path)
 out.save(path)
 print('HAIR_CHOICES',path,flush=True)
(ROOT/'LONG_HAIR_NATIVE_SHORTLIST.json').write_text(json.dumps(rows,indent=2))
