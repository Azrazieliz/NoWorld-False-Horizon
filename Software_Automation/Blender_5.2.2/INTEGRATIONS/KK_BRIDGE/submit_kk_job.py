"""Gate01/02 -> Blender 5.2 -> Koikatsu Studio asynchronous OBJ bridge.
No user clicking. Submits a source-preserving Blender job and an atomic import request.
Koikatsu BepInEx NoWorldBridge watches queue on Studio main thread.
"""
import pathlib,subprocess,hashlib,json,argparse,re,sys,datetime,os
BLENDER=pathlib.Path(r"D:\Tools\Blender-5.2.2-LTS\blender-5.2.2-windows-x64\blender.exe")
RUNNER=pathlib.Path(__file__).resolve().parents[2]/"CLI"/"gate_runner.py"
SANDBOX=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014").resolve()
ROOT_ENV=os.environ.get("NW_KK_GAME_ROOT","")
GAME=pathlib.Path(ROOT_ENV).resolve() if ROOT_ENV else None
def sha(p):
 h=hashlib.sha256()
 with p.open("rb") as f:
  for b in iter(lambda:f.read(1<<20),b""):h.update(b)
 return h.hexdigest()
def submit(src,job_id,objects=None):
 source=pathlib.Path(src).resolve()
 if not source.is_file() or source.suffix.lower()!=".blend":raise FileNotFoundError("Missing native .blend source: "+str(source))
 if not re.fullmatch(r"[A-Za-z0-9_-]{1,70}",job_id):raise ValueError("Use safe alphanumeric job id")
 if not BLENDER.is_file() or not RUNNER.is_file():raise FileNotFoundError("Missing Blender or master runner")
 if GAME is None or GAME!=SANDBOX:raise RuntimeError("Studio OBJ bridge requires exact authorized SM01 sandbox path via NW_KK_GAME_ROOT; original Steam is forbidden")
 root=GAME/"BepInEx"/"NoWorldBridge"
 assets=root/"assets";inbox=root/"inbox";events=root/"events"
 for p in (assets,inbox,events):p.mkdir(parents=True,exist_ok=True)
 obj=assets/(job_id+".obj")
 report=assets/(job_id+".export.json")
 ready=inbox/(job_id+".ready")
 tmppath=inbox/(job_id+".pending")
 for f in (obj,report,ready,tmppath,events/(job_id+".imported"),events/(job_id+".failed")):
  if f.exists():raise FileExistsError("Refusing duplicate job id or output: "+str(f))
 cmd=[str(BLENDER),"--background","--factory-startup","--threads","2",
      "--python",str(RUNNER),"--","--op","kk_obj","--source",str(source),
      "--output",str(obj),"--report",str(report)]
 if objects:cmd+=["--objects",*objects]
 run=subprocess.run(cmd,capture_output=True,text=True,timeout=240,encoding="utf-8",errors="replace")
 if run.returncode!=0:raise RuntimeError("Blender export failed: "+run.stderr[-1000:]+" "+run.stdout[-1600:])
 verify=json.loads(report.read_text(encoding="utf-8"))
 if not verify.get("source_unchanged"):raise RuntimeError("Blender source protection check failed")
 digest=sha(obj)
 if digest!=verify["result"]["sha256"]:raise RuntimeError("OBJ checksum mismatch")
 # UTF-8 no BOM, .ready published atomically after both OBJ and MTL exist.
 tmppath.write_text(str(obj)+"\n"+digest+"\n",encoding="utf-8")
 tmppath.replace(ready)
 res={"status":"QUEUED_FOR_KK_STUDIO","job":job_id,"source":str(source),
    "source_sha256":verify["source_sha256"],"source_unchanged":True,
    "assets_obj":str(obj),"asset_sha256":digest,"obj_bytes":obj.stat().st_size,
    "mesh_parts":verify["result"]["generated_mesh_parts"],"mtl":verify["result"]["mtl"],
    "ready":str(ready),"event_expected":str(events/(job_id+".imported")),
    "generated_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
    "accepted_in_kk":False,"art_quality_accepted":False}
 (assets/(job_id+".submission.json")).write_text(json.dumps(res,indent=2),encoding="utf-8")
 return res
def main():
 p=argparse.ArgumentParser()
 p.add_argument("--source",required=True)
 p.add_argument("--id",required=True)
 p.add_argument("--objects",nargs="*")
 opts=p.parse_args()
 print("NOWORLD_KK_BRIDGE_SUBMIT="+json.dumps(submit(opts.source,opts.id,opts.objects)))
if __name__=="__main__":main()
