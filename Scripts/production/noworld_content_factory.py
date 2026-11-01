#!/usr/bin/env python3
"""NoWorld 5-star content factory: native source -> original runtime asset workflow.

The Windows worker executes installed Koikatsu/KKBP, Blender, UE Python,
and existing shared catalog binder in that order. Inputs have immutable SHA256,
real receipts, stage ledger and safe resumption. NO authoring approval is
automatically invented. Not a replacement for Unreal or accepted content.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import traceback

REPO=Path(__file__).resolve().parents[2]
STAGES=("card_export","blender","unreal","motion_bind","artwork")
SHA=re.compile(r"^[0-9a-f]{64}$")
IDENT=re.compile(r"^[a-z0-9][a-z0-9_.-]{2,80}$")
SCHEMA="noworld.factory.job.v1"

def digest(path):
    h=hashlib.sha256()
    with Path(path).open("rb") as f:
        for block in iter(lambda:f.read(4*1024*1024),b""): h.update(block)
    return h.hexdigest()

def read_json(path):
    return json.loads(Path(path).read_text(encoding="utf-8-sig"))

def save(path,value):
    p=Path(path)
    p.parent.mkdir(parents=True,exist_ok=True)
    temp=p.with_suffix(p.suffix+".tmp")
    temp.write_text(json.dumps(value,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    os.replace(temp,p)

def required_file(path,hash_value=None):
    p=Path(path).resolve(strict=True)
    if not p.is_file(): raise ValueError("Expected a real file: "+str(p))
    if hash_value:
        if not isinstance(hash_value,str) or not SHA.fullmatch(hash_value.lower()):
            raise ValueError("Expected actual SHA-256")
        if digest(p)!=hash_value.lower(): raise ValueError("Immutable source SHA256 differs: "+str(p))
    return p

def config_file(value):
    p=Path(value)
    return p if p.is_absolute() else (REPO/p)

def process(args, *, env=None, cwd=None, timeout=1800):
    # No shell, no injection, no inherited writes into private source folders.
    args=[str(x) for x in args]
    print("FACTORY_STAGE_EXEC",args[0],Path(args[1]).name if len(args)>1 else "",flush=True)
    subprocess.run(args,env=dict(os.environ,**(env or {})),cwd=cwd,
                   timeout=timeout,check=True)

def editor(cfg,mode,jobpath,stage,receipt,timeout):
    exe=required_file(cfg["unreal_editor_cmd"])
    project=required_file(cfg["unreal_project"])
    script=REPO/"Scripts/production/noworld_unreal_content_import.py"
    if not script.is_file(): raise ValueError("Missing native UE Python importer")
    process([exe,project,"-ExecutePythonScript="+str(script),
             "-unattended","-nop4","-nosplash"],
            env={"NOWORLD_FACTORY_JOB":str(jobpath),
                 "NOWORLD_FACTORY_STAGE":stage,
                 "NOWORLD_FACTORY_RECEIPT":str(receipt)},
            cwd=project.parent,timeout=timeout)

def run(jobpath,config,mode="run"):
    jobpath=Path(jobpath).resolve(strict=True)
    data=read_json(jobpath)
    if data.get("schema")!=SCHEMA: raise ValueError("Wrong 5-star job schema")
    ident=data.get("job_id")
    kind=data.get("kind")
    if not isinstance(ident,str) or not IDENT.fullmatch(ident) or kind not in ("character","motion","artwork","bundle"):
        raise ValueError("Invalid job ID/type")
    if not isinstance(data.get("sources"),list) or not data["sources"]:
        raise ValueError("Real hashed inputs are required")
    source=[]
    for obj in data["sources"]:
        p=required_file(obj["path"],obj["sha256"])
        source.append({"role":obj["role"],"path":str(p),"sha256":digest(p)})
    ledger_root=Path(config["work_root"]).resolve()
    # Job names are restricted, and outputs never replace the original card.
    work=ledger_root/ident
    work.mkdir(parents=True,exist_ok=True)
    status=work/"status.json"
    signature=hashlib.sha256(json.dumps({"job":data,"source":source},
           sort_keys=True).encode()).hexdigest()
    if status.exists():
        state=read_json(status)
        if state.get("signature")!=signature:
            raise ValueError("Job inputs changed: use a new ID; source history is immutable")
    else:
        state={"schema":"noworld.factory.status.v1","job_id":ident,
               "signature":signature,"completed":[],"results":{},
               "state":"QUEUED","runtime_verified":False,
               "device_verified":False,"owner_accepted":False}
    save(status,state)
    def stage(name,fn,output=None,needed=True):
        if not needed: return
        if name in state["completed"]:
            if output and not Path(output).is_file():
                raise ValueError("Missing prior stage receipt "+name)
            return
        state["state"]="RUNNING_"+name.upper()
        save(status,state)
        fn()
        if output and (not Path(output).is_file() or Path(output).stat().st_size==0):
            raise RuntimeError(name+" returned without source receipt")
        if output:
            result=read_json(output)
            if result.get("status") in ("BLOCKED","PARTIAL_SOURCE_EXPORT","FAILED") or not result.get("status"):
                raise RuntimeError(name+" created a failure/partial source receipt, not a completed real asset")
        state["completed"].append(name)
        state["results"][name]=str(output or "DONE")
        save(status,state)
    try:
        native=data.get("native",{})
        artwork=data.get("artwork",[])
        motions=data.get("motions_recipe")
        timeout=int(config.get("stage_timeout_seconds",3600))
        if not 120<=timeout<=21600: raise ValueError("Invalid stage timeout")
        if kind in ("character","bundle"):
            card=next((a for a in source if a["role"]=="koikatsu_card"),None)
            if not card: raise ValueError("Character job requires original Koikatsu card")
            if not data.get("identity") or not data.get("version"):
                raise ValueError("Character requires exact canonical Identity and Version")
            export=work/"card_export_receipt.json"
            request=work/"card_export_request.tsv"
            if not request.exists():
                request.write_text("\t".join([
                    ident,str(card["path"]),card["sha256"],str(export)])+"\n",encoding="utf-8")
            def card_export():
                # Existing KKBP/BepInEx Maker API service reads an exact-card job
                # from a local job inbox, never from title/window click automation.
                queue=Path(config["koikatsu_queue"]).resolve()
                queue.parent.mkdir(parents=True,exist_ok=True)
                target=queue/(ident+".tsv")
                if target.exists() and target.read_bytes()!=request.read_bytes():
                    raise ValueError("Existing Koikatsu request changed")
                if not target.exists(): target.write_bytes(request.read_bytes())
                launched=None
                exit_flag=queue/"NOWORLD_FACTORY_EXIT_AFTER_BATCH.flag"
                # In dedicated worker mode, the API plugin closes only the
                # Koikatsu instance started by this job. Never terminate an
                # existing user Maker process or interact with its window.
                if not export.exists() and config.get("launch_koikatsu_worker",False):
                    exe=required_file(config["koikatsu_exe"])
                    exit_flag.write_text(ident+"\n",encoding="utf-8")
                    launched=subprocess.Popen([str(exe)],cwd=str(exe.parent))
                deadline=time.monotonic()+timeout
                while not export.exists() and time.monotonic()<deadline:
                    time.sleep(1)
                if not export.exists():
                    if launched is not None and launched.poll() is None:
                        launched.terminate()
                    raise RuntimeError("Koikatsu Maker export service unavailable or missing required mods; receipt absent")
                # A source receipt is more important than the author's
                # transient game window; remove our launch marker afterward.
                if launched is not None:
                    try: launched.wait(timeout=30)
                    except subprocess.TimeoutExpired:
                        if launched.poll() is None: launched.terminate()
                    if exit_flag.is_file(): exit_flag.unlink()
                result=read_json(export)
                if result.get("source_card_sha256")!=card["sha256"] or result.get("status")!="EXPORTED_NATIVE_EXACT_ORIGINAL_CARD":
                    raise RuntimeError("Maker did not export this exact authenticated card")
                required_file(result["pmx_path"],result["pmx_sha256"])
            stage("card_export",card_export,export)
            blend=work/"blender_receipt.json"
            def blender():
                process([required_file(config["blender_exe"]),"-b","--factory-startup",
                    "--python",REPO/"Scripts/production/noworld_kk_blender_generic.py",
                    "--",str(jobpath),str(export),str(blend),str(work/"blender")],
                    timeout=timeout)
            stage("blender",blender,blend)
            receipt=work/"unreal_receipt.json"
            stage("unreal",lambda:editor(config,"unreal",jobpath,"character",receipt,timeout),receipt)
        if kind in ("motion","bundle") or motions:
            if not motions:
                raise ValueError("Motion jobs need original approved motions_recipe")
            m=config_file(motions)
            required_file(m)
            stage("motion_bind",lambda:process([
                 sys.executable,REPO/"Scripts/production/bind_native_motion.py",m],
                 timeout=timeout))
        if artwork:
            if not isinstance(artwork,list): raise ValueError("artwork must be source asset list")
            for asset in artwork:
                required_file(asset["path"],asset["sha256"])
            receipt=work/"artwork_receipt.json"
            stage("artwork",lambda:editor(config,"artwork",jobpath,"artwork",receipt,timeout),receipt)
        state["state"]="SOURCE_ASSETS_INTEGRATED_NOT_PLAYABILITY_ACCEPTED"
        state["runtime_verified"]=False
        state["device_verified"]=False
        state["owner_accepted"]=False
        save(status,state)
        print(json.dumps({"job_id":ident,"status":state["state"],
                          "completed":state["completed"],"ledger":str(status)}))
        return 0
    except Exception as error:
        state["state"]="BLOCKED"
        state["error"]=str(error)
        state["traceback"]=traceback.format_exc()[-4000:]
        save(status,state)
        print(json.dumps({"job_id":ident,"status":"BLOCKED","error":str(error),
                          "ledger":str(status)}),file=sys.stderr)
        return 1

def main():
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument("--job",help="Fully sourced immutable job JSON")
    a.add_argument("--inbox",help="Process every prepared job in the inbox")
    a.add_argument("--config",default="ContentFactory/windows_worker.json")
    args=a.parse_args()
    cfg=read_json(config_file(args.config))
    if args.job and args.inbox: a.error("Select either --job or --inbox")
    if args.job: return run(args.job,cfg)
    if args.inbox:
        jobs=sorted(Path(args.inbox).glob("*.json"))
        if not jobs: raise ValueError("No jobs found")
        codes=[run(job,cfg) for job in jobs]
        return 1 if any(codes) else 0
    a.error("Specify --job or --inbox")

if __name__=="__main__": sys.exit(main())
