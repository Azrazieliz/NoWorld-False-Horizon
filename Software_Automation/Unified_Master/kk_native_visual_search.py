#!/usr/bin/env python3
"""Reference-guided native Koikatsu trial-and-compare loop (CPU, NO paid inference).

Uses authentic SM01 native setters and persistent passive white-background cameras.
Safely restores an EXPLICIT source baseline after EVERY candidate. No card saves,
Unity restarts, original install changes or source mod copies.

Do not confuse image score with semantic recognition or production approval.
Reference must be accessible locally on the authorized PC at native resolution.
"""
from __future__ import annotations
import argparse,contextlib,importlib.util,io,json,pathlib,sys,time,uuid
import kk_one_session as master
import kk_visual_fit as vision

OP_SET={"set_eye_iris","set_iris_asset","set_hair_palette","set_hair","set_clothes",
        "set_clothes_top","set_accessory_vector","apply_morph_batch"}
PROFILE_OPS={"set_hair_palette","set_hair","set_clothes_top","set_accessory_vector"}
VIEWS={"face","front","upper","body"}
ROOT=pathlib.Path(r"D:\UnrealProjects\NW_Blender_Private_Review_20261017\REFERENCE_GUIDED_QA")
CAMERA=master.RUNTIME/"camera_bank"

def parse_spec(path):
    data=json.loads(pathlib.Path(path).read_text(encoding="utf-8-sig"))
    if data.get("schema")!="NOWORLD_NATIVE_VISUAL_SWEEP_V1":
        raise ValueError("Unknown sweep recipe schema")
    baseline=data.get("restore_baseline")
    choices=data.get("candidates")
    if not isinstance(baseline,list) or not 1<=len(baseline)<=12:
        raise ValueError("Explicit source restoration recipe required")
    if not isinstance(choices,list) or not 1<=len(choices)<=32:
        raise ValueError("Only 1..32 candidates per bounded trial")
    if data.get("view") not in VIEWS:raise ValueError("Unknown fixed camera")
    for item in baseline:
        if item.get("op") not in OP_SET or not isinstance(item.get("args"),dict):
            raise ValueError("Invalid baseline setter")
        if set(item["args"])-master.PARAMETERS:raise ValueError("Unsafe baseline setter args")
    for candidate in choices:
        if candidate.get("label") is None or not isinstance(candidate.get("steps"),list) or not 1<=len(candidate["steps"])<=12:
            raise ValueError("Candidate must have named 1..12 step recipe")
        if not isinstance(candidate["label"],str) or not 1<=len(candidate["label"])<=75:
            raise ValueError("Invalid candidate label")
        for step in candidate["steps"]:
            if step.get("op") not in OP_SET or not isinstance(step.get("args"),dict):
                raise ValueError("Invalid native candidate step")
            if set(step["args"])-master.PARAMETERS:raise ValueError("Unsupported native API parameters")
    ref=pathlib.Path(data["reference"])
    if not ref.is_file():raise FileNotFoundError(ref)
    from PIL import Image
    with Image.open(ref) as img:
        if min(img.size)<384:
            raise ValueError("Refuse low-res 144p reconstruction source; use original ≥384 px shortest side")
    return data

def client_factory():
    spec=importlib.util.spec_from_file_location("nw_sm01_client",master.CLIENT)
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
    return m.submit

