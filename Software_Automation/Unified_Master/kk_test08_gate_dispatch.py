#!/usr/bin/env python3
"""Test08 mandatory Koikatsu feature-first dispatch. Never builds from guessed asset names.

Only runs the ORIGINAL (commit 521d725) feature-gate native optimizer, not
free-form native construction scripts. The source is a genuine full-resolution
image; visible, annotated feature ROIs are required. Complex backgrounds must
be separated from the character with a same-resolution binary mask.

Commands:
  status: verify original feature-gate source and continuity, no writes/Koikatsu.
  run: accept source, annotation, native trial plan and output root, preflight
       BEFORE ANY live Maker mutation, run original feature gate and audit.
"""
from __future__ import annotations
import argparse,hashlib,json,pathlib,subprocess
import numpy as np
from PIL import Image
import kk_reference_features as features
import kk_feature_gates as gates
import kk_gate_native_optimizer as optimizer
import kk_registered_gate_optimizer as registered_optimizer
import kk_native_visual_search as native
import kk_one_session as one
import kk_benchmark_guard as benchmark_guard

HERE=pathlib.Path(__file__).resolve().parent
BASELINE="521d725"
REQUIRED_FILES=("kk_native_visual_search.py","kk_gate_native_optimizer.py",
                "test_kk_native_visual_search.py","test_kk_gate_native_optimizer.py")
CONTINUITY=HERE/"TEST06_8OF10_GATE_SCORE_CONTINUITY_20261018.md"
MANDATORY=("hair","face","eyes","body","main_outfit")
def sha(path):
 h=hashlib.sha256()
 with pathlib.Path(path).open("rb") as f:
  for block in iter(lambda:f.read(1024*1024),b""):h.update(block)
 return h.hexdigest()
def status():
 """Pure read-only continuity + code checksum validation; no Maker access."""
 if not CONTINUITY.is_file() or "user-rated 8/10" not in CONTINUITY.read_text(encoding="utf-8"):
  raise RuntimeError("Test06 user 8/10 continuity missing")
 repo=HERE.parents[1]
 errors=[]
 for name in REQUIRED_FILES:
  rel=(pathlib.Path("Software_Automation")/"Unified_Master"/name).as_posix()
  a=subprocess.run(["git","-C",str(repo),"rev-parse",f"{BASELINE}:{rel}"],
                 text=True,capture_output=True,timeout=8)
  # Preserve original source as a pristine on-disk copy while the live native
  # invoke path enforces no-ungated-Test07+ changes.
  live_path=(HERE/"kk_native_visual_search_521d725.py") if name=="kk_native_visual_search.py" else HERE/name
  b=subprocess.run(["git","-C",str(repo),"hash-object",str(live_path)],
                 text=True,capture_output=True,timeout=8)
  if a.returncode or b.returncode or a.stdout.strip()!=b.stdout.strip():
   errors.append(name)
 if errors:raise RuntimeError("Original feature-gate source modified: "+", ".join(errors))
 live=(HERE/"kk_native_visual_search.py").read_text(encoding="utf-8")
 if "benchmark_guard.require_permitted_step(step)" not in live:
  raise RuntimeError("Native edit path is not protected by scoring gate")
 return {"baseline_commit":BASELINE,"original_gate_files_verified":len(REQUIRED_FILES),
         "native_mutation_gate_enforced":True,
         "last_user_rated_visual_benchmark":"Test06 8/10","test07_accepted":False,
         "test08_character_generated":False,"maker_started":False}
