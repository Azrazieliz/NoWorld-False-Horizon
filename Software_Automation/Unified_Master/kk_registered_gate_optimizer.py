"""Subject-registered Koikatsu visual gate, preserving old optimizer unchanged."""
from __future__ import annotations
import hashlib,json,pathlib,time
import kk_native_visual_search as native
import kk_feature_gates as gate
import kk_gate_native_optimizer as old
from kk_native_feature_alignment import register_boxes

def run(plan,manifest,out_dir,submit=None,game_guard=True,reference_mask=None):
    old.validate(plan,manifest)
    out_dir=pathlib.Path(out_dir)
    if out_dir.exists():raise FileExistsError(out_dir)
    out_dir.mkdir(parents=True)
    baseline_registration=None
    bank={}
    def scorer(reference,candidate,reference_roi=None,candidate_roi=None):
        nonlocal baseline_registration
        if baseline_registration is None:
            baseline_registration=register_boxes(manifest,candidate,reference_mask)
        card=gate.score_features(manifest,candidate,
                  rois=baseline_registration["candidate_rois"])
        bank[str(pathlib.Path(candidate))]=card
        return {"score_0_to_100":card["weighted_metric_score_0_to_100"],
                "components":{f["id"]:{"score":f["score_0_to_100"],
                    "blocked":f["gate_status"]!="METRIC_GATE_PASS"}
                    for f in card["features"]}}
    sweep=native.run(plan,out_dir/"NATIVE_NONDESTRUCTIVE_SWEEP.json",
                     submit=submit,game_guard=game_guard,score=scorer)
    if baseline_registration is None:
        raise RuntimeError("No native baseline registered")
    baseline=bank[sweep["baseline_image"]]
    trials=[{"label":rec["label"],"image":rec["native_image"],
             "gates":bank[rec["native_image"]]}
            for rec in sweep["results"]]
    ranked=old.select_compare(baseline,trials)
    best=ranked[0] if ranked else None
    if best is not None:
        best["feature_scorecard"]=gate.score_features(manifest,best["native_render"],
           rois=baseline_registration["candidate_rois"],
           report_dir=out_dir/"BEST_FEATURE_CROPS")
    report={"schema":"NOWORLD_KOIKATSU_REGISTERED_GATE_OPTIMIZER_V1",
        "feature_manifest_sha256":hashlib.sha256(
            json.dumps(manifest,sort_keys=True).encode()).hexdigest(),
        "reference":plan["reference"],"baseline":baseline,
        "candidates_ranked":ranked,
        "selected_for_human_inspection":
            best["label"] if best and best["eligible_for_user_review"] else None,
        "feature_count":len(baseline["features"]),
        "native_state_restored":sweep["baseline_restored"],
        "registration":baseline_registration,
        "all_gates_proven":False,
        "native_card_saved":False,
        "production_character_accepted":False,
        "status":"REGISTERED_METRIC_SHORTLIST_NOT_ARTISTIC_APPROVAL"}
    (out_dir/"REGISTERED_FEATURE_GATED_OPTIMIZATION.json").write_text(
        json.dumps(report,indent=2),encoding="utf-8")
    return report
