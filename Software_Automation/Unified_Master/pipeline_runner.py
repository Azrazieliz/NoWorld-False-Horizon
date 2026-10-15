"""NoWorld production-tool workflow orchestrator: fully sequential, resumable,
source-hash-protected, fail-closed. Coordinates Blender/SM01/Gaea/Unreal gatectl jobs.
No automatic production game edits; expects test_only jobs and immutable sources.
"""
import argparse,pathlib,hashlib,json,datetime,sys,importlib.util,time,os
HERE=pathlib.Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location("nw_gatectl",HERE/"gatectl.py")
gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
def sha(raw):return hashlib.sha256(raw).hexdigest()
def now():return datetime.datetime.now(datetime.timezone.utc).isoformat()
def atomic_json(path,data):
 tmp=path.with_suffix(path.suffix+".tmp")
 tmp.write_text(json.dumps(data,indent=2),encoding="utf-8")
 tmp.replace(path)
def plan_validate(source):
 data=json.loads(source.read_text(encoding="utf-8-sig"))
 if data.get("schema")!="NOWORLD_SOFTWARE_PIPELINE_V1":raise ValueError("Unsupported pipeline schema")
 jobs=data.get("steps")
 if not isinstance(jobs,list) or not 1<=len(jobs)<=16:raise ValueError("Pipeline must have 1–16 steps")
 unique=[]
 for job in jobs:
  gate.validate(job)
  cmd=gate.command(job)
  exe=pathlib.Path(cmd[0])
  if not exe.is_file():raise FileNotFoundError(exe)
  for v in cmd[1:]:
   if v.lower().endswith(".py") and not pathlib.Path(v.split("=",1)[-1]).is_file():
    raise FileNotFoundError(v)
  key=str(pathlib.Path(job["outputs"]["receipt_directory"]).resolve()).lower()
  if key in unique:raise ValueError("Duplicate receipt path between steps")
  unique.append(key)
 return data
def main():
 p=argparse.ArgumentParser()
 p.add_argument("--plan",required=True)
 p.add_argument("--preflight",action="store_true")
 p.add_argument("--resume",action="store_true")
 args=p.parse_args()
 path=pathlib.Path(args.plan).resolve()
 plan=plan_validate(path)
 checkpoint=pathlib.Path(plan["checkpoint"]).resolve()
 plan_hash=sha(path.read_bytes())
 if args.preflight:
  print("NW_PIPELINE_PREFLIGHT="+json.dumps({"steps":len(plan["steps"]),"status":"PASS",
    "software":[j["software"] for j in plan["steps"]],"source_hash":plan_hash}))
  return
 if checkpoint.exists() and not args.resume:raise FileExistsError("Use --resume to continue a previous checkpoint")
 if not checkpoint.exists() and args.resume:raise FileNotFoundError("Cannot resume nonexistent checkpoint")
 if checkpoint.exists():
  state=json.loads(checkpoint.read_text(encoding="utf-8"))
  if state.get("plan_sha256")!=plan_hash:raise RuntimeError("Plan changed since checkpoint; refusing replay")
 else:
  checkpoint.parent.mkdir(parents=True,exist_ok=True)
  state={"schema":"NW_SOFTWARE_PIPELINE_CHECKPOINT_V1","plan_sha256":plan_hash,
   "started_utc":now(),"status":"RUNNING","steps":[]}
  atomic_json(checkpoint,state)
 lock=checkpoint.with_suffix(".lock")
 fd=os.open(str(lock),os.O_CREAT|os.O_EXCL|os.O_WRONLY)
 try:
  os.write(fd,json.dumps({"process":os.getpid(),"time":now()}).encode())
 finally:os.close(fd)
 try:
  for i,job in enumerate(plan["steps"]):
   if i<len(state["steps"]):
    prior=state["steps"][i]
    receipt=pathlib.Path(prior["receipt"])
    if prior.get("ok") and receipt.is_file() and sha(receipt.read_bytes())==prior["receipt_hash"]:
     print("NW_PIPELINE_RESUME_SKIP="+job["job_id"],flush=True)
     continue
    raise RuntimeError("Previous receipt invalid; stop instead of duplicating dangerous job")
   print("NW_PIPELINE_STAGE_BEGIN="+job["job_id"],flush=True)
   try:
    res=gate.run(job)
    receipt=pathlib.Path(job["outputs"]["receipt_directory"])/"receipt.json"
    success=res["status"] in ("NATIVE_VALIDATED","PROCESS_EXIT_ZERO")
    if "postconditions" in job and res["status"]!="NATIVE_VALIDATED":success=False
    entry={"step":i,"job_id":job["job_id"],"software":job["software"],
     "status":res["status"],"ok":success,"receipt":str(receipt),
     "receipt_hash":sha(receipt.read_bytes()) if receipt.exists() else None,
     "finished_utc":now()}
   except Exception as e:
    success=False
    entry={"step":i,"job_id":job["job_id"],"software":job["software"],
     "status":"EXCEPTION","ok":False,"error":repr(e),"finished_utc":now()}
   state["steps"].append(entry)
   state["status"]="RUNNING" if success else "STOPPED_AT_FAILURE"
   atomic_json(checkpoint,state)
   print("NW_PIPELINE_STAGE_END="+json.dumps(entry),flush=True)
   if not success:sys.exit(2)
  state["status"]="PASS_ALL_STAGES"
  state["ended_utc"]=now()
  atomic_json(checkpoint,state)
  print("NW_PIPELINE_COMPLETED="+json.dumps({"status":state["status"],
   "stages":len(state["steps"]),"checkpoint":str(checkpoint)}),flush=True)
 finally:
  lock.unlink(missing_ok=True)
if __name__=="__main__":main()
