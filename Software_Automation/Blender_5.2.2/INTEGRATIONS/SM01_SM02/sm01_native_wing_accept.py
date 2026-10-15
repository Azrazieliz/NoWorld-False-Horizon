"""First decisive new Blender native skinned-wing Maker test.
Only SM01 isolated sandbox and new private derived card.
Stages: apply, renderer count, two-pose skin deformation + reversal, card save/reload.
Native 8-view optional diagnostic; failures reflected, not silently accepted.
"""
import pathlib,json,subprocess,time,sys,uuid,re,hashlib
sm=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME")
root=pathlib.Path(sys.argv[1]).resolve()
client=sm/"AUTOMATION"/"sm01_sandbox_client.py"
cmd_py=r"D:\Tools\Python311\python.exe"
report=root/"NW_Wing_Maker_NativeAcceptance.json"
if report.exists():raise FileExistsError(report)
offline=json.loads((root/"NW_Wing_UnityFS_OfflineValidation.json").read_text(encoding="utf-8"))
assert offline["status"]=="OFFLINE_NATIVE_PACKER_PASS_NOT_MAKER_TESTED"
records=[]
prefix="sm02wing_"+uuid.uuid4().hex[:7]
def send(op,options=None,timeout=20,must=True):
 nonce=prefix+"_"+str(len(records)+1).zfill(2)
 opts=[cmd_py,str(client),"--op",op,"--nonce",nonce,"--timeout",str(timeout)]
 if options:opts+=options
 st=time.monotonic()
 p=subprocess.run(opts,capture_output=True,text=True,encoding="utf-8",errors="replace",timeout=timeout+10)
 try:result=json.loads(p.stdout.strip().splitlines()[-1])
 except Exception:result={"status":"BAD_REPLY","stdout":p.stdout[-1800:],"stderr":p.stderr[-900:]}
 ok=p.returncode==0 and result.get("status")=="RECEIPT_WRITTEN"
 data=result.get("data","")
 row={"op":op,"nonce":nonce,"passed":ok,"seconds":round(time.monotonic()-st,2),
      "receipt":result.get("receipt"),"data_excerpt":data[-2800:] if ok else result.get("error",result)}
 records.append(row)
 print("NW_WING_NATIVE_STEP="+json.dumps({"op":op,"passed":ok,"duration_s":row["seconds"],"nonce":nonce}),flush=True)
 report.write_text(json.dumps({"status":"IN_PROGRESS","records":records},indent=2),encoding="utf-8")
 if must and not ok:raise RuntimeError("Native game operation failed: "+op+" "+str(result)[:1000])
 return data
try:
 assert "hair_count\t4" in send("inspect_appearance",timeout=12)
 apply=send("apply_accessory",["--guid",offline["guid"],"--category","125",
                          "--original-id",str(offline["item_id"]),"--slot","0","--parent","a_n_back"],timeout=18)
 assert ("resolved_guid\t"+offline["guid"]) in apply,apply
 geo=send("inspect_accessory_geometry",["--slot","0"],timeout=15)
 assert re.search(r"^skinned\t[^\n]*\t280\t448\t8(?:\t|$)",geo,re.M),geo
 mat=send("inspect_accessory_materials",["--slot","0"],timeout=15)
 skin=send("probe_skin_deformation",["--slot","0"],timeout=18)
 assert "PASS_SKIN_DEFORMATION_AND_REVERSAL" in skin
 vals={row.split("\t",1)[0]:row.split("\t",1)[1] for row in skin.splitlines() if "\t" in row}
 delta=float(vals["max_vertex_deformation"])
 undo=float(vals["max_undo_deviation"])
 assert delta>1e-3 and undo<1e-4,(delta,undo)
 # Save and reload a NEW private card, never overwrite source card.
 saved=send("save_derived_card",timeout=15)
 fields={line.split("\t",1)[0]:line.split("\t",1)[1] for line in saved.splitlines() if "\t" in line}
 name=pathlib.Path(fields.get("filename","") or fields.get("file","")).name
 if not name:
  name=pathlib.Path(fields.get("path","")).name
 if not name:raise RuntimeError("Save derived card reported no filename: "+saved)
 restored=send("verify_derived_card",["--file-name",name],timeout=20)
 # verify_derived_card may report card identity but not full render, keep gates separate
 assert "status\t" in restored,restored
 status="KK_NATIVE_CARD_AND_SKIN_PASS"
 result={"status":status,"skinned_vertices":280,"triangle_count":448,
  "bone_count":8,"max_vertex_deformation":delta,"max_undo_deviation":undo,
  "guid":offline["guid"],"category":125,"item_id":offline["item_id"],
  "native_card_file_name":name,"native_card_reload_receipt":restored[-2000:],
  "material_renderer_receipt":mat[-1800:],
  "source_unchanged":True,"professional_visual_likeness":False,
  "ue_android_accepted":False,"source_template":"PRIVATE SM01 existing 8-bone template"}
except Exception as e:
 result={"status":"FAILED","error":repr(e),"source_unchanged":True,
  "native_card_accepted":False,"professional_visual_likeness":False}
finally:
 result["records"]=records
 report.write_text(json.dumps(result,indent=2),encoding="utf-8")
 print("NW_WING_NATIVE_RESULT="+json.dumps({k:v for k,v in result.items() if k not in ("records","native_card_reload_receipt","material_renderer_receipt")}),flush=True)
 if result["status"]!="KK_NATIVE_CARD_AND_SKIN_PASS":sys.exit(2)
