"""Immutable second-pass real-native receipts; distinguishes face skin vs surrounding hair.

Reanalyses EXACT earlier full-resolution native screenshots, uses current sandbox
snapshot as an independent original-morph readback. Does not alter any parameter.
"""
import json,pathlib
import kk_native_parameter_calibration as cal
import kk_native_visual_search as native
import kk_one_session as master
ROOT=pathlib.Path(r"D:\UnrealProjects\NW_Blender_Private_Review_20261017\REFERENCE_GUIDED_QA\native_parameter_live_20261017_v1")
OUT=ROOT/"REASSESSMENT_V2_NATIVE_READBACK.json"
if OUT.exists():raise FileExistsError("Prior QA reassessment immutable")
old=json.loads((ROOT/"NATIVE_MORPH_CALIBRATION_SUMMARY.json").read_text())
master.game_guards(stage=False);master.profile_valid()
submit=native.client_factory()
nonce,_,snap=native.invoke(submit,{"op":"snapshot","args":{}},timeout=15)
state={}
for line in snap["data"].splitlines():
 t=line.split("\t")
 if len(t)>=3 and t[0] in ("face","body"):
  state[(t[0],int(t[1]))]=float(t[2])
audit=[]
for p in old["probes"]:
 record=json.loads(pathlib.Path(p["receipt"]).read_text())
 c=record["captures"]
 arr={k:cal.load_images(c[k]) for k in ("baseline","trial","restore")}
 head,_,_,_=cal.compute(arr["baseline"],arr["trial"],arr["restore"],
     [.17,.42,.83,.82],.007,.22)
 # Differentiates actual facial surface evidence from hairline/outer head movement.
 skin,_,_,_=cal.compute(arr["baseline"],arr["trial"],arr["restore"],
     [.33,.61,.69,.82],.006,.12)
 restored_morph=state[(p["group"],p["index"])]
 restored_exact=abs(restored_morph-p["original"])<.00001
 audit.append({"group":p["group"],"index":p["index"],
      "native_original_value":p["original"],"native_final_readback":restored_morph,
      "native_exact_restoration":restored_exact,
      "head_region":head,"face_skin_rectangle_proxy":skin,
      "native_visual_restore_inside_noise_envelope":head["restoration_visual_verified"],
      "face_structure_calibrated":None,
      "warning":"ROI is a skin-area rectangle, not semantic skin segmentation; identity not proven"})
report={"schema":"NW_REAL_NATIVE_MORPH_REASSESSMENT_V2",
    "source_capture_session":str(ROOT),
    "independent_native_snapshot_nonce":nonce,
    "controls":audit,
    "all_native_morphs_restored_exactly":all(q["native_exact_restoration"] for q in audit),
    "all_visual_returns_within_noise":all(q["native_visual_restore_inside_noise_envelope"] for q in audit),
    "reference_illustration_compared":False,"artistic_similarity_approved":False,
    "no_character_card_saves":True}
OUT.write_text(json.dumps(report,indent=2))
print("REAL_NATIVE_REASSESSMENT="+json.dumps({"native_restore_exact":report["all_native_morphs_restored_exactly"],
   "visual_returns_within_noise":report["all_visual_returns_within_noise"],
   "probes":[{"control":q["group"]+str(q["index"]),
     "head_change":q["head_region"]["local_changed_fraction"],
     "skin_proxy_change":q["face_skin_rectangle_proxy"]["local_changed_fraction"],
     "skin_proxy_localized":q["face_skin_rectangle_proxy"]["correctly_localized"],
     "restored":q["native_exact_restoration"]} for q in audit]}),flush=True)
if not report["all_native_morphs_restored_exactly"]:raise SystemExit(2)
