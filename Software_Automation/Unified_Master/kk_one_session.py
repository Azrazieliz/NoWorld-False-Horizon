#!/usr/bin/env python3
"""NoWorld / Gate01: one-launch Koikatsu asset planning and low-overhead native batch.

Phases:
  1. PLAN   read-only indexed asset search; resolve all likely dependencies BEFORE launch.
  2. STAGE  explicitly copy hash-locked archives in a single PRE-LAUNCH action.
  3. EXECUTE never launch/restart Koikatsu; send many native ops from ONE Python process.
  4. REPORT record every operation's latency, utilization and regressions.

No source mods or original Steam Party files may be written. Production content is never
approved automatically. All actions default to planning only; staging/execution explicit.
"""
from __future__ import annotations
import argparse, hashlib, importlib.util, json, os, pathlib, re, shutil, sqlite3
import subprocess, sys, time, uuid

HERE = pathlib.Path(__file__).resolve().parent
PACKAGE = pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME")
SANDBOX = pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014")
LIBRARY = SANDBOX / "mods_LIBRARY_FULL_20261014"
SELECTED = SANDBOX / "mods" / "SM01_SELECTED"
PROFILE = SANDBOX / "BepInEx" / "SM01_PROFILE_STATUS.json"
DB = PACKAGE / "CATALOG" / "FULL_INSTALLED_MOD_LIBRARY.sqlite"
CLIENT = PACKAGE / "AUTOMATION" / "sm01_sandbox_client.py"
RUNTIME = SANDBOX / "BepInEx" / "SM01_SANDBOX"

ALLOWED_NATIVE_OPS = {
    "snapshot","probe_morph","apply_morph_batch","inspect_accessory",
    "apply_accessory","apply_accessory_native","inspect_appearance",
    "inspect_mod_resolution","set_eye_iris","set_iris_asset",
    "set_hair_palette","set_face_makeup","set_hair","set_clothes",
    "set_clothes_top","set_coordinate","set_clothes_state",
    "set_accessory_axis","set_accessory_vector","set_accessory_parent",
    "inspect_accessory_pose","render_camera_bank","render_camera_view",
    "inspect_camera_bank","inspect_surface_targets","set_surface_color",
    "inspect_surface_shader_profile","set_surface_float","set_surface_shader",
    "export_surface_texture","apply_surface_texture","inspect_accessory_materials",
    "inspect_accessory_geometry","probe_skin_deformation",
    "render_isolated_views","render_views","inspect_bones",
    "inspect_bone_runtime_pose","set_bone_deformation","set_accessory_color",
    "set_bone_scale"
}
SAVING_OPS = {"save_derived_card","verify_derived_card","kkbp_export_native"}
FORBIDDEN_OPS = {"undo_last_batch","undo_last_accessory","revert_surface_texture","undo_appearance"}
ARG_NAMES = {
  "original_id":"originalId","slot":"accessorySlot","parent":"parentBone",
  "effective_id":"effectiveItemId","v2":"value2","v3":"value3","v4":"value4",
  "file_name":"fileName","recipe_file":"recipeFile"
}
PARAMETERS = set(ARG_NAMES) | {"index","value","group","guid","category","morphs"}
VALID_CATEGORIES = {str(x) for x in range(100,201)}

def safe_name(x: str)->str:
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]{0,95}",x):
        raise ValueError("Unsafe QA session name")
    return x

def safe_rel(relative: str) -> pathlib.PurePosixPath:
    path=pathlib.PurePosixPath(relative)
    if path.is_absolute() or ".." in path.parts or ":" in relative or "\\" in relative:
        raise ValueError("Source package path escapes indexed library")
    return path

def file_sha(path: pathlib.Path) -> str:
    h=hashlib.sha256()
    with path.open("rb") as f:
        for b in iter(lambda:f.read(8<<20),b""):h.update(b)
    return h.hexdigest()

def profile_valid() -> bool:
    state=json.loads(PROFILE.read_text(encoding="utf-8-sig"))
    if state.get("now_profile")!="SELECTIVE":
        raise RuntimeError("Only the hash-locked SELECTIVE sandbox profile is permitted")
    return True

