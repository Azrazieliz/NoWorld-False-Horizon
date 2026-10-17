"""Single-session noncanonical native Koikatsu shape calibration proof.

Loads ACTUAL current Maker face/body morph values from a native snapshot, tests
two changes separately and reverts to the read values. No card saves.
"""
from __future__ import annotations
import json,pathlib,time
import kk_native_visual_search as native
import kk_native_parameter_calibration as cal
import kk_one_session as master

ROOT=pathlib.Path(r"D:\UnrealProjects\NW_Blender_Private_Review_20261017\REFERENCE_GUIDED_QA\native_parameter_live_20261017_v1")
if ROOT.exists():raise FileExistsError("Do not replace existing real native receipt")
master.profile_valid();master.game_guards(stage=False)
submit=native.client_factory()
ROOT.mkdir(parents=True)
nonce,elapsed,snap=native.invoke(submit,{"op":"snapshot","args":{}},timeout=22)
data=snap["data"]
state={}
for line in data.splitlines():
    parts=line.split("\t")
    if len(parts)>=3 and parts[0] in {"face","body"}:
        state[(parts[0],int(parts[1]))]=float(parts[2])
probes=[
    {"label":"FaceBaseW native index 0 width","group":"face","index":0,
     "roi":[.17,.42,.83,.82],"min_fraction":.007,"min_share":.22},
    {"label":"Body HeadSize native index 1","group":"body","index":1,
     "roi":[.15,.40,.86,.88],"min_fraction":.008,"min_share":.24}]
summary=[]
for p in probes:
    group=p["group"];idx=p["index"]
    original=state[(group,idx)]
    if not 0<=original<=1:raise RuntimeError("Original morph outside conservative 0..1; stop and preserve")
    candidate=.95 if original<.55 else .05
    base={"op":"apply_morph_batch","args":{"morphs":[{"group":group,"index":idx,"value":original}]}}
    trial={"op":"apply_morph_batch","args":{"morphs":[{"group":group,"index":idx,"value":candidate}]}}
    plan={"schema":cal.SCHEMA,"view":"face","label":p["label"],
        "restore_baseline":[base],"trial_steps":[trial],
        "target_roi_norm":p["roi"],
        "min_local_delta_fraction":p["min_fraction"],
        "min_local_change_share":p["min_share"]}
    dest=ROOT/(group+"_"+str(idx))
    result=cal.run(plan,dest,submit=submit)
    summary.append({"group":group,"index":idx,"original":original,"trial":candidate,
        "calibrated":result["calibrated_native_effect"],
        "restored":result["baseline_restoration_verified"],
        "measurement":result["observations"],
        "receipt":str(dest/"NATIVE_PARAMETER_CALIBRATION.json")})
    if not result["baseline_restoration_verified"]:
        break
report={"schema":"NOWORLD_REAL_NATIVE_MORPH_CALIBRATION_SESSION_V1",
    "source_snapshot_native_nonce":nonce,"source_snapshot_sec":elapsed,
    "native_real_koikatsu":True,"source_install_untouched":True,
    "source_card_saved":False,"probes":summary,
    "tested_both":len(summary)==len(probes),
    "all_baselines_restored":all(x["restored"] for x in summary) and len(summary)==len(probes),
    "artistic_reference_reconstruction_proven":False}
(ROOT/"NATIVE_MORPH_CALIBRATION_SUMMARY.json").write_text(json.dumps(report,indent=2))
print("REAL_NATIVE_MORPH_VISUAL_PROOF="+json.dumps(report),flush=True)
if not report["all_baselines_restored"]:raise SystemExit(2)
