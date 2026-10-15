"""NoWorld cross-Gate production dispatcher. No Blender GUI clicks or manual operations.
Executes one heavyweight job at a time (8 GiB PC) with source/derivative isolation.
Tasks: architecture, semantic_motion, kk_direct, blender_batch, api_search.
"""
from pathlib import Path
import argparse,json,subprocess,datetime,hashlib,os,sys,time,uuid
ROOT=Path(__file__).resolve().parents[1]
PYTHON=Path(r"D:\Tools\Python311\python.exe")
BLENDER=Path(r"D:\Tools\Blender-5.2.2-LTS\blender-5.2.2-windows-x64\blender.exe")
ROUTES={
 "architecture":("blender",ROOT/"GATE02"/"architecture_job.py"),
 "morphology":("blender",ROOT/"GATE01"/"morphology_job.py"),
 "weapon":("blender",ROOT/"GATE01"/"weapon_job.py"),
 "semantic_motion":("blender",ROOT/"GATE09"/"semantic_motion_job.py"),
 "fight_mocap":("blender",ROOT/"GATE09"/"combat_capture_job.py"),
 "grease_2d":("blender",ROOT/"GATE09"/"grease_game2d_job.py"),
 "kk_direct":("python",ROOT/"INTEGRATIONS"/"KK_BRIDGE"/"submit_kk_job.py"),
 "blender_batch":("python",ROOT/"CLI"/"job_dispatch.py"),
 "api_search":("python",ROOT/"MASTER_API"/"search_methods.py")}
def sha(path):
 h=hashlib.sha256()
 with open(path,"rb") as f:
  for b in iter(lambda:f.read(1<<20),b""):h.update(b)
 return h.hexdigest()
def build_command(job):
 kind=job["operation"]
 if kind not in ROUTES:raise ValueError("Unsupported Gate operation: "+str(kind))
 runtime,script=ROUTES[kind]
 if not script.is_file():raise FileNotFoundError(script)
 if runtime=="blender":
  spec=Path(job["spec"]).resolve()
  if not spec.is_file():raise FileNotFoundError(spec)
  cmd=[str(BLENDER),"--background","--threads","2","--python",str(script),"--",str(spec)]
 elif runtime=="python":
  cmd=[str(PYTHON),str(script)]
  if kind=="kk_direct":
   source=Path(job["source"]).resolve()
   if not source.is_file():raise FileNotFoundError(source)
   cmd+=["--source",str(source),"--id",job["id"]]
   if job.get("objects"):cmd+=["--objects",*list(job["objects"])]
  elif kind=="blender_batch":
   source=Path(job["job_spec"]).resolve()
   if not source.is_file():raise FileNotFoundError(source)
   cmd+=["--job",str(source)]
   if job.get("summary"):cmd+=["--summary",str(Path(job["summary"]).resolve())]
  elif kind=="api_search":
   cmd+=["--query",str(job["query"]),"--limit",str(job.get("limit",8))]
 else:raise RuntimeError("Unsupported runtime")
 if not Path(cmd[0]).is_file():raise FileNotFoundError(cmd[0])
 return cmd
def run(plan,dry=False):
 tasks=plan.get("jobs",[])
 if not isinstance(tasks,list) or not tasks or len(tasks)>50:
  raise ValueError("Need 1–50 explicit jobs")
 results=[]
 for i,job in enumerate(tasks):
  t=time.perf_counter();command=build_command(job)
  report={"index":i,"operation":job["operation"],"task_id":job.get("id",str(i)),
     "command":command,"passed":False}
  if dry:
   report.update(status="PREFLIGHT_PASS",passed=True)
  else:
   try:
    p=subprocess.run(command,capture_output=True,text=True,
       timeout=int(job.get("timeout_seconds",360)),encoding="utf-8",errors="replace")
    report.update(exit_code=p.returncode,stdout_tail=p.stdout[-6000:],
       stderr_tail=p.stderr[-1800:],status="PASS" if p.returncode==0 else "FAILED")
    report["passed"]=p.returncode==0
   except subprocess.TimeoutExpired:
    report["status"]="TIMEOUT"
  report["seconds"]=round(time.perf_counter()-t,3)
  results.append(report)
  if not report["passed"]:break
 return {"system":"NoWorld production command dispatcher","package":"Blender_5.2.2",
     "from_commit_required":False,"mode":"dry_run" if dry else "real",
     "timestamp_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
     "requested":len(tasks),"completed":len(results),"passed":sum(j["passed"] for j in results),
     "success":len(tasks)==len(results) and all(j["passed"] for j in results),
     "results":results,
     "certified_artist_quality":False}
def main():
 parser=argparse.ArgumentParser()
 parser.add_argument("--plan",required=True)
 parser.add_argument("--summary",required=True)
 parser.add_argument("--dry-run",action="store_true")
 args=parser.parse_args()
 source=Path(args.plan).resolve()
 outfile=Path(args.summary).resolve()
 if outfile.exists():raise FileExistsError("Refusing to overwrite summary "+str(outfile))
 results=run(json.loads(source.read_text(encoding="utf-8")),args.dry_run)
 outfile.parent.mkdir(parents=True,exist_ok=True)
 outfile.write_text(json.dumps(results,indent=2,ensure_ascii=False),encoding="utf-8")
 print("NW_MASTER_ROUTER="+json.dumps({"success":results["success"],
    "requested":results["requested"],"passed":results["passed"],
    "summary":str(outfile)}))
 if not results["success"]:sys.exit(2)
if __name__=="__main__":main()