def running_games() -> list[str]:
    """Return exact executable paths. Fail closed if Windows process query fails."""
    command=[
      "powershell.exe","-NoProfile","-NonInteractive","-Command",
      "Get-CimInstance Win32_Process -Filter \"Name='Koikatsu Party.exe'\" | ForEach-Object {$_.ExecutablePath}"
    ]
    r=subprocess.run(command,capture_output=True,text=True,timeout=8)
    if r.returncode:raise RuntimeError("Cannot verify Koikatsu process state")
    return [x.strip() for x in r.stdout.splitlines() if x.strip()]

def game_guards(stage: bool=False) -> None:
    games=running_games()
    if stage and games:
        raise RuntimeError("Package staging only BEFORE first launch; STOP rather than restart an active user/session")
    if not stage:
        if len(games)!=1 or pathlib.Path(games[0]).resolve()!= (SANDBOX/"Koikatsu Party.exe").resolve():
            raise RuntimeError("Native batch requires ONE already-running isolated sandbox; will not launch/close any game")

def catalog_plan(terms:list[str],mandatory:list[str],top_per_term:int=12,
                 max_packages:int=130,max_bytes:int=9*1024**3,db: pathlib.Path=DB)->dict:
    """Fast readonly catalog lookup: all 27k+ assets remain searchable without game relaunch."""
    if not terms and not mandatory:raise ValueError("Specify benchmark features and/or pinned GUIDs")
    if any(not 2<=len(t.strip())<=80 for t in terms):raise ValueError("Invalid short/broad feature keyword")
    if not (1<=top_per_term<=100 and 1<=max_packages<=300 and max_bytes>0):
        raise ValueError("Invalid staging budget")
    con=sqlite3.connect("file:"+db.as_posix()+"?mode=ro",uri=True)
    con.row_factory=sqlite3.Row
    chosen={}; matched={}; unresolved=[]
    try:
        for guid in mandatory:
            rows=con.execute("SELECT guid,relative_path,name,size_bytes,asset_count FROM packages WHERE guid=?",(guid,)).fetchall()
            if len(rows)!=1:
                unresolved.append(guid);continue
            row=dict(rows[0]);row["why"]="pinned:"+guid
            chosen[row["relative_path"]]=row
        for term in terms:
            pattern="%"+term.replace("%","\\%").replace("_","\\_")+"%"
            rows=con.execute(
              """SELECT p.guid,p.relative_path,p.name,p.size_bytes,p.asset_count,
                 a.category_no,a.original_id,a.display_name
                 FROM assets a JOIN packages p ON p.id=a.package_id
                 WHERE lower(a.display_name) LIKE lower(?) ESCAPE '\\'
                 ORDER BY p.asset_count DESC,a.display_name LIMIT ?""",
                 (pattern,max(40,top_per_term*7))
            ).fetchall()
            records=[]
            for record in rows:
                r=dict(record)
                if len(records)<top_per_term:records.append(r)
                if r["relative_path"] not in chosen and len(chosen)<max_packages:
                    chosen[r["relative_path"]]={k:r[k] for k in ("guid","relative_path","name","size_bytes","asset_count")}
                    chosen[r["relative_path"]]["why"]="feature:"+term
                if len(records)>=top_per_term and len(chosen)>=max_packages:break
            matched[term]=records[:top_per_term]
    finally:con.close()
    pkg=list(chosen.values())
    total=sum(p["size_bytes"] for p in pkg)
    if total>max_bytes:raise RuntimeError("One-launch pool exceeds explicit disk/RAM budget; narrow feature set BEFORE boot")
    already=[]
    for p in pkg:
        rel=safe_rel(p["relative_path"])
        if (SELECTED.joinpath(*rel.parts)).is_file():already.append(p["relative_path"])
    return {"schema":"NW_KK_ONE_SESSION_POOL_V1","status":"READY" if not unresolved else "UNRESOLVED_REQUIRED_GUIDS",
        "terms":terms,"matched_samples":matched,"missing_mandatory_guids":unresolved,
        "packages":pkg,"package_count":len(pkg),"estimated_bytes":total,
        "already_selected_count":len(already),"new_packages_count":len(pkg)-len(already),
        "max_bytes":max_bytes,"full_index_retained":True,
        "one_launch_only":True,"mid_session_mod_restart_allowed":False,
        "source_install_untouched":True,"quality_not_approved":True}