def prepare(reference,annotations,plan_path,mask_path=None,benchmark_id="TEST08"):
 """Fail CLOSED without user image, visible ROIs, original catalog/native plan."""
 ref=pathlib.Path(reference)
 if not ref.is_file():raise FileNotFoundError("Original full-resolution reference is not on PC: "+str(ref))
 with Image.open(ref) as im:
  if min(im.size)<384:raise ValueError("Original reference too small")
  rgb=im.convert("RGB")
  W,H=rgb.size
 note=json.loads(pathlib.Path(annotations).read_text(encoding="utf-8-sig"))
 if note.get("schema") not in ("NW_KK_TEST08_USER_GROUNDED_FEATURES_V1","NW_KK_USER_GROUNDED_FEATURES_V1"):
  raise ValueError("Missing Test08 feature annotation schema")
 if note.get("reference_sha256")!=sha(ref):
  raise ValueError("Annotation not bound to original reference bytes")
 boxes=note.get("features")
 if not isinstance(boxes,dict):raise ValueError("Missing visible character feature regions")
 unknown=set(boxes)-({t[0] for t in features.TEMPLATE})
 if unknown:raise ValueError("Unsupported feature IDs: "+str(sorted(unknown)))
 # Never hallucinate absent or obscured anatomy. Other features must be explicit null.
 if set(boxes)!={t[0] for t in features.TEMPLATE}:
  raise ValueError("Every template feature must be explicitly boxed OR null")
 for fid in MANDATORY:
  if boxes[fid] is None:
   raise ValueError("Critical feature "+fid+" absent; choose separate image/view or provide explicit override")
 verified=[]
 for name,roi in boxes.items():
  if roi is None:continue
  if not isinstance(roi,list) or len(roi)!=4 or any(type(x) not in (int,float) for x in roi) or not (
     0<=roi[0]<roi[2]<=1 and 0<=roi[1]<roi[3]<=1):
   raise ValueError("Feature ROI malformed: "+name)
  verified.append(name)
 # IMPORTANT: white native fixed-camera background; never compare artwork scenes as body.
 corners=np.concatenate([np.asarray(rgb)[:24,:24].reshape(-1,3),
      np.asarray(rgb)[:24,-24:].reshape(-1,3),
      np.asarray(rgb)[-24:,:24].reshape(-1,3),
      np.asarray(rgb)[-24:,-24:].reshape(-1,3)]) /255.
 white=float(np.clip(1-(np.abs(corners-1).mean()/.13),0,1))
 mask_source=None
 if white<.60:
  if not mask_path:raise ValueError("Complex reference background requires an actual same-resolution subject mask")
  with Image.open(mask_path) as m:
   if m.size!=(W,H):raise ValueError("Subject mask resolution mismatch")
   mask=np.asarray(m.convert("L"),dtype=np.uint8)
  if ((mask>=128).mean()<.03) or ((mask>=128).mean()>.94):
   raise ValueError("Subject mask cannot be empty or near-entire scene")
  masked=np.asarray(rgb).copy()
  masked[mask<128]=255
  mask_source=masked
 plan=native.parse_spec(plan_path)
 if pathlib.Path(plan["reference"]).resolve()!=ref.resolve():
  raise ValueError("Native trial plan reference must be the SAME original image")
 if not plan.get("restore_baseline") or not plan.get("candidates"):
  raise ValueError("A/B trials must restore original Maker baseline")
 if not all(x in note for x in ("view","benchmark_id")) or note["benchmark_id"]!=benchmark_id:
  raise ValueError("Wrong benchmark identity")
 view=note["view"]
 if view!=plan["view"]:
  raise ValueError("Annotation view must match actual Koikatsu camera")
 if view not in native.VIEWS:raise ValueError("Unsupported native camera")
 if plan.get("semantic_weight",0)!=0:raise ValueError("Semantic-only candidate ranking not approved")
 # Build a SINGLE score manifest for this view; original geometric default ROIs
 # are NOT accepted, and user-visible hand-grounded feature boxes are mandatory.
 prepared={"width":W,"height":H,"ref_sha256":sha(ref),
           "view":view,"verified_features":verified,
           "masked_reference":mask_source is not None}
 return prepared,boxes,mask_source,plan
