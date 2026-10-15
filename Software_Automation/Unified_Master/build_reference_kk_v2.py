"""Generalizable native Koikatsu reference recipe builder. Single final save; visual acceptance separate.
Keeps original Steam untouched. All GUI-free edits occur via native sandbox API.
"""
import argparse,pathlib,subprocess,json,uuid,time,shutil,sys
ROOT=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME")
SB=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014")
CLI=ROOT/"AUTOMATION"/"sm01_sandbox_client.py"
def run(recipe,out):
 out.mkdir(parents=True,exist_ok=True); start=time.perf_counter();operations=[]
 def call(op,optional=False,**kw):
  n="nw"+uuid.uuid4().hex[:14]
  args=[sys.executable,str(CLI),"--op",op,"--nonce",n,"--timeout","14"]
  for k,v in kw.items():args+=["--"+k.replace("_","-"),str(v)]
  p=subprocess.run(args,capture_output=True,text=True,timeout=25,encoding="utf8",errors="replace")
  try:r=json.loads(p.stdout.strip().splitlines()[-1])
  except:raise RuntimeError("UNPARSEABLE "+op+" "+p.stdout[-700:])
  if p.returncode or r.get("status")!="RECEIPT_WRITTEN":
   if optional:
    print("SKIPPED",op,str(r)[-250:],flush=True);return None
   raise RuntimeError("NATIVE_FAILURE "+op+" "+str(r)[-800:])
  operations.append(op);print("APPLIED",op,round(time.perf_counter()-start,2),flush=True)
  return (r,n)
 def recolor(kind,slot,rgb):
  scan=call("inspect_surface_targets",optional=True,group=kind,slot=slot)
  if not scan:return 0
  seen=set();success=0
  for l in scan[0]["data"].splitlines():
   bits=l.split("\t")
   if len(bits)<4 or bits[0]!="target":continue
   ident=(bits[1],bits[2])
   if ident in seen:continue
   seen.add(ident)
   result=call("set_surface_color",optional=True,group=kind,slot=slot,file_name=bits[1],parent=bits[2],value=rgb[0],v2=rgb[1],v3=rgb[2],v4=1)
   if result:success+=1
  return success
 call("inspect_appearance")
 morphs=recipe.get("morphs")
 if morphs is None:
  morphs=[{"group":g,"index":i,"value":v} for g,values in (("face",recipe.get("face_shape",[])),("body",recipe.get("body_shape",[]))) for i,v in enumerate(values)]
 m=out/"morphs_for_reference.json";m.write_text(json.dumps(morphs))
 if morphs:call("apply_morph_batch",morphs=str(m))
 call("set_face_makeup")
 if "eye_iris" in recipe:call("set_eye_iris",value=recipe["eye_iris"][0],v2=recipe["eye_iris"][1],v3=recipe["eye_iris"][2])
 for h in recipe.get("hair",[]):call("set_hair",index=h["slot"],effective_id=h.get("runtime_id",h.get("native_id",0)))
 for h in recipe.get("hair",[]):
  if h.get("color"):recolor("hair",h["slot"],h["color"])
 for cl in recipe.get("clothing",[]):
  if cl["slot"]==0:call("set_clothes_top",guid=cl["guid"],category=cl["category"],original_id=cl["original_id"])
  else:call("set_clothes",optional=True,index=cl["slot"],effective_id=cl.get("runtime_id",0) if "runtime_id" in cl else 0,guid=cl.get("guid",""),category=cl.get("category",0),original_id=cl.get("original_id",0))
  if cl.get("color"):recolor("clothing",cl["slot"],cl["color"])
 for a in recipe.get("accessories",[]):
  if "effective_id" in a:
   call("apply_accessory_native",optional=a.get("optional",False),effective_id=a["effective_id"],category=a["category"],slot=a["slot"],parent=a["parent"])
  else:
   call("apply_accessory",optional=a.get("optional",False),guid=a["guid"],category=a["category"],original_id=a["original_id"],slot=a["slot"],parent=a["parent"])
  if a.get("color"):recolor("accessory",a["slot"],a["color"])
  transforms=a.get("transforms") or {k:a[k] for k in ("pos","rot","scale","pos2","rot2","scale2") if k in a}
  for group,v in transforms.items():
   if (group.startswith("scale") and all(abs(x-1)<.0001 for x in v)) or (not group.startswith("scale") and all(abs(x)<.0001 for x in v)):continue
   call("set_accessory_vector",optional=False,slot=a["slot"],group=group,value=v[0],v2=v[1],v3=v[2])
 call("render_views")
 saved,nonce=call("save_derived_card")
 fname="SM01_AUTOTEST_"+nonce+".png"
 verified=call("verify_derived_card",file_name=fname)
 if "PASS_NATIVE_CARD_RELOAD" not in verified[0]["data"]:raise RuntimeError("Card reload not verified")
 src=SB/"UserData"/"chara"/"female"/fname
 dest=out/(recipe["name"]+"_KK_Editable.png");shutil.copy2(src,dest)
 receipt={"schema":"NW_REF_TO_KK_NATIVE_V2","status":"NATIVE_KK_EDITABLE_CARD_VERIFIED","card":str(dest),"card_bytes":dest.stat().st_size,"elapsed_seconds":round(time.perf_counter()-start,2),"operations":operations,"visual_fidelity_accepted":False,"blender_polish_pending":True}
 (out/"receipt.json").write_text(json.dumps(receipt,indent=2))
 print("KK_NATIVE_CARD="+json.dumps(receipt),flush=True)
if __name__=="__main__":
 ap=argparse.ArgumentParser();ap.add_argument("--recipe",required=True);ap.add_argument("--out",required=True);v=ap.parse_args();run(json.loads(pathlib.Path(v.recipe).read_text()),pathlib.Path(v.out))