def stage_plan(plan:dict, destination:pathlib.Path) ->dict:
    """Only a deliberate pre-launch command can mutate sandbox selected-mod copies."""
    if plan.get("schema")!="NW_KK_ONE_SESSION_POOL_V1" or plan.get("status")!="READY":
        raise ValueError("Unresolved dependencies; staging blocked")
    profile_valid()
    game_guards(stage=True)
    if destination.exists():raise FileExistsError("Immutable staging receipt already exists")
    if not LIBRARY.is_dir():raise FileNotFoundError(LIBRARY)
    records=[];start=time.perf_counter()
    for package in plan["packages"]:
        rel=safe_rel(package["relative_path"])
        src=LIBRARY.joinpath(*rel.parts);dst=SELECTED.joinpath(*rel.parts)
        if not src.is_file():raise FileNotFoundError(src)
        if src.stat().st_size!=package["size_bytes"]:raise RuntimeError("Source changed since catalog index")
        if dst.exists():
            if dst.stat().st_size!=src.stat().st_size or file_sha(dst)!=file_sha(src):
                raise RuntimeError("Selected mod changed; refuse replace")
            result="CACHED"
        else:
            dst.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(src,dst)
            if file_sha(src)!=file_sha(dst):
                dst.unlink(missing_ok=True)
                raise RuntimeError("Staged mod hash mismatch")
            result="STAGED"
        records.append({"guid":package["guid"],"source":str(src),"active":str(dst),
                        "sha256":file_sha(dst),"result":result})
    outcome={"schema":"NW_KK_ONE_SESSION_STAGE_V1","status":"PASS_STAGED_ONCE",
        "count":len(records),"elapsed_seconds":round(time.perf_counter()-start,3),
        "packages":records,"restart_count":0,"quality_not_approved":True}
    destination.parent.mkdir(parents=True,exist_ok=True)
    destination.write_text(json.dumps(outcome,indent=2),encoding="utf-8")
    return outcome

def validate_recipe(steps:list[dict],allow_save:bool=False) ->list[dict]:
    if not isinstance(steps,list) or not (1<=len(steps)<=300):
        raise ValueError("Batch must contain 1..300 steps")
    clean=[]
    for idx,step in enumerate(steps):
        if not isinstance(step,dict):raise ValueError("Step must be object")
        op=step.get("op")
        if op not in ALLOWED_NATIVE_OPS and not (allow_save and op in SAVING_OPS):
            raise ValueError("Unsupported/destructive/unauthorized op at %d: %s"%(idx,op))
        args=step.get("args",{})
        if not isinstance(args,dict) or set(args)-PARAMETERS:
            raise ValueError("Unknown native API parameters in step %d"%idx)
        if "nonce" in args:raise ValueError("Nonce ownership must stay with session runner")
        clean.append({"op":op,"args":args})
    # Last-write wins only for adjacent exact duplicates; keep distinct changes needed for A/B testing.
    return [r for i,r in enumerate(clean) if i==0 or r!=clean[i-1]]

