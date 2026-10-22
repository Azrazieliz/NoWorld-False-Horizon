"""Author original Koikatsu installed STUDIO animation clips into true native
UE AnimSequence assets, three approved Alter wardrobe rigs, batch QA intake.
Character gameplay and rejected dance assets never modified.
"""
import os,runpy,json,pathlib,unreal,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
PHASE=int(os.environ["G12_KK_STUDIO_PHASE"])
ALL=("Idle_03_01","Idle_00_00","f_Land_00_00","live_dance",
     "f_talk_00_13_01","Stand_Oth_00","f_sleep_00_03_L","f_action_00_00")
assert PHASE in (0,1,2)
CLIPS=ALL[:4] if PHASE==0 else ALL[4+2*(PHASE-1):6+2*(PHASE-1)]
SCRIPT=R/"Scripts/production/gate12_import_real_koikatsu_studio_anim_on_alter_native_ue583_20261022.py"
REC=R/"Saved/Gate12Readiness"/f"GATE12_NATIVE_ORIGINAL_KOIKATSU_STUDIO_BULK_PART_{PHASE}_20261022.json"
report={"schema":"noworld.gate12.real.kk.ue.animation.preview.v1","batch":PHASE,
"clips":list(CLIPS),"status":"STARTED","results":[],
"native_genuine_koikatsu_motion":True,"approved_gameplay_motion":False,
"rejected_dance_enabled":False,"original_approved_character_unchanged":True}
try:
 for clip in CLIPS:
  for state in ("CLOTHED","UNDERWEAR","NUDE"):
   receipt=R/"Saved/Gate12Readiness"/f"GATE12_NATIVE_KOIKATSU_STUDIO_{state}_{clip}_20261022.json"
   if receipt.exists():
    value=json.loads(receipt.read_text(encoding="utf8"))
    if value["status"]=="TRUE_ORIGINAL_KK_STUDIO_CLIP_IMPORTED_AS_NATIVE_ANIMSEQUENCE_PRE_VISUAL":
     report["results"].append({"clip":clip,"state":state,"status":value["status"],
        "asset":value["native_animsequence"],"tracks":value["model_track_count"],
        "preserved_existing":True})
     continue
   os.environ["G12_KK_SOURCE_STATE"]=state
   os.environ["G12_KK_SOURCE_CLIP"]=clip
   try:runpy.run_path(str(SCRIPT),run_name="__main__")
   except Exception:
    unreal.log_error("GATE12_KK_SOURCE_CALL_FAILED "+state+" "+clip+" "+traceback.format_exc()[-1000:])
   if receipt.exists():
    value=json.loads(receipt.read_text(encoding="utf8"))
    report["results"].append({"clip":clip,"state":state,"status":value["status"],
        "asset":value.get("native_animsequence"),
        "tracks":value.get("model_track_count"),"error":value.get("error")})
   else:report["results"].append({"clip":clip,"state":state,"status":"NO_UE_ACCEPTANCE_RECEIPT"})
   unreal.log("GATE12_KK_REAL_SOURCE_BATCH_PROGRESS "+str(len(report["results"]))+"/"+str(len(CLIPS)*3))
 report["accepted_native_sequences"]=sum(x["status"]=="TRUE_ORIGINAL_KK_STUDIO_CLIP_IMPORTED_AS_NATIVE_ANIMSEQUENCE_PRE_VISUAL"for x in report["results"])
 report["status"]="BATCH_ALL_REAL_UNREAL_ANIMSEQUENCES_PRODUCED_PENDING_VISUAL" if report["accepted_native_sequences"]==len(CLIPS)*3 else "PARTIAL_UE_ANIM_INTAKE_NOT_ALL_NATIVE_VALID"
except Exception:
 report["status"]="PARTIAL_UE_ANIM_INTAKE_RUNTIME_EXCEPTION"
 report["error"]=traceback.format_exc()[-3500:]
finally:
 REC.parent.mkdir(parents=True,exist_ok=True)
 REC.write_text(json.dumps(report,indent=2,default=str)+"\n",encoding="utf8")
 unreal.log("GATE12_KOIKATSU_ORIGINAL_NATIVE_ANIM_BATCH="+report["status"])
