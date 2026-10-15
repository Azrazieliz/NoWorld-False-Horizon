"""Real four-router smoke without opening heavy editors; unique receipts every run."""
import copy,json,pathlib,subprocess,sys,datetime
root=pathlib.Path(__file__).resolve().parent
example=json.loads((root/"PIPELINE_EXAMPLE_FOUR_SOFTWARE.json").read_text())
stamp=datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S%f")
out=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015")/("FreshSmoke_"+stamp)
example["checkpoint"]=str(out/"checkpoint.json")
for i,job in enumerate(example["steps"]):
 job["job_id"]="RealSmoke_"+stamp+"_"+str(i)
 job["outputs"]["receipt_directory"]=str(out/job["software"])
path=out/"plan.json"
out.mkdir(parents=True,exist_ok=False)
path.write_text(json.dumps(example,indent=2))
cmd=[sys.executable,str(root/"pipeline_runner.py"),"--plan",str(path)]
p=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
print(p.stdout,flush=True)
if p.stderr:print(p.stderr,file=sys.stderr)
if p.returncode:raise SystemExit(p.returncode)
resume=subprocess.run(cmd+["--resume"],capture_output=True,text=True,timeout=40)
print("RESUME_CHECK="+resume.stdout[-1200:],flush=True)
if resume.returncode:raise SystemExit(resume.returncode)
print("FOUR_ROUTE_SMOKE_PASS="+str(path),flush=True)
