"""NoWorld software capability dispatcher for Gates 01/02/09/12.
No gameplay implementation and no canonical content edits.
Calls existing tested SM01, SM02, Gaea and UE scripts. FAIL-CLOSED receipts.
"""
import pathlib,subprocess,json,sys,hashlib,datetime,time,argparse,re,os,uuid
ROOT=pathlib.Path(__file__).resolve().parents[1]
HERE=pathlib.Path(__file__).resolve().parent
BLENDER=ROOT/"Blender_5.2.2"
UE=ROOT/"Unreal_5.8.3_Master"
GAEA=ROOT/"Gaea_2.3.1_Master"
PY=pathlib.Path(r"D:\Tools\Python311\python.exe")
BX=pathlib.Path(r"D:\Tools\Blender-5.2.2-LTS\blender-5.2.2-windows-x64\blender.exe")
UEEXE=pathlib.Path(r"D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe")
UEPROJECT=pathlib.Path(r"D:\UnrealProjects\NW_BlenderInterop_Sandbox_20261015\NW_BlenderInterop.uproject")
SM01=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME")
SM01_SANDBOX=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014")
GATES={"Gate01","Gate02","Gate09","Gate12","Gate00"}
ALLOWED={
 "blender":{"api_search","architecture","morphology","weapon","semantic_motion","fight_mocap","grease_2d","kk_obj"},
 "gaea":{"graph","native_build","doctor"},
 "unreal":{"capabilities","asset_import","animation_validate","pcg_fixture"},
 "koikatsu":{"recipe_dry_run","joint_wing_offline"}}
def sha(p):
 h=hashlib.sha256()
 with open(p,"rb") as f:
  for part in iter(lambda:f.read(1<<20),b""):h.update(part)
 return h.hexdigest()
def safe_id(x):
 if not re.fullmatch("[A-Za-z0-9_-]{3,90}",x):raise ValueError("Invalid job id")
 return x
def validate(job):
 if job.get("schema")!="NOWORLD_SOFTWARE_JOB_V1":raise ValueError("Unknown schema")
 safe_id(job["job_id"])
 if job.get("owner_gate") not in GATES:raise ValueError("Unrecognized Gate")
 if job.get("test_only") is not True:raise ValueError("Only test_only jobs supported in software mastery")
 s=job.get("software");op=job.get("operation")
 if s not in ALLOWED or op not in ALLOWED[s]:raise ValueError("Unsupported software/op")
 req=job.get("requirements",{})
 if not req.get("preserve_originals",False):raise ValueError("Source preservation required")
 if int(req.get("max_walltime_seconds",0))<10:raise ValueError("Time limit required")
 return job
def command(job):
 o=job["operation"];s=job["software"];p=job.get("params",{})
 if s=="blender":
  if o=="api_search":
   return [str(PY),str(BLENDER/"MASTER_API"/"search_methods.py"),
    "--query",str(p["query"]),"--limit",str(p.get("limit",8))]
  elif o=="kk_obj":
   return [str(PY),str(BLENDER/"INTEGRATIONS"/"KK_BRIDGE"/"submit_kk_job.py"),
       "--source",str(p["source"]),"--id",str(p["native_job_id"])]
  elif o in ("architecture","morphology","weapon","semantic_motion","fight_mocap","grease_2d"):
   scripts={"architecture":"GATE02/architecture_job.py",
    "morphology":"GATE01/morphology_job.py","weapon":"GATE01/weapon_job.py",
    "semantic_motion":"GATE09/semantic_motion_job.py",
    "fight_mocap":"GATE09/combat_capture_job.py",
    "grease_2d":"GATE09/grease_game2d_job.py"}
   return [str(BX),"--background","--threads","2","--python",str(BLENDER/scripts[o]),
     "--",str(p["spec"])]
 if s=="koikatsu":
  if o=="recipe_dry_run":
   return [str(PY),str(SM01/"AUTOMATION"/"produce_from_reference_recipe.py"),
       "--recipe",str(p["recipe"])]
  if o=="joint_wing_offline":
   return [str(PY),str(BLENDER/"INTEGRATIONS"/"SM01_SM02"/"sm01_pack_native_8bone.py"),
       str(p["workdir"])]
 if s=="gaea":
  if o=="graph":return [str(PY),str(GAEA/"gaea_graph_job.py"),str(p["spec"])]
  if o=="native_build":return [str(PY),str(GAEA/"test_real_native_build.py")]
  if o=="doctor":return [str(PY),"-m","gaea_mcp.doctor"]
 if s=="unreal":
  scripts={"capabilities":"probe_unreal_master_subsystems.py",
    "asset_import":"qa_batch_import.py",
    "animation_validate":"qa_animation_validate.py",
    "pcg_fixture":"qa_pcg_procedural.py"}
  script=UE/scripts[o]
  return [str(UEEXE),str(UEPROJECT),"-run=pythonscript","-script="+str(script),
     "-nullrhi","-unattended","-nosplash","-nop4","-NoSound"]
 raise ValueError("Unhandled job")
