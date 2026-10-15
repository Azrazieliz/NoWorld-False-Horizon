#!/usr/bin/env python3
"""Fast zero-click command transport to the isolated runtime plugin only.
Never addresses user's original game; refuses non-sandbox path or an existing command.
"""
import argparse, pathlib, json, time, os, uuid, sys
SB=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014")
CH=SB/"BepInEx"/"SM01_SANDBOX"
def submit(data,seconds):
 if not SB.is_dir() or not (SB/"BepInEx"/"plugins"/"SM01RuntimeController.dll").is_file():
  raise RuntimeError("Authorized sandbox or runtime plugin absent")
 CH.mkdir(parents=True,exist_ok=True);(CH/"receipts").mkdir(exist_ok=True)
 target=CH/"command.json"
 if target.exists():raise RuntimeError("Refuse overwrite unprocessed command")
 nonce=data.get("nonce") or "sm01_"+uuid.uuid4().hex[:12]
 data["nonce"]=nonce
 if (CH/"receipts"/(nonce+".tsv")).exists() or (CH/"receipts"/("ERROR_"+nonce+".txt")).exists():
  raise RuntimeError("Refuse reusing a previous test nonce")
 draft=CH/("command_"+nonce+".tmp")
 draft.write_text(json.dumps(data,ensure_ascii=False,separators=(",",":"))+"\n",encoding="utf-8")
 os.replace(str(draft),str(target))
 receipt=CH/"receipts"/(nonce+".tsv")
 start=time.perf_counter()
 while time.perf_counter()-start<seconds:
  if receipt.exists():
   s=receipt.read_text(encoding="utf-8-sig")
   print(json.dumps({"status":"RECEIPT_WRITTEN","nonce":nonce,"elapsed_seconds":round(time.perf_counter()-start,3),"receipt":str(receipt),"data":s[:25000]},ensure_ascii=True),flush=True)
   return 0
  error_file=CH/"receipts"/("ERROR_"+nonce+".txt")
  if error_file.exists():
   print(json.dumps({"status":"ERROR_LOG","file":str(error_file),"error":error_file.read_text(encoding="utf-8-sig")[:6000]}),flush=True)
   return 2
  time.sleep(.35)
 print(json.dumps({"status":"TIMEOUT","nonce":nonce,"seconds":seconds,"still_pending":target.exists()}),flush=True)
 return 3
def main():
 p=argparse.ArgumentParser()
 p.add_argument("--op",choices=("snapshot","probe_morph","apply_morph_batch","undo_last_batch","inspect_accessory","apply_accessory","undo_last_accessory","save_derived_card","verify_derived_card","inspect_appearance","inspect_mod_resolution","set_eye_iris","set_face_makeup","kkbp_export_native","set_hair","set_clothes","set_clothes_top","set_coordinate","set_clothes_state","set_accessory_axis","set_accessory_vector","set_accessory_parent","inspect_accessory_pose","render_camera_bank","render_camera_view","inspect_camera_bank","inspect_surface_targets","set_surface_color","inspect_surface_shader_profile","set_surface_float","set_surface_shader","export_surface_texture","apply_surface_texture","revert_surface_texture","inspect_accessory_materials","inspect_accessory_geometry","probe_skin_deformation","render_isolated_views","render_views","set_bone_deformation","inspect_bone_runtime_pose","set_accessory_color","inspect_bones","set_bone_scale","undo_appearance"),required=True)
 p.add_argument("--index",type=int,default=0);p.add_argument("--value",type=float,default=.71);p.add_argument("--group",default="face")
 p.add_argument("--morphs",default="")
 p.add_argument("--nonce",default="")
 p.add_argument("--file-name",default="")
 p.add_argument("--recipe-file",default="")
 p.add_argument("--guid",default="")
 p.add_argument("--original-id",type=int,default=0)
 p.add_argument("--category",type=int,default=0)
 p.add_argument("--slot",type=int,default=0)
 p.add_argument("--effective-id",type=int,default=0)
 p.add_argument("--v2",type=float,default=0.0)
 p.add_argument("--v3",type=float,default=0.0)
 p.add_argument("--v4",type=float,default=1.0)
 p.add_argument("--parent",default="")
 p.add_argument("--timeout",type=float,default=90)
 a=p.parse_args()
 data={"op":a.op,"index":a.index,"value":a.value,"group":a.group,"nonce":a.nonce,
       "guid":a.guid,"originalId":a.original_id,"category":a.category,
       "accessorySlot":a.slot,"parentBone":a.parent,"effectiveItemId":a.effective_id,
       "value2":a.v2,"value3":a.v3,"value4":a.v4}
 if a.morphs:
  parts=json.loads(pathlib.Path(a.morphs).read_text(encoding="utf-8-sig"))
  if not isinstance(parts,list) or len(parts)<1 or len(parts)>96:raise ValueError("Invalid morph count")
  nonce=data.get("nonce") or "sm01_"+uuid.uuid4().hex[:12]
  data["nonce"]=nonce
  recipes=CH/"recipes";recipes.mkdir(parents=True,exist_ok=True)
  recipe_name="morph_"+nonce+".tsv"
  recipe_path=recipes/recipe_name
  if recipe_path.exists():raise ValueError("Refusing recipe overwrite")
  rows=[]
  for p in parts:
   group=p["group"];idx=p["index"];value=p["value"]
   if group not in ("face","body") or not isinstance(idx,int) or idx<0 or idx>=(52 if group=="face" else 44):
    raise ValueError("Invalid morph index/group")
   if not isinstance(value,(int,float)) or not (0<=value<=2):raise ValueError("Invalid float value")
   rows.append("%s\t%d\t%.9g"%(group,idx,value))
  recipe_path.write_text("\n".join(rows)+"\n",encoding="utf-8")
  data["recipeFile"]=recipe_name
 if a.file_name:data["fileName"]=a.file_name
 if a.recipe_file:data["recipeFile"]=a.recipe_file
 sys.exit(submit(data,a.timeout))
if __name__=="__main__":main()
