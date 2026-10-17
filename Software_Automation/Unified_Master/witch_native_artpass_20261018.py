"""Real native Koikatsu witch artistic correction pass: deep plum/teal, cloak, tails."""
from __future__ import annotations
import pathlib,shutil,json,hashlib,io,contextlib,uuid
import kk_native_visual_search as sweep
import kk_one_session as one
from PIL import Image, ImageEnhance
import numpy as np
OUT=pathlib.Path(r"D:\UnrealProjects\NW_Blender_Private_Review_20261017\WITCH_20261018_NATIVE_BUILD")
submit=sweep.client_factory()
one.profile_valid();one.game_guards(stage=False)
events=[]
def edit(op,args=None,required=False):
 try:
  nonce,secs,receipt=sweep.invoke(submit,{"op":op,"args":args or {}},timeout=16)
  result={"op":op,"args":args,"ok":True,"sec":secs,"data":receipt.get("data","")[-450:]}
 except Exception as exc:
  result={"op":op,"args":args,"ok":False,"err":str(exc)[:700]}
  if required:raise
 events.append(result)
 print(("OK" if result["ok"] else "FAIL"),op,(args or {}),flush=True)
 return result
def capture(name,view="front"):
 n,_,_=sweep.invoke(submit,{"op":"render_camera_view","args":{"group":view}},timeout=15)
 src=one.RUNTIME/"camera_bank"/("SM01_CAM_"+n+"_"+view+".png")
 target=OUT/(name+"_"+view+".png")
 if target.exists():raise FileExistsError(target)
 shutil.copy2(src,target)
 print("ARTPASS_IMAGE",target,flush=True)
 return str(target)
def recolor_skirt():
 # Native MaterialEditor UV persistent: retain folds and structure, recolor the red plaid.
 req={"group":"clothing","slot":1,"file_name":"o_bot_skirt06_a","parent":"cf_m_top_tennis_d"}
 r=edit("export_surface_texture",req)
 if not r["ok"]:return
 import re
 t=re.search(r"source_png\t([^\r\n]+)",r["data"])
 if not t: print("NO_EXPORTED_PATH");return
 src=pathlib.Path(t.group(1))
 if not src.is_file():return
 im=Image.open(src).convert("RGBA")
 a=np.array(im,dtype=np.uint8)
 # Deep violet/plum using source grayscale folds; preserve alpha and geometric details.
 rgb=a[:,:,:3].astype(np.float32)
 v=(.23*rgb[:,:,0]+.65*rgb[:,:,1]+.12*rgb[:,:,2])/255.
 hi=np.max(rgb,axis=-1)/255.
 strength=np.clip(.70*v+.30*hi,0,1)
 # Slanted subtle dark trim pattern instead of red check plaid.
 base=np.array([67,47,82],dtype=np.float32)
 a[:,:,:3]=np.uint8(np.clip(base[None,None,:]*(.42+.86*strength[...,None]),0,255))
 target=one.RUNTIME/"surface_edits"/("witch_slit_skirt_plum_"+uuid.uuid4().hex[:6]+".png")
 target.parent.mkdir(parents=True,exist_ok=True);Image.fromarray(a).save(target)
 for rend in ["o_bot_skirt06_a","o_bot_skirt06_b"]:
  edit("apply_surface_texture",{"group":"clothing","slot":1,"file_name":rend,
    "parent":"cf_m_top_tennis_d","recipe_file":target.name})
def main():
 # deep violet textile: main top of dark queen
 for name,mat,rgb in [
   ("o_top_a","bodytight",(.31,.20,.35)),
   ("o_bot_a","bodytight",(.30,.19,.35)),
   ("o_belt_a","item_01",(.52,.43,.41)),
   ("o_skirt","skirt",(.27,.18,.34)),
   ("o_top_b","bodytight",(.30,.19,.34)),
   ("o_bot_b","bodytights_destoryed",(.31,.20,.35))]:
  edit("set_surface_color",{"group":"clothing","slot":0,"file_name":name,
    "parent":mat,"value":rgb[0],"v2":rgb[1],"v3":rgb[2],"v4":1.0})
 recolor_skirt()
 # Fix incorrect twin-tail skull silhouette; use fitted long rear hair shape.
 edit("set_hair",{"index":0,"effective_id":19})
 # Base long head elements deliberately two tones.
 for idx,col in enumerate(((.075,.105,.24),(.09,.12,.23),(.08,.53,.62),(.08,.14,.28))):
  edit("set_hair_palette",{"index":idx,"value":col[0],"v2":col[1],"v3":col[2]})
 # White front hair comes from actual mod renderer not the native hair RGB.
 edit("set_surface_color",{"group":"accessory","slot":4,"file_name":"Hair Base",
    "parent":"main","value":.05,"v2":.43,"v3":.52,"v4":1})
 # Glasses model in base was bulky white-goggle. Try small default native spectacles.
 edit("apply_accessory_native",{"slot":1,"effective_id":2,"category":123,
    "parent":"a_n_head"})
 # Hat needs scalp contact and a wider asymmetric brim.
 edit("set_accessory_vector",{"slot":0,"group":"scale","value":1.63,"v2":1.32,"v3":1.49})
 edit("set_accessory_vector",{"slot":0,"group":"pos","value":0,"v2":-.18,"v3":0})
 edit("set_accessory_vector",{"slot":0,"group":"rot","value":0,"v2":0,"v3":-7})
 # Tail must project laterally, not thin straight down behind feet.
 edit("set_accessory_vector",{"slot":2,"group":"rot","value":0,"v2":0,"v3":60})
 edit("set_accessory_vector",{"slot":2,"group":"scale","value":1.18,"v2":1.20,"v3":1.18})
 # Book currently thin/orange from edge-on: rotate into the viewer.
 edit("set_accessory_vector",{"slot":3,"group":"rot","value":10,"v2":75,"v3":0})
 front=capture("ARTPASS_1","front")
 face=capture("ARTPASS_1","face")
 (OUT/"ARTPASS_1_EVENTS.json").write_text(json.dumps({"events":events,"images":[front,face]},indent=2))
if __name__=="__main__":main()