def make_payload(step,nonce):
    args=step["args"]
    out={"op":step["op"],"nonce":nonce}
    for k,v in args.items():
        if k=="morphs":continue
        out[master.ARG_NAMES.get(k,k)]=v
    if "morphs" in args:
        src=args["morphs"]
        if isinstance(src,str):src=json.loads(pathlib.Path(src).read_text(encoding="utf-8-sig"))
        if not isinstance(src,list) or not (1<=len(src)<=96):raise ValueError("Invalid morph batch")
        lines=[]
        for r in src:
            g=r["group"];idx=r["index"];v=r["value"]
            if g not in ("body","face") or type(idx)!=int or not 0<=idx<(44 if g=="body" else 52):
                raise ValueError("Invalid morph source/index")
            if not isinstance(v,(int,float)) or not 0<=v<=1:raise ValueError("Invalid morph value")
            lines.append(f"{g}\t{idx}\t{v:.9g}")
        folder=master.RUNTIME/"recipes";folder.mkdir(parents=True,exist_ok=True)
        filename="sweep_"+nonce+".tsv"
        with (folder/filename).open("x",encoding="utf-8") as f:f.write("\n".join(lines)+"\n")
        out["recipeFile"]=filename
    return out

def invoke(submit,step,timeout=8):
    nonce="NWFIT_"+uuid.uuid4().hex[:14]
    payload=make_payload(step,nonce)
    log=io.StringIO()
    stamp=time.perf_counter()
    with contextlib.redirect_stdout(log):
        rc=submit(payload,timeout)
    try:last=json.loads(log.getvalue().strip().splitlines()[-1])
    except Exception as exc:raise RuntimeError("Malformed sandbox native receipt: "+str(exc))
    if rc!=0 or last.get("status")!="RECEIPT_WRITTEN":
        raise RuntimeError("Native setter or camera failed: "+str(last)[:500])
    return nonce,round(time.perf_counter()-stamp,3),last