def run_batch(steps:list[dict],receipt:pathlib.Path,allow_save=False,
              submit=None, guard=True, per_op_timeout:float=7.0)->dict:
    """One persistent Python process; no subprocess per native op, no GUI restarts."""
    ops=validate_recipe(steps,allow_save=allow_save)
    if receipt.exists():raise FileExistsError("Never overwrite previous session evidence")
    if guard:profile_valid();game_guards(stage=False)
    if submit is None:
        spec=importlib.util.spec_from_file_location("nw_sm01_client",CLIENT)
        mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
        submit=mod.submit
    receipt.parent.mkdir(parents=True,exist_ok=True)
    started=time.perf_counter();rows=[];failed=False
    for i,item in enumerate(ops):
        stamp=time.perf_counter()
        nonce="NW1_"+uuid.uuid4().hex[:19]
        data={"op":item["op"],"nonce":nonce}
        for key,val in item["args"].items():
            if key=="morphs":
                continue  # write a validated temporary native TSV once, below
            data[ARG_NAMES.get(key,key)]=val
        try:
            if item['op']=='apply_morph_batch':
                payload=item['args'].get('morphs')
                if isinstance(payload,str):
                    payload=json.loads(pathlib.Path(payload).read_text(encoding='utf-8-sig'))
                if not isinstance(payload,list) or not (1<=len(payload)<=96):
                    raise ValueError('Invalid morph batch')
                lines=[]
                for morph in payload:
                    g=morph['group'];index=morph['index'];value=morph['value']
                    if g not in ('face','body') or not isinstance(index,int) or not 0<=index<(52 if g=='face' else 44):
                        raise ValueError('Out of bounds native morph index')
                    if not isinstance(value,(int,float)) or not 0<=value<=1:
                        raise ValueError('Unsupported native normalized morph value')
                    lines.append(f'{g}\t{index}\t{value:.9g}')
                folder=RUNTIME/'recipes';folder.mkdir(parents=True,exist_ok=True)
                recipe='morph_'+nonce+'.tsv'
                recipe_path=folder/recipe
                with recipe_path.open('x',encoding='utf-8') as writer:
                    writer.write('\n'.join(lines)+'\n')
                data.pop('morphs',None);data['recipeFile']=recipe
            # Registered transport returns an int, prints details itself; direct in-process reduces startup cost.
            rc=submit(data,per_op_timeout)
            success=rc==0
            error="" if success else "native_receipt_failed_or_timeout"
        except Exception as exc:
            success=False;error=type(exc).__name__+":"+str(exc)
        elapsed=time.perf_counter()-stamp
        rows.append({"step":i,"op":item["op"],"nonce":nonce,"elapsed_seconds":round(elapsed,3),
                     "ok":success,"failure":error})
        if not success:failed=True;break
    elapsed=time.perf_counter()-started
    active=sum(x["elapsed_seconds"] for x in rows)
    result={"schema":"NW_KK_ONE_SESSION_TELEMETRY_V1","status":"PASS_NATIVE_OPS" if not failed else "FAIL_FAST",
            "operations":rows,"ops_requested":len(steps),"ops_dispatched":len(rows),
            "wall_seconds":round(elapsed,3),"native_dispatch_seconds":round(active,3),
            "runner_overhead_seconds":round(max(0,elapsed-active),3),"app_restarts":0,
            "source_mods_modified":False,"visual_fidelity_approved":False}
    receipt.write_text(json.dumps(result,indent=2),encoding="utf-8")
    if failed:raise RuntimeError("Native batch failed fast; evidence saved to "+str(receipt))
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    modes=p.add_subparsers(dest="mode",required=True)
    a=modes.add_parser("plan")
    a.add_argument("--keyword",action="append",default=[],help="Appearance component; repeat")
    a.add_argument("--guid",action="append",default=[],help="Known needed package GUID; repeat")
    a.add_argument("--out",required=True)
    a.add_argument("--max-packages",type=int,default=130)
    a.add_argument("--max-gib",type=float,default=9)
    b=modes.add_parser("stage")
    b.add_argument("--plan",required=True);b.add_argument("--receipt",required=True)
    c=modes.add_parser("batch")
    c.add_argument("--recipe",required=True);c.add_argument("--receipt",required=True)
    c.add_argument("--per-op-timeout",type=float,default=7)
    c.add_argument("--allow-save",action="store_true")
    opt=p.parse_args()
    if opt.mode=="plan":
        obj=catalog_plan(opt.keyword,opt.guid,max_packages=opt.max_packages,max_bytes=int(opt.max_gib*1024**3))
        dest=pathlib.Path(opt.out);dest.parent.mkdir(parents=True,exist_ok=True)
        if dest.exists():raise FileExistsError(dest)
        dest.write_text(json.dumps(obj,indent=2),encoding="utf-8")
        print("ONE_SESSION_POOL="+json.dumps({"status":obj["status"],"packages":obj["package_count"],
            "already_loaded":obj["already_selected_count"],"remaining":obj["new_packages_count"],
            "bytes":obj["estimated_bytes"],"missing_mandatory":obj["missing_mandatory_guids"]}))
    elif opt.mode=="stage":
        r=stage_plan(json.loads(pathlib.Path(opt.plan).read_text(encoding="utf-8")),pathlib.Path(opt.receipt))
        print("ONE_SESSION_STAGED="+json.dumps({"packages":r["count"],"elapsed_seconds":r["elapsed_seconds"]}))
    else:
        steps=json.loads(pathlib.Path(opt.recipe).read_text(encoding="utf-8"))
        if not 0.5<=opt.per_op_timeout<=20:raise ValueError("Per-operation timeout out of range")
        r=run_batch(steps,pathlib.Path(opt.receipt),allow_save=opt.allow_save,per_op_timeout=opt.per_op_timeout)
        print("ONE_SESSION_BATCH="+json.dumps({"status":r["status"],"ops":r["ops_dispatched"],"wall_seconds":r["wall_seconds"]}))
if __name__=="__main__":main()
