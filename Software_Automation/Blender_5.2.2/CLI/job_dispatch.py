"""Unattended multi-operation dispatch for Gate 01/12; Python 3.11 standard library only.

python job_dispatch.py --job job.json [--dry-run]
Each job uses Blender 5.2.2, reports source SHA256, native result status and errors.
The user never needs to click Blender. Protected frozen Gate files never overwritten.
"""
import argparse,subprocess,json,os,sys,pathlib,hashlib,time,datetime,shutil
BASE=pathlib.Path(__file__).resolve().parents[1]
DEFAULT_BLENDER=pathlib.Path(r"D:\Tools\Blender-5.2.2-LTS\blender-5.2.2-windows-x64\blender.exe")
SCRIPT=BASE/"CLI"/"gate_runner.py"
OPS={"audit","gradient","look","static","skeletal","animation","kk_obj"}
def sha(p):
 h=hashlib.sha256()
 with open(p,"rb") as f:
  for c in iter(lambda:f.read(1<<20),b""):h.update(c)
 return h.hexdigest()
def dispatch(config,dry_run=False):
 blender=pathlib.Path(config.get("blender",str(DEFAULT_BLENDER))).resolve()
 if not blender.is_file():raise FileNotFoundError("No Blender executable: "+str(blender))
 jobs=config.get("jobs",[])
 if not isinstance(jobs,list) or not jobs:raise ValueError("JSON requires nonempty jobs array")
 outputs=set();results=[]
 for idx,job in enumerate(jobs):
  operation=job.get("op")
  if operation not in OPS:raise ValueError(f"Invalid job {idx} operation {operation}")
  src=pathlib.Path(job["source"]).resolve()
  if not src.is_file():raise FileNotFoundError(src)
  resultfile=pathlib.Path(job["report"]).resolve()
  paths=[resultfile]
  if operation!="audit":paths.append(pathlib.Path(job["output"]).resolve())
  if any(p==src for p in paths):raise ValueError("Refusing self overwrite: "+str(src))
  for p in paths:
   if p in outputs or p.exists():raise FileExistsError("Output collision: "+str(p))
   outputs.add(p)
  cmd=[str(blender),"--background","--factory-startup","--threads","2",
       "--python",str(SCRIPT),"--","--op",operation,"--source",str(src),
       "--report",str(resultfile)]
  if operation!="audit":cmd+=["--output",str(paths[1])]
  if job.get("object"):cmd+=["--object",str(job["object"])]
  if job.get("rig"):cmd+=["--rig",str(job["rig"])]
  if job.get("slot") is not None:cmd+=["--slot",str(job["slot"])]
  if job.get("strength") is not None:cmd+=["--strength",str(job["strength"])]
  if job.get("world"):cmd+=["--world"]
  if job.get("preset"):cmd+=["--preset",str(job["preset"])]
  if job.get("objects"):cmd+=["--objects",*list(job["objects"])]
  result={"job_id":str(job.get("id",idx)),"op":operation,"source":str(src),
    "start_sha256":sha(src),"status":"PENDING","dry_run":dry_run}
  if dry_run:
   result["command"]=cmd;result["status"]="PRECHECK_PASS"
  else:
   start=time.perf_counter()
   call=subprocess.run(cmd,capture_output=True,text=True,
       timeout=int(job.get("timeout_seconds",180)),encoding="utf-8",errors="replace")
   result["seconds"]=round(time.perf_counter()-start,2)
   result["exit_code"]=call.returncode
   result["stdout_tail"]=call.stdout[-3000:];result["stderr_tail"]=call.stderr[-900:]
   result["end_sha256"]=sha(src)
   if result["end_sha256"]!=result["start_sha256"]:
    result["status"]="SOURCE_CHANGED_UNEXPECTEDLY"
   elif call.returncode!=0:
    result["status"]="BLENDER_FAILURE"
   elif not resultfile.is_file():
    result["status"]="REPORT_MISSING"
   else:
    evidence=json.loads(resultfile.read_text(encoding="utf-8"))
    result["status"]="PASS" if evidence.get("source_unchanged") else "REPORT_NOT_PASSED"
    result["evidence_file"]=str(resultfile)
    if len(paths)>1 and not paths[1].exists():
     result["status"]="OUTPUT_MISSING"
   if result["status"]!="PASS":
    results.append(result)
    break
  results.append(result)
 overall={"runner_version":"1.0.0","blender":str(blender),
   "generated_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
   "dry_run":dry_run,"total_requested":len(jobs),"completed":len(results),
   "passed":sum(r["status"] in {"PASS","PRECHECK_PASS"} for r in results),
   "status":"PASS" if len(results)==len(jobs) and all(r["status"] in {"PASS","PRECHECK_PASS"} for r in results) else "FAILED",
   "results":results}
 return overall
def main():
 p=argparse.ArgumentParser()
 p.add_argument("--job",required=True)
 p.add_argument("--dry-run",action="store_true")
 p.add_argument("--summary")
 args=p.parse_args()
 config=json.loads(pathlib.Path(args.job).read_text(encoding="utf-8"))
 result=dispatch(config,args.dry_run)
 if args.summary:
  path=pathlib.Path(args.summary)
  if path.exists():raise FileExistsError(path)
  path.parent.mkdir(parents=True,exist_ok=True)
  path.write_text(json.dumps(result,indent=2),encoding="utf-8")
 print("NW_GATE_DISPATCH="+json.dumps(result))
 if result["status"]!="PASS":sys.exit(2)
if __name__=="__main__":main()