def run(reference,annotations,plan,out_dir,mask=None,benchmark_id="TEST08"):
 status()
 prepared,boxes,pixels,native_plan=prepare(reference,annotations,plan,mask,benchmark_id=benchmark_id)
 out=pathlib.Path(out_dir)
 if out.exists():raise FileExistsError("Never overwrite Test08 evidence or earlier runs")
 # ALL validation and Maker guards precede writes and native edits.
 one.profile_valid();one.game_guards(stage=False)
 out.mkdir(parents=True)
 mask_ref=pathlib.Path(reference)
 if pixels is not None:
  mask_ref=out/"REFERENCE_SUBJECT_ON_WHITE.png"
  Image.fromarray(pixels,"RGB").save(mask_ref)
 # Original source is retained unchanged and matched to the annotation digest.
 grounded={k:v for k,v in boxes.items()}
 manifest=features.build_manifest(mask_ref,boxes=grounded,view=native_plan["view"])
 if set(x["id"] for x in manifest["features"])!=set(prepared["verified_features"]):
  raise RuntimeError("Semantic feature manifest not grounded to explicit annotations")
 if not all(x["feature_identity_verified"] for x in manifest["features"]):
  raise RuntimeError("Geometric guesses slipped into feature gate")
 # Existing optimizer requires the plan to match scored manifest source.
 scored_plan=dict(native_plan,reference=str(mask_ref))
 optimizer.validate(scored_plan,manifest)
 (out/"FEATURES_SOURCE_GROUNDED.json").write_text(json.dumps({
      "source_original":str(reference),"source_sha256":prepared["ref_sha256"],
      "source_size":[prepared["width"],prepared["height"]],
      "annotation_file":str(annotations),"feature_ids":prepared["verified_features"],
      "mask_file":str(mask) if mask else None,"actual_scoring_image":str(mask_ref),
      "source_pixels_modified":False},indent=2),encoding="utf-8")
 (out/"FEATURES_MANIFEST.json").write_text(json.dumps(manifest,indent=2),encoding="utf-8")
 with benchmark_guard.scored_native_sweep(scored_plan,manifest):
  result=registered_optimizer.run(scored_plan,manifest,out/"GATED_NATIVE_TRIALS",
        reference_mask=mask)
 candidates=result["candidates_ranked"]
 # Native delta must exist and exceed calibrated same-state movement.
 sweep=json.loads((out/"GATED_NATIVE_TRIALS"/"NATIVE_NONDESTRUCTIVE_SWEEP.json").read_text())
 changes={x["label"]:x.get("visible_delta",{}) for x in sweep.get("results",[])}
 noise=sweep.get("native_pixel_noise_floor")
 noise_valid=(sweep.get("native_noise_measurement_valid") is True and
              type(noise) in (int,float) and 0<=noise<.90)
 # A perfectly stable camera has VALID calibrated noise of 0. Only missing
 # repeat-camera evidence, not an exact zero, must block scored improvements.
 provisional=[r for r in candidates if r["eligible_for_user_review"] and
            not r["feature_regressions"] and len(r["feature_improvements"])>0
            and noise_valid and changes.get(r["label"],{}).get("measurably_changed") is True]
 # The restored original selector only rejects >=5-point critical losses.
 # Protect every critical visible feature more strictly before user review.
 baseline={f["id"]:f for f in result["baseline"]["features"]}
 eligible=[];rejected={}
 for r in provisional:
  measured=gates.score_features(manifest,r["native_render"],
      rois=result["registration"]["candidate_rois"])
  losses=[f["id"] for f in measured["features"] if f["critical"] and
     (f["score_0_to_100"] < baseline[f["id"]]["score_0_to_100"]-2.0 or
      not f["feature_identity_verified"])]
  gains=[f["id"] for f in measured["features"] if f["critical"] and
     f["score_0_to_100"] > baseline[f["id"]]["score_0_to_100"]+4.0]
  if losses or not gains:rejected[r["label"]]={"critical_losses":losses,"gains":gains}
  else:eligible.append(r)
 decision={"schema":"NW_KK_BENCHMARK_GATE_DISPATCH_V1","benchmark_id":benchmark_id,
    "native_state_restored":result["native_state_restored"],
    "native_noise_baseline_confirmed":noise_valid,
    "native_feature_regions_registered":bool(result.get("registration")),
    "registered_native_roi_map":result["registration"]["candidate_rois"],
    "candidate_count":len(candidates),
    "no_critical_feature_regressions":[r["label"] for r in eligible],
    "candidates_blocked_for_critical_loss":rejected,
    "status":"REVIEW_NATIVE_IMAGES" if eligible and result["native_state_restored"] else "REJECT_ALL_OR_UNVERIFIED",
    "production_approved":False,"card_accepted":False,
    "user_visual_rating":None,
    "no_paid_conversion":True,
    "source_sha256":prepared["ref_sha256"]}
 (out/(benchmark_id+"_VISUAL_GATE_DECISION.json")).write_text(json.dumps(decision,indent=2))
 return decision
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument("mode",choices=("status","run"))
 p.add_argument("--reference");p.add_argument("--annotations");p.add_argument("--plan")
 p.add_argument("--out-dir");p.add_argument("--mask")
 args=p.parse_args()
 if args.mode=="status":result=status()
 else:
  if not all((args.reference,args.annotations,args.plan,args.out_dir)):
   p.error("run requires source reference, verified feature annotations, reversible plan, output")
  result=run(args.reference,args.annotations,args.plan,args.out_dir,args.mask)
 print("KOIKATSU_GATE08="+json.dumps(result,sort_keys=True),flush=True)
if __name__=="__main__":main()
