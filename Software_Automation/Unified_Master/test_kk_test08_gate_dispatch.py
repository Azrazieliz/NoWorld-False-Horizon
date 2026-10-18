"""Offline Test08 native gate dispatch QA. No Maker, no cards, no artistic claim."""
import copy,json,pathlib,tempfile
from PIL import Image,ImageDraw
import kk_test08_gate_dispatch as t
checks={}
def check(k,fun):
 try:
  fun();checks[k]="PASS"
 except Exception as e:checks[k]=repr(e)[:260]
def require(cond,msg):
 if not cond:raise AssertionError(msg)
def refuses(fun):
 try:fun()
 except (ValueError,RuntimeError,FileNotFoundError):return
 raise AssertionError("Unsafe candidate was admitted")
with tempfile.TemporaryDirectory(prefix="nw_kk_t08_gate_") as td:
 root=pathlib.Path(td)
 im=Image.new("RGB",(640,960),(255,255,255))
 draw=ImageDraw.Draw(im);draw.ellipse((220,40,410,280),fill=(120,30,38))
 draw.rectangle((200,260,450,710),fill=(25,25,36))
 ref=root/"source_original.png";im.save(ref)
 boxes={"hair":[.23,.01,.72,.32],"face":[.35,.12,.66,.34],
   "eyes":[.38,.19,.63,.30],"body":[.24,.29,.78,.75],
   "main_outfit":[.22,.32,.79,.84],
   "sleeves":None,"legs":None,"footwear":None}
 annotation={"schema":"NW_KK_TEST08_USER_GROUNDED_FEATURES_V1",
   "benchmark_id":"TEST08","view":"front","reference_sha256":t.sha(ref),
   "features":boxes}
 ann=root/"regions.json";ann.write_text(json.dumps(annotation))
 native={"schema":"NOWORLD_NATIVE_VISUAL_SWEEP_V1","reference":str(ref),
   "view":"front","restore_baseline":[{"op":"set_hair_palette",
     "args":{"index":0,"value":.5,"v2":.2,"v3":.3}}],
   "candidates":[{"label":"candidate","steps":[{"op":"set_hair_palette",
     "args":{"index":0,"value":.7,"v2":.2,"v3":.3}}]}]}
 plan=root/"native_trials.json";plan.write_text(json.dumps(native))
 check("original_gate_code_unchanged",lambda:require(
   t.status()["original_gate_files_verified"]==4,"Base gate checksum differs"))
 def valid():
  result,b,masked,sweep=t.prepare(ref,ann,plan)
  require(len(result["verified_features"])==5,"Expected five explicit critical features")
  require(masked is None and sweep["view"]=="front","Wrong masked/plan state")
 check("reference_and_feature_regions_validate",valid)
 check("missing_source_rejected",lambda:refuses(
   lambda:t.prepare(root/"missing.png",ann,plan)))
 def bad_hash():
  a=dict(annotation,reference_sha256="deadbeef")
  path=root/"hash.json";path.write_text(json.dumps(a))
  refuses(lambda:t.prepare(ref,path,plan))
 check("original_source_hash_enforced",bad_hash)
 def weak():
  a=copy.deepcopy(annotation);a["features"].pop("face")
  path=root/"weak.json";path.write_text(json.dumps(a))
  refuses(lambda:t.prepare(ref,path,plan))
 check("unverified_feature_box_not_substituted",weak)
 def hidden():
  a=copy.deepcopy(annotation);a["features"]["face"]=None
  path=root/"hidden.json";path.write_text(json.dumps(a))
  refuses(lambda:t.prepare(ref,path,plan))
 check("mandatory_absent_feature_rejected",hidden)
 def mismatch():
  a=dict(native,view="face")
  path=root/"camera.json";path.write_text(json.dumps(a))
  refuses(lambda:t.prepare(ref,ann,path))
 check("camera_view_mismatch_rejected",mismatch)
 def fake_plan():
  a=dict(native,reference=str(root/"other.png"))
  path=root/"fake.json";path.write_text(json.dumps(a))
  refuses(lambda:t.prepare(ref,ann,path))
 check("plan_reference_substitution_rejected",fake_plan)
 def complex_background():
  bg=Image.new("RGB",(640,960),(39,35,34))
  draw=ImageDraw.Draw(bg);draw.rectangle((190,110,460,850),fill=(170,80,58))
  src=root/"dark.png";bg.save(src)
  a=dict(annotation,reference_sha256=t.sha(src))
  path=root/"dark_regions.json";path.write_text(json.dumps(a))
  refuses(lambda:t.prepare(src,path,plan))
 check("unmasked_dark_scene_never_scored_as_character",complex_background)
 def mask_case():
  bg=Image.new("RGB",(640,960),(39,35,34))
  draw=ImageDraw.Draw(bg);draw.rectangle((190,110,460,850),fill=(170,80,58))
  src=root/"dark2.png";bg.save(src)
  a=dict(annotation,reference_sha256=t.sha(src))
  path=root/"dark2_regions.json";path.write_text(json.dumps(a))
  p=dict(native,reference=str(src));pfile=root/"dark2_trials.json";pfile.write_text(json.dumps(p))
  mask=Image.new("L",(640,960),0);ImageDraw.Draw(mask).rectangle((190,110,460,850),fill=255)
  maskpath=root/"mask.png";mask.save(maskpath)
  result,_,masked,_=t.prepare(src,path,pfile,maskpath)
  require(masked is not None and masked[0,0,0]==255,"No valid cutout")
  require(masked[450,300,0]!=255,"Figure incorrectly whitened")
 check("actual_same_resolution_binary_mask_accepted",mask_case)
 require_all=all(v=="PASS" for v in checks.values())
 print("TEST08_GATE_DISPATCH_OFFLINE_QA="+json.dumps({"passed":sum(v=="PASS" for v in checks.values()),
 "total":len(checks),"checks":checks,"maker_edited":False,"character_generated":False},sort_keys=True))
 if not require_all:raise SystemExit(2)
