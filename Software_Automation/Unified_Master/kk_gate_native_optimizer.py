#!/usr/bin/env python3
"""Koikatsu visual-gated native fitting.

REFERENCE image -> first-pass feature manifest -> native pre-staged candidate trials ->
independent feature scores -> hard gate failures -> no saved/approved card.

Every meaningful feature gets individual score; success of overall metric cannot
cancel a failing critical face/hair/clothes/pose gate. This is the user requested
system IN Koikatsu's existing native edit/render loop, not third-party paid API.

True 2D->3D identity geometry generation is NOT yet implemented; do not label
this a TRELLIS-equivalent without independent held-out illustration acceptance.
"""
from __future__ import annotations
import argparse,hashlib,json,pathlib,time
import kk_native_visual_search as native
import kk_reference_features as detector
import kk_feature_gates as gates

def validate(plan,manifest):
 if manifest.get("schema")!="NW_KK_REFERENCE_FEATURES_V1":
  raise ValueError("First-program feature manifest absent/invalid")
 if pathlib.Path(manifest["reference_path"]).resolve()!=pathlib.Path(plan["reference"]).resolve():
  raise ValueError("Reference in candidate plan and gate manifest differs")
 if detector.digest(plan["reference"])!=manifest["reference_sha256"]:
  raise ValueError("Reference source image hash changed; regenerate features")
 if plan.get("semantic_weight",0):
  raise ValueError("Unreliable CLIP identity ranking disabled in feature-first gate optimizer")
 if not manifest.get("features"):raise ValueError("No reference features to gate")
 if len(set(f["id"] for f in manifest["features"]))!=len(manifest["features"]):
  raise ValueError("Non-unique feature IDs")
 view=plan["view"]
 for f in manifest["features"]:
  if f.get("view")!=view:
   raise ValueError(f"Feature {f['id']} requires separate {f.get('view')} camera, plan uses {view}")
 return True

def select_compare(base,candidates):
 """Reject regressions; favor lower critical failure count before total score.
 Only verified per-feature gates can authorize meaningful improvement.
 """
 b={x["id"]:x for x in base["features"]}
 ordered=[]
 for record in candidates:
  g=record["gates"]
  regressions=[];improvements=[]
  for f in g["features"]:
   prev=b[f["id"]]
   if not f["critical"] or not f["feature_identity_verified"]:continue
   change=f["score_0_to_100"]-prev["score_0_to_100"]
   if change<=-5:regressions.append(f["id"])
   if change>=5:improvements.append(f["id"])
  blocked=[x for x in g["blocked_critical_gates"] if
           next(f for f in g["features"] if f["id"]==x)["feature_identity_verified"]]
  unverified=[x for x in g["features"] if x["critical"] and not x["feature_identity_verified"]]
  can_review=not regressions and bool(improvements) and not unverified and len(blocked)<len([
      x for x in base["blocked_critical_gates"] if b[x]["feature_identity_verified"]])
  ordered.append({"label":record["label"],"score":g["weighted_metric_score_0_to_100"],
   "blocked_critical":g["blocked_critical_gates"],
   "verified_blocked_count":len(blocked),
   "feature_improvements":improvements,"feature_regressions":regressions,
   "eligible_for_user_review":can_review,
   "native_render":record["image"]})
 ordered.sort(key=lambda x:(len(x["feature_regressions"]),x["verified_blocked_count"],
              -len(x["feature_improvements"]),-x["score"]))
 return ordered

def run(plan,manifest,out_dir,submit=None,game_guard=True):
 validate(plan,manifest)
 out_dir=pathlib.Path(out_dir)
 if out_dir.exists():raise FileExistsError("Never overwrite prior reference-fitting evidence")
 out_dir.mkdir(parents=True)
 bank={}
 def scorer(reference,candidate,reference_roi=None,candidate_roi=None):
  # Cross-check source paths; never allow silent ROI substitution.
  card=gates.score_features(manifest,candidate)
  bank[str(pathlib.Path(candidate))]=card
  return {"score_0_to_100":card["weighted_metric_score_0_to_100"],
          "components":{f["id"]:{"score":f["score_0_to_100"],"blocked":
           f["gate_status"]!="METRIC_GATE_PASS"} for f in card["features"]}}
 sweep=native.run(plan,out_dir/"NATIVE_NONDESTRUCTIVE_SWEEP.json",
          submit=submit,game_guard=game_guard,score=scorer)
 baseline=bank[sweep["baseline_image"]]
 trials=[{"label":v["label"],"image":v["native_image"],
          "gates":bank[v["native_image"]]} for v in sweep["results"]]
 ranked=select_compare(baseline,trials)
 # Always produce side-by-side per-feature evidence for best candidate
 # under the ranking, never assume overall pixel metric is sufficient.
 if ranked:
  best=ranked[0]
  best_gates=gates.score_features(manifest,best["native_render"],
       report_dir=out_dir/"BEST_FEATURE_CROPS")
  best["feature_scorecard"]=best_gates
 else:best=None
 report={"schema":"NOWORLD_KOIKATSU_FEATURE_GATED_OPTIMIZER_V1",
  "feature_manifest_sha256":hashlib.sha256(json.dumps(manifest,sort_keys=True).encode()).hexdigest(),
  "reference":plan["reference"],"baseline":baseline,"candidates_ranked":ranked,
  "selected_for_human_inspection":best["label"] if best and best["eligible_for_user_review"] else None,
  "status":"CANDIDATES_SCORED_NOT_ARTISTICALLY_APPROVED",
  "feature_count":len(baseline["features"]),
  "native_state_restored":sweep["baseline_restored"],
  "all_gates_proven":False,
  "generated_3d_topology_from_image":False,
  "paid_api_calls":0,
  "native_card_saved":False,
  "production_character_accepted":False,
  "note":"Human reference identification and true shape reconstruction remain blocking; per-feature image scores are necessary, not sufficient."}
 (out_dir/"FEATURE_GATED_OPTIMIZATION.json").write_text(json.dumps(report,indent=2))
 print("FEATURE_GATE_SWEEP="+json.dumps({"features":len(baseline["features"]),
  "candidates":len(ranked),"best_review":report["selected_for_human_inspection"],
  "fails":[{"id":f["id"],"score":f["score_0_to_100"],"status":f["gate_status"]}
             for f in best["feature_scorecard"]["features"]] if best else [],
  "baseline_restored":report["native_state_restored"]}),flush=True)
 return report

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument("--plan",required=True);p.add_argument("--feature-manifest",required=True)
 p.add_argument("--out-dir",required=True)
 a=p.parse_args()
 plan=native.parse_spec(a.plan)
 m=json.loads(pathlib.Path(a.feature_manifest).read_text(encoding="utf-8-sig"))
 run(plan,m,pathlib.Path(a.out_dir))
if __name__=="__main__":main()