def run(data,out,submit=None,game_guard=True,score=vision.evaluate):
    if out.exists():raise FileExistsError(out)
    if game_guard:master.profile_valid();master.game_guards(stage=False)
    if submit is None:submit=client_factory()
    ref=data["reference"];roi=data.get("reference_roi");candidate_roi=data.get("candidate_roi",roi)
    semantic_weight=float(data.get("semantic_weight",0))
    if not 0<=semantic_weight<=.65:raise ValueError("Invalid semantic ranking weight")
    encoder=None;source_embedding=None
    if semantic_weight:
        from kk_local_vision_embeddings import LocalEncoder,sim
        encoder=LocalEncoder();source_embedding,_=encoder.embed(ref)
    view=data["view"]
    out.parent.mkdir(parents=True,exist_ok=True)
    baseline=data["restore_baseline"]
    results=[];failed=[];started=time.perf_counter()
    def restore():
        for op in baseline:invoke(submit,op)
    def screenshot():
        nonce,t,_=invoke(submit,{"op":"render_camera_view","args":{"group":view}},timeout=10)
        file=CAMERA/("SM01_CAM_"+nonce+"_"+view+".png")
        if not file.is_file():raise RuntimeError("Native camera succeeded without image: "+str(file))
        return file,t
    baseline_ok=False
    try:
        restore()
        baseline_path,baseline_capture_sec=screenshot()
        try:
            repeat_path,_=screenshot()
            drift=vision.measure_visible_change(str(baseline_path),str(repeat_path))
            noise_floor=drift['changed_subject_fraction']
        except (ValueError,OSError):
            noise_floor=0.
        baseline_eval=score(ref,str(baseline_path),reference_roi=roi,candidate_roi=candidate_roi)
        baseline_score=baseline_eval["score_0_to_100"]
        baseline_semantic=None
        if encoder is not None:
            vec,_=encoder.embed(str(baseline_path));baseline_semantic=sim(source_embedding,vec)['weighted_cosine']
        baseline_rank_score=(1-semantic_weight)*baseline_score+semantic_weight*(50+50*baseline_semantic) if encoder else baseline_score
        baseline_ok=True
        for i,item in enumerate(data["candidates"]):
            started_trial=time.perf_counter()
            try:
                restore()
                steps=[]
                for step in item["steps"]:
                    _,secs,_=invoke(submit,step)
                    steps.append({"op":step["op"],"seconds":secs})
                result_path,render_sec=screenshot()
                match=score(ref,str(result_path),reference_roi=roi,candidate_roi=candidate_roi)
                try:
                    change=vision.measure_visible_change(str(baseline_path),str(result_path),roi=candidate_roi,noise_floor=noise_floor)
                except (ValueError,OSError) as ex:
                    change={"unavailable":str(ex)[:160]}
                local_semantic=None
                if encoder is not None:
                    vec,_=encoder.embed(str(result_path));local_semantic=sim(source_embedding,vec)
                ranking_score=(1-semantic_weight)*match['score_0_to_100']+semantic_weight*(50+50*local_semantic['weighted_cosine']) if encoder else match['score_0_to_100']
                results.append({"index":i,"label":item["label"],"visible_delta":change,
                    "ranking_score":round(ranking_score,3),"semantic_embedding":local_semantic,
                    "score":match["score_0_to_100"],"components":match["components"],
                    "native_image":str(result_path),"steps":steps,
                    "render_sec":render_sec,"wall_sec":round(time.perf_counter()-started_trial,3),
                    "above_baseline":match["score_0_to_100"]>baseline_score})
                print("VISUAL_TRIAL",i,item["label"],match["score_0_to_100"],flush=True)
            except Exception as e:
                failed.append({"index":i,"label":item["label"],"error":str(e)[:450]})
                print("VISUAL_TRIAL_FAIL",i,item["label"],str(e)[:150],flush=True)
    finally:
        # Fail closed: never leave a trial's hair/outfit/shape as active source.
        # If native communication dies, the caller MUST treat QA as corrupted.
        try:restore();restored=True
        except Exception as e:restored=False;failed.append({"baseline_restoration_failure":str(e)})
    results.sort(key=lambda r:r["ranking_score"],reverse=True)
    best=results[0] if results else None
    margin=float(data.get("minimum_score_gain",4.0))
    if not 0<margin<=20:raise ValueError("Unreasonable image score improvement margin")
    recommendation="KEEP_BASELINE" if not best or best['ranking_score']<=(baseline_rank_score+margin) else "REVIEW_BEST_CANDIDATE_WITH_USER"
    report={
     "schema":"NOWORLD_NATIVE_VISUAL_SWEEP_RECEIPT_V1","status":
       "PASS_RANKED_WITH_RESTORED_BASELINE" if baseline_ok and restored else "FAIL_RESTORE_OR_BASELINE",
     "reference":ref,"view":view,
     "baseline_score":baseline_score if baseline_ok else None,
     "baseline_combined_rank_score":round(baseline_rank_score,3) if baseline_ok else None,
     "semantic_weight":semantic_weight,
     "baseline_image":str(baseline_path) if baseline_ok else None,
     "results":results,"failures":failed,"best":best,
     "baseline_restored":restored,"native_calls_without_restart":True,
     "recommendation":recommendation,"min_improvement_margin":margin,"native_pixel_noise_floor":noise_floor,
     "candidate_count":len(data["candidates"]),
     "total_elapsed_seconds":round(time.perf_counter()-started,2),
     "no_card_or_original_source_saved":True,
     "fidelity_approved":False,
     "warning":"This is a metric-driven shortlist, not an artist-level identity acceptance decision."}
    out.write_text(json.dumps(report,indent=2),encoding="utf-8")
    if not restored:raise RuntimeError("BASELINE RESTORE FAILED; stop live editing, inspect source state")
    return report

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--plan",required=True);p.add_argument("--receipt",required=True)
    a=p.parse_args()
    plan=parse_spec(a.plan)
    result=run(plan,pathlib.Path(a.receipt))
    print("VISUAL_SWEEP="+json.dumps({"status":result["status"],
        "trials":len(result["results"]),"best":result["best"]["label"] if result["best"] else None,
        "baseline_restored":result["baseline_restored"],
        "elapsed":result["total_elapsed_seconds"]}))
    return 0 if result["status"]=="PASS_RANKED_WITH_RESTORED_BASELINE" else 2
if __name__=="__main__":raise SystemExit(main())