def run(job,preflight=False):
 validate(job)
 out=pathlib.Path(job["outputs"]["receipt_directory"]).resolve()
 if out.exists():raise FileExistsError("Refuse existing receipt folder "+str(out))
 out.mkdir(parents=True)
 src_hashes={}
 for x in job.get("source_paths",[]):
  path=pathlib.Path(x).resolve()
  if not path.is_file():raise FileNotFoundError(path)
  src_hashes[str(path)]=sha(path)
 cmd=command(job)
 if not pathlib.Path(cmd[0]).is_file():raise FileNotFoundError(cmd[0])
 missing=[x for x in cmd if x.endswith(".py") and not pathlib.Path(x).is_file()]
 if missing:raise FileNotFoundError(str(missing))
 provenance={"schema":"NOWORLD_SOFTWARE_JOB_RECEIPT_V1","job_id":job["job_id"],
   "owner_gate":job["owner_gate"],"software":job["software"],
   "operation":job["operation"],"start_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
   "source_hashes":src_hashes,"command":cmd,"status":"PREFLIGHT_PASS" if preflight else "RUNNING",
   "artist_acceptance":False,"canonical_content_modified":False}
 (out/"submitted_job.json").write_text(json.dumps(job,indent=2))
 if not preflight:
  env=os.environ.copy()
  sdk=r"D:\Tools\DotNet8SDK_NoWorld_Portable"
  env["DOTNET_ROOT"]=sdk;env["PATH"]=sdk+os.pathsep+env.get("PATH","")
  env["GAEA_ROOT"]=r"D:\Tools\Gaea 2"
  t=time.perf_counter()
  try:
   p=subprocess.run(cmd,capture_output=True,text=True,encoding="utf-8",errors="replace",
     timeout=int(job["requirements"]["max_walltime_seconds"]),env=env)
   provenance["status"]="PROCESS_EXIT_ZERO" if p.returncode==0 else "PROCESS_FAILED"
   provenance["returncode"]=p.returncode
   (out/"stdout.log").write_text(p.stdout,encoding="utf-8")
   (out/"stderr.log").write_text(p.stderr,encoding="utf-8")
  except subprocess.TimeoutExpired as e:
   provenance["status"]="TIMED_OUT"
   (out/"timeout.json").write_text(json.dumps({"seconds":e.timeout}))
  provenance["execution_seconds"]=round(time.perf_counter()-t,3)
 bad=[name for name,old in src_hashes.items() if sha(name)!=old]
 if bad:
  provenance["status"]="SOURCE_MODIFIED"
  provenance["source_protection_violations"]=bad
 provenance["ended_utc"]=datetime.datetime.now(datetime.timezone.utc).isoformat()
 (out/"receipt.json").write_text(json.dumps(provenance,indent=2))
 print("NW_UNIFIED_GATE="+json.dumps({"job":job["job_id"],"status":provenance["status"],"receipt":str(out/"receipt.json")}))
 return provenance
def main():
 p=argparse.ArgumentParser();p.add_argument("--job",required=True);p.add_argument("--preflight",action="store_true")
 a=p.parse_args()
 row=run(json.loads(pathlib.Path(a.job).read_text(encoding="utf-8-sig")),a.preflight)
 if row["status"] not in ("PREFLIGHT_PASS","PROCESS_EXIT_ZERO"):sys.exit(2)
if __name__=="__main__":main()
