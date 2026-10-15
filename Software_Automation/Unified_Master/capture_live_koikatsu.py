"""One-command live Maker -> native card -> independent reload -> immutable private copy.
Uses existing SM01 v0.8.0 runtime; no mouse clicks or Steam installation writes.
"""
import argparse,datetime,hashlib,json,pathlib,subprocess,sys,uuid
ROOT=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation")
MASTER=ROOT/"MASTER_PACKAGES"/"KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME"
SANDBOX=ROOT/"SANDBOXES"/"SM01_KKParty_6436979_20261014"
CLIENT=MASTER/"AUTOMATION"/"sm01_sandbox_client.py"
HERE=pathlib.Path(__file__).resolve().parent
def op(name,opts=None):
 nonce="native_"+uuid.uuid4().hex[:18]
 args=[sys.executable,str(CLIENT),"--op",name,"--nonce",nonce,"--timeout","35"]
 for k,v in (opts or {}).items():args+=["--"+k,str(v)]
 proc=subprocess.run(args,capture_output=True,text=True,timeout=45)
 data=json.loads(proc.stdout.strip().splitlines()[-1])
 if proc.returncode or data.get("status")!="RECEIPT_WRITTEN":
  raise RuntimeError("SM01 native op "+name+" failed: "+proc.stdout[-500:])
 return nonce,data
def run(dest):
 dest=pathlib.Path(dest).resolve()
 if dest.exists():raise FileExistsError(dest)
 # Fail before using live Maker if sandbox is unavailable.
 if not CLIENT.is_file() or not (SANDBOX/"Koikatsu Party.exe").is_file():raise RuntimeError("SM01 sandbox unavailable")
 _,inspect=op("inspect_appearance")
 nonce,saved=op("save_derived_card")
 name="SM01_AUTOTEST_"+nonce+".png"
 src=SANDBOX/"UserData"/"chara"/"female"/name
 if not src.is_file():raise FileNotFoundError(src)
 _,verified=op("verify_derived_card",{"file-name":name})
 if "PASS_NATIVE_CARD_RELOAD" not in verified.get("data",""):
  raise RuntimeError("Native independent card reload rejected")
 import importlib.util
 s=importlib.util.spec_from_file_location("nw_intake",HERE/"intake_native_kk_card.py")
 m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
 record=m.ingest(src,dest)
 record.update(native_appearance_receipt=inspect["receipt"],native_save_receipt=saved["receipt"],
               native_reload_receipt=verified["receipt"],native_reload_pass=True)
 (dest/"intake_receipt.json").write_text(json.dumps(record,indent=2))
 return record
if __name__=="__main__":
 parser=argparse.ArgumentParser()
 parser.add_argument("--output",required=True)
 args=parser.parse_args()
 print("NW_LIVE_CHARACTER_CAPTURE="+json.dumps(run(args.output)))
