"""Recoverable, serial UE 5.8 native intake of immutable Gate02 original
Eastern Market glTF source sections. This driver uses installed Python 3.11,
NOT PowerShell script execution policy bypass. Each section has its own native
StaticMesh, receipt, stdout log and resumable checkpoint.
"""
import os,sys,subprocess,hashlib,json,argparse
from pathlib import Path
ROOT=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
G2=Path(r"D:\NoWorld_Art_Production\NoWorld_FirstWorld_RegionKit_R1_20261019")
UE=Path(r"D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe")
UP=ROOT/"OfflineGame.uproject"
SCRIPT=ROOT/"Scripts/Gate12World/gate12_import_one_original_market_section_ue583.py"
DIR=ROOT/"Saved/Gate12Readiness/FirstWorldR1_MergedSource/eastern_market_original_object_shards"
arg=argparse.ArgumentParser()
arg.add_argument("--first",type=int,default=0)
arg.add_argument("--last",type=int,default=11)
opts=arg.parse_args()
if opts.first<0 or opts.last>11 or opts.first>opts.last:
 raise SystemExit("Expected 0<=first<=last<=11")
for p in (UE,UP,SCRIPT,DIR):assert p.exists(),p
m=json.loads((DIR/"eastern_market_original_object_shards_receipt.json").read_text(encoding="utf8"))
assert m["status"]=="ORIGINAL_EASTERN_MARKET_SOURCE_PRESERVED_12_FBXS_PENDING_NATIVE_UE"
assert m["source_sha256"]=="a6373a82b629b667714f4e39b741ff66450d8ece338bf5eaf72431148d489e3c"
ledger=DIR/"gate12_market_incremental_native_source_receipt.json"
current={"schema":"gate12.eastern_market_native_sections.incremental.v1",
         "project":str(ROOT),"source_sha256":m["source_sha256"],
         "status":"IN_PROGRESS","sections":[]}
def save():
 ledger.write_text(json.dumps(current,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
save()
for i in range(opts.first,opts.last+1):
 row=next(x for x in m["shards"] if x["section"]==i)
 f=Path(row["FBX"])
 assert f.is_file() and hashlib.sha256(f.read_bytes()).hexdigest()==row["fbx_sha256"]
 receipt=DIR/f"market_native_intake_section_{i:02d}.json"
 log=DIR/f"UE_market_section_{i:02d}.log"
 if receipt.exists():
  prior=json.loads(receipt.read_text(encoding="utf8"))
  if prior["status"]=="REAL_SOURCE_MARKET_SECTION_NATIVE_IMPORTED_NOT_MOBILE_OR_ART_APPROVED" and prior["source_fbx_sha256"]==row["fbx_sha256"] and prior["ue_triangles"]>=int(row["source_polygons"]*.93):
   current["sections"].append({"section":i,"status":"VERIFIED_EXISTING_ORIGINAL_SOURCE_NO_OVERWRITE","triangles":prior["ue_triangles"]})
   save()
   print("EASTERN_SECTION",i,"PRESERVED_VERIFIED",flush=True)
   continue
 env=os.environ.copy()
 env["GATE12_MARKET_SECTION"]=str(i)
 args=[str(UE),str(UP),"-run=pythonscript",f"-script={SCRIPT}",
       "-unattended","-nop4","-nosplash","-NoSound","-NullRHI","-log"]
 print("EASTERN_SECTION",i,"IMPORTING_GENUINE_GATE02_SOURCE",flush=True)
 with log.open("w",encoding="utf8",errors="replace") as output:
  done=subprocess.run(args,stdout=output,stderr=subprocess.STDOUT,
                      env=env,check=False)
 results=json.loads(receipt.read_text(encoding="utf8")) if receipt.exists() else {}
 text=log.read_text(encoding="utf8",errors="replace")
 bad=("LogInterchangeEngine: Error:" in text or
      "LogPython: Error:" in text or
      "LogInit: Display: Failure" in text)
 entry={"section":i,"exit_code":done.returncode,
        "status":results.get("status","NO_RECEIPT"),
        "ue_triangles":results.get("ue_triangles",0),
        "fatal_or_interchange_errors":bad}
 current["sections"].append(entry)
 save()
 print("EASTERN_SECTION",i,entry,flush=True)
 if done.returncode!=0 or bad or entry["status"]!="REAL_SOURCE_MARKET_SECTION_NATIVE_IMPORTED_NOT_MOBILE_OR_ART_APPROVED":
  current["status"]="HALTED_AT_FIRST_UNACCEPTABLE_NATIVE_SECTION"
  save()
  raise SystemExit(f"STOP_SECTION_{i:02d}; preserved prior assets, see {log}")
current["status"]="ALL_REQUESTED_SECTION_FILES_NATIVE_IMPORTED_SOURCE_ONLY_UNBUILT"
save()
print("GATE12_EASTERN_MARKET_NATIVE_BATCH_COMPLETE",opts.first,opts.last,flush=True)
