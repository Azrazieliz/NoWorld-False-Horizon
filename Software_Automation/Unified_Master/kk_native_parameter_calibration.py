#!/usr/bin/env python3
"""Reversible, native Koikatsu per-parameter VISUAL calibration.

A native receipt alone is not proof of an observable or correctly located edit.
Capture three baseline, two trial and two restored fixed-camera images within ONE
running sandbox Maker; suppress per-pixel same-state drift; verify localization
and final restoration. Produces side-by-side and delta evidence. Never saves cards.
No independent source artwork or artistic resemblance is inferred.
"""
from __future__ import annotations
import argparse, json, pathlib, time
import numpy as np
from PIL import Image, ImageDraw
from scipy.ndimage import gaussian_filter
import kk_native_visual_search as native
import kk_visual_fit as vision
import kk_one_session as master

SCHEMA="NW_KK_NATIVE_PARAMETER_CALIBRATION_V1"
ALLOWED={"apply_morph_batch","set_hair_palette","set_hair","set_eye_iris",
         "set_iris_asset","set_accessory_vector","set_clothes","set_clothes_top"}
SCOPE_ARGS={"set_hair_palette":("index",),"set_hair":("index",),
    "set_eye_iris":("index",),"set_iris_asset":("index",),
    "set_accessory_vector":("slot","group","index"),
    "set_clothes":("index",),"set_clothes_top":(),
    "apply_morph_batch":()}
def validate(plan):
    if plan.get("schema")!=SCHEMA:raise ValueError("Unknown calibration schema")
    if plan.get("view") not in native.VIEWS:raise ValueError("Unsupported camera")
    roi=plan.get("target_roi_norm")
    if not isinstance(roi,list) or len(roi)!=4 or any(type(v) not in (float,int) for v in roi) or not (
        0<=roi[0]<roi[2]<=1 and 0<=roi[1]<roi[3]<=1):
        raise ValueError("Expected normalized target feature rectangle")
    baseline=plan.get("restore_baseline")
    trials=plan.get("trial_steps")
    if not isinstance(baseline,list) or not 1<=len(baseline)<=12 or not isinstance(trials,list) or not 1<=len(trials)<=8:
        raise ValueError("Explicit bounded baseline and trial steps required")
    for step in baseline+trials:
        if not isinstance(step,dict) or step.get("op") not in ALLOWED or not isinstance(step.get("args"),dict):
            raise ValueError("Non-reversible or unsupported step")
        if set(step["args"])-master.PARAMETERS:raise ValueError("Unsupported argument")
    for trial in trials:
        matched=False
        for base in baseline:
            if trial["op"]!=base["op"]:continue
            if any(trial["args"].get(k)!=base["args"].get(k) for k in SCOPE_ARGS[trial["op"]]):
                continue
            if trial["op"]=="apply_morph_batch":
                a=trial["args"].get("morphs");b=base["args"].get("morphs")
                if not isinstance(a,list) or not isinstance(b,list):continue
                ba={(x["group"],x["index"]) for x in b}
                if not all((x["group"],x["index"]) in ba for x in a):continue
            matched=True;break
        if not matched:raise ValueError("Trial lacks matching explicit source restoration setter")
    return plan

def load_images(paths):
    images=[];size=None
    for path in paths:
        with Image.open(path) as im:
            if size is None:size=im.size
            if im.size!=size:raise ValueError("Camera dimensions changed during trial")
            if min(im.size)<384:raise ValueError("Camera capture too small for visual calibration")
            data=np.asarray(im.convert("RGB"),dtype=np.float32)/255.
            if vision.white_background_confidence(data)<.68:
                raise ValueError("Uncontrolled native background")
            images.append(data)
    return np.stack(images)

def compute(baselines,trials,restores,roi,min_fraction=.018,min_share=.4):
    """All images are same-frame native captures; no per-subject recentering."""
    if len(baselines)<3 or len(trials)<2 or len(restores)<2:
        raise ValueError("Need repeated images in each native state")
    allframes=np.concatenate((baselines,trials,restores))
    if allframes.ndim!=4 or allframes.shape[-1]!=3:
        raise ValueError("Expected RGB arrays")
    H,W=allframes.shape[1:3]
    if H<384 or W<384:raise ValueError("Native visual frames too small")
    def blur(frames):return gaussian_filter(frames,sigma=(0,1.1,1.1,0))
    b=blur(baselines);t=blur(trials);r=blur(restores)
    # Maximum original-state pair delta is a conservative local animation/noise map.
    bgnoise=np.maximum.reduce([np.linalg.norm(b[i]-b[j],axis=-1)
                               for i,j in ((0,1),(0,2),(1,2))])
    reference=b.mean(axis=0)
    candidate=t.mean(axis=0)
    reverted=r.mean(axis=0)
    candidate_excess=np.maximum(0.,np.linalg.norm(candidate-reference,axis=-1)-bgnoise)
    restore_excess=np.maximum(0.,np.linalg.norm(reverted-reference,axis=-1)-bgnoise)
    # Difference threshold after noise subtraction, rather than global pixel similarity.
    changed=candidate_excess>.13
    restored_changed=restore_excess>.13
    x1,y1,x2,y2=roi
    bounds=[int(x1*W),int(y1*H),int(x2*W),int(y2*H)]
    mask=np.zeros((H,W),bool)
    mask[bounds[1]:bounds[3],bounds[0]:bounds[2]]=True
    if mask.sum()<100:raise ValueError("ROI too small")
    local=int((changed&mask).sum())
    changed_total=int(changed.sum())
    frac=local/int(mask.sum())
    share=local/max(1,changed_total)
    restored_ratio=float(restored_changed.sum()/(W*H))
    noise_ratio=float((bgnoise>.13).mean())
    # Baseline includes native idle animation; return-to-source need only fall
    # within a conservative fraction of the measured same-state noise envelope.
    restoration_tolerance=max(.005,noise_ratio*.25)
    return {"target_roi_pixels":bounds,"target_area_pixels":int(mask.sum()),
        "local_changed_pixels":local,"total_changed_pixels":changed_total,
        "local_changed_fraction":round(frac,5),
        "changed_pixel_target_share":round(share,5),
        "same_state_noise_fraction":round(noise_ratio,5),
        "restoration_excess_changed_fraction":round(restored_ratio,5),
        "restoration_noise_tolerance":round(restoration_tolerance,5),
        "visibly_changed":bool(frac>=min_fraction),
        "correctly_localized":bool(frac>=min_fraction and share>=min_share),
        "restoration_visual_verified":bool(restored_ratio<=restoration_tolerance),
        "restoration_certainty":"WITHIN_REPEATED_RENDER_NOISE_NOT_PIXEL_IDENTITY",
        "artistic_likeness_improved":None}, changed,reference,candidate

