"""Native Koikatsu white-camera feedback smoke on an ephemeral test-only Maker state.

Target is generated from the SAME source rig/color for transport/scoring regression.
This does NOT prove anime-reference reconstruction from arbitrary 2D artwork.
No card saving, original Party edits or change to installed mods.
"""
import json,pathlib,contextlib,io,time
from datetime import datetime
import kk_native_visual_search as sweep
import kk_one_session as master
ROOT=pathlib.Path(r"D:\UnrealProjects\NW_Blender_Private_Review_20261017\REFERENCE_GUIDED_QA")
ROOT.mkdir(parents=True,exist_ok=True)
OUT=ROOT/"native_hair_visual_feedback_smoke_20261017"
OUT.mkdir(exist_ok=True)
master.profile_valid();master.game_guards(stage=False)
submit=sweep.client_factory()
reference_state=[
  {"op":"set_hair","args":{"index":0,"effective_id":19}},
  {"op":"set_hair_palette","args":{"index":0,"value":.82,"v2":.86,"v3":.90}}]
for step in reference_state:sweep.invoke(submit,step,timeout=12)
nonce,t,receipt=sweep.invoke(submit,{"op":"render_camera_view","args":{"group":"face"}},timeout=15)
reference=master.RUNTIME/"camera_bank"/("SM01_CAM_"+nonce+"_face.png")
if not reference.is_file():raise RuntimeError("Maker did not generate source face")
# Retain original full-res 768x1024 face render as the test reference.
plan={
 "schema":"NOWORLD_NATIVE_VISUAL_SWEEP_V1","reference":str(reference),
 "view":"face","restore_baseline":reference_state,
 "candidates":[
  {"label":"red_mismatch","steps":[{"op":"set_hair_palette","args":{"index":0,"value":.80,"v2":.11,"v3":.12}}]},
  {"label":"blue_mismatch","steps":[{"op":"set_hair_palette","args":{"index":0,"value":.1,"v2":.20,"v3":.82}}]},
  {"label":"gold_mismatch","steps":[{"op":"set_hair_palette","args":{"index":0,"value":.83,"v2":.60,"v3":.24}}]},
  {"label":"reference_silver_recovery","steps":[{"op":"set_hair_palette","args":{"index":0,"value":.82,"v2":.86,"v3":.90}}]},
  {"label":"dark_plum_mismatch","steps":[{"op":"set_hair_palette","args":{"index":0,"value":.19,"v2":.09,"v3":.23}}]}
 ]}
planfile=OUT/"source_hair_visual_plan.json"
if planfile.exists():raise FileExistsError(planfile)
planfile.write_text(json.dumps(plan,indent=2))
sweep.parse_spec(planfile)
result=sweep.run(plan,OUT/"native_sweep_results.json",submit=submit)
if result["best"] is None:raise RuntimeError("No candidate score obtained")
best=result["best"]["label"]
# The program must choose the ground-truth silver color, if chroma is truly responsive.
passed=best=="reference_silver_recovery"
proof={"schema":"NOWORLD_SM01_REAL_NATIVE_VISUAL_FEEDBACK_SMOKE_V1",
 "real_Koikatsu_runtime_used":True,"camera_view":"face","candidate_count":5,
 "reference_full_resolution":str(reference),
 "expected_best":"reference_silver_recovery",
 "actual_best":best,
 "best_matches_ground_truth":passed,
 "baseline_restored":result["baseline_restored"],
 "results":[{"label":v["label"],"score":v["score"],"path":v["native_image"]} for v in result["results"]],
 "total_seconds":result["total_elapsed_seconds"],
 "source_card_saved":False,"background_reference_external":False,
 "claim_reconstruction_of_arbitrary_anime_reference":False}
(OUT/"native_visual_proof.json").write_text(json.dumps(proof,indent=2))
print("REAL_KOIKATSU_VISUAL_FEEDBACK_PROOF="+json.dumps(proof),flush=True)
if not passed:raise SystemExit(2)
