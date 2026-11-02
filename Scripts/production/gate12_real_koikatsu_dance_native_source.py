"""Preserve genuine installed Koikatsu full dance motion clips, excluding
previously rejected live_dance and pole_dance. Full original timing,
343-bone Unity sources; native QA candidates are never auto-approved.
"""
import json,pathlib,sys,gzip
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
sys.path.insert(0,str(R/"Scripts/production"))
import gate12_decode_genuine_koikatsu_studio_to_native_joint_curves_20261022 as core
INPUT=R/"SourceAssets/Gate12/Animations20261021/ImportedGate09OriginalUnityCurves/GATE09_ALL_SOURCE_INTAKE_PROGRESS.json"
OUT=R/"SourceAssets/Gate12/Animations20261023/KoikatsuCalibratedNativeTracks"
PLAN=R/"SourceAssets/Gate12/Animations20261023/KoikatsuFullDanceSourcePlan.json"
rows=json.loads(INPUT.read_text("utf8"))["source_files"]
chosen=[r for r in rows if r["source_bundle_index"]==1
        and ("dance" in r["source_clip_name"].casefold())
        and r["source_clip_name"] not in ("live_dance","pole_dance_00")]
assert len(chosen)==7,(len(chosen),[e["source_clip_name"] for e in chosen])
plan=[]
for r in chosen:
    alias=f"KKD_{r['source_bundle_index']:03d}_{r['original_clip_path_id'] & ((1<<64)-1):016x}"
    plan.append({"source_clip":alias,"original_clip_name":r["source_clip_name"],
        "source_unity_bundle_index":r["source_bundle_index"],
        "source_original_path_id":r["original_clip_path_id"],
        "source_unity_bundle_hash":r["source_hash"],
        "source_curve_path":r["raw_curve_path"],
        "native_file":alias+".genuine_unity_kk_stream_native_deltas.json.gz",
        "action":"Interaction.Dance."+alias,"context":"Interaction",
        "capability":"dance","category":"Interaction","gameplay_approved":False,
        "review_state":"REQUIRES_FULL_ARTISTIC_VISUAL_ACCEPTANCE"})
if PLAN.exists():
    old=json.loads(PLAN.read_text("utf8"))["motions"]
    assert [(x["source_clip"],x["source_original_path_id"])for x in old] == [(x["source_clip"],x["source_original_path_id"])for x in plan]
else:
    PLAN.write_text(json.dumps({"schema":"noworld.gate12.authentic_koikatsu_full_dance.v1",
        "motions":plan,"rejected_sources_reintroduced":False,
        "visual_approval":False},indent=2)+"\n",encoding="utf8")
core.OUT=OUT
ref=core.source_bone_catalog()
for p in plan:
    path=OUT/p["native_file"]
    if path.exists():
        with gzip.open(path,"rt",encoding="utf8")as f:data=json.load(f)
        assert data["source_path_id"]==p["source_original_path_id"]
        print("DANCE_PRESERVED",p["original_clip_name"],flush=True)
        continue
    core.convert(p["original_clip_name"],ref,
        source_row={"source_clip_name":p["original_clip_name"],
            "source_bundle_index":p["source_unity_bundle_index"],
            "raw_curve_path":p["source_curve_path"],
            "source_hash":p["source_unity_bundle_hash"]},
        output_alias=p["source_clip"])
    print("DANCE_CALIBRATED",p["original_clip_name"],flush=True)
print("DANCE_SOURCE_GROUPS_COMPLETE",len(plan),flush=True)