def contact(baseline,candidate,changed,bounds,path):
    H,W=changed.shape
    def tile(data):
        # Preserve full native-camera evidence resolution; do not shrink the proof.
        return Image.fromarray((np.clip(data,0,1)*255).astype("uint8"))
    first=tile(baseline);second=tile(candidate)
    heat=np.zeros((H,W,3),dtype=np.uint8)
    heat[changed]=[255,65,30]
    third=Image.fromarray(heat)
    output=Image.new("RGB",(W*3,H+36),(255,255,255))
    for i,pic in enumerate((first,second,third)):
        output.paste(pic,(W*i,26))
    d=ImageDraw.Draw(output)
    for i,label in enumerate(("Restored baseline","Trial native render","Change ABOVE animation noise")):
        d.text((W*i+8,7),label,fill=(14,14,14))
    x1,y1,x2,y2=bounds
    for i in (0,1,2):
        d.rectangle((W*i+x1,26+y1,W*i+x2,26+y2),
                    outline=(247,30,155),width=3)
    output.save(path)

def run(plan,outdir,submit=None,game_guard=True):
    validate(plan)
    outdir=pathlib.Path(outdir)
    if outdir.exists():raise FileExistsError("Never replace existing calibration QA")
    if game_guard:master.profile_valid();master.game_guards(stage=False)
    if submit is None:submit=native.client_factory()
    outdir.mkdir(parents=True)
    calls=0;paths={"baseline":[],"trial":[],"restore":[]};errors=[]
    camera=native.CAMERA
    view=plan["view"]
    def step(op):
        nonlocal calls
        native.invoke(submit,op,timeout=12);calls+=1
    def capture(key,n):
        nonlocal calls
        for _ in range(n):
            nonce,_,_=native.invoke(submit,{"op":"render_camera_view","args":{"group":view}},timeout=16)
            calls+=1
            path=camera/("SM01_CAM_"+nonce+"_"+view+".png")
            if not path.exists():raise FileNotFoundError("Native capture missing: "+str(path))
            paths[key].append(path)
    try:
        for op in plan["restore_baseline"]:step(op)
        capture("baseline",3)
        for op in plan["trial_steps"]:step(op)
        capture("trial",2)
    except Exception as exc:
        errors.append("TRIAL_FAILURE:"+str(exc))
    finally:
        # Always attempt restoration even after a trial/render error.
        try:
            for op in plan["restore_baseline"]:step(op)
            capture("restore",2)
        except Exception as exc:errors.append("RESTORE_FAILURE:"+str(exc))
    result=None
    if not errors and all(len(paths[k])==n for k,n in (("baseline",3),("trial",2),("restore",2))):
        try:
            q,changed,b,c=compute(load_images(paths["baseline"]),
                load_images(paths["trial"]),load_images(paths["restore"]),
                plan["target_roi_norm"],
                float(plan.get("min_local_delta_fraction",.018)),
                float(plan.get("min_local_change_share",.4)))
            result=q
            contact(b,c,changed,q["target_roi_pixels"],outdir/"NATIVE_PARAMETER_VISUAL_DELTA.png")
        except Exception as exc:errors.append("MEASUREMENT_FAILURE:"+str(exc))
    success=bool(result and result["correctly_localized"] and result["restoration_visual_verified"] and not errors)
    report={"schema":"NW_KK_NATIVE_PARAMETER_CALIBRATION_RECEIPT_V1",
        "status":"VISIBLE_LOCALIZED_AND_RESTORED" if success else "BLOCKED_NO_VERIFIED_LOCAL_EFFECT",
        "view":view,"parameter_label":plan.get("label","unnamed"),
        "observations":result,"native_call_count":calls,
        "captures":{k:[str(p) for p in v] for k,v in paths.items()},
        "errors":errors,"baseline_restore_attempted":True,
        "baseline_restoration_verified":bool(result and result["restoration_visual_verified"] and not errors),
        "calibrated_native_effect":success,"source_card_saved":False,
        "production_accepted":False,"independent_reference_fidelity_proven":False}
    (outdir/"NATIVE_PARAMETER_CALIBRATION.json").write_text(json.dumps(report,indent=2))
    print("NATIVE_PARAMETER_CALIBRATION="+json.dumps({k:report[k] for k in
        ("status","parameter_label","observations","errors","native_call_count")}),flush=True)
    return report

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--plan",required=True);p.add_argument("--out-dir",required=True)
    a=p.parse_args()
    plan=json.loads(pathlib.Path(a.plan).read_text(encoding="utf-8-sig"))
    report=run(plan,pathlib.Path(a.out_dir))
    return 0 if report["calibrated_native_effect"] else 2
if __name__=="__main__":raise SystemExit(main())
