"""Convert original installed Koikatsu Party H-animation clips by bundle/path ID,
not ambiguous name. Uses original Koikatsu rig only for female-compatible tracks.
Male/prop rigs stay inventoried until a verified rig is available.
No eligibility, choreography or visual QA is implicitly approved.
"""
import argparse,collections,gzip,json,pathlib,re,sys
ROOT=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
sys.path.insert(0,str(ROOT/"Scripts/production"))
import gate12_decode_genuine_koikatsu_studio_to_native_joint_curves_20261022 as core
INDEX=ROOT/"SourceAssets/Gate12/Animations20261021/KoikatsuHOriginalSourceCurves/GATE12_INSTALLED_H_ANIMATION_SOURCE_CURVE_INDEX_COMPLETE.json"
TARGET=ROOT/"SourceAssets/Gate12/Animations20261023/KoikatsuCalibratedNativeTracks"
PLAN=ROOT/"SourceAssets/Gate12/Animations20261023/KoikatsuHExactSourcePlan.json"
def createplan(full=False):
    raw=json.loads(INDEX.read_text("utf8"));items=raw["clip_rows"]
    female=[r for r in items if "\\female\\" in r["source_bundle"].lower()]
    male=[r for r in items if "\\male\\" in r["source_bundle"].lower()]
    prop_count=len(items)-len(female)-len(male)
    # Broad distribution rather than all variants from a single scene.
    per_bundle=collections.Counter();selected=[]
    for row in female:
        b=row["bundle_index"]
        if not full and per_bundle[b]>=8:continue
        per_bundle[b]+=1
        number=row["native_path_id"] & ((1<<64)-1)
        alias=f"KKH_F_{b:03d}_{number:016x}"
        selected.append({
          "source_clip":alias,"original_clip_name":row["clip_name"],
          "source_original_path_id":row["native_path_id"],
          "source_unity_bundle_index":b,
          "source_unity_bundle_hash":row["source_digest_sha256"],
          "source_curve_path":row["curve_file"],
          "native_file":alias+".genuine_unity_kk_stream_native_deltas.json.gz",
          "action":"AdultInteraction.SourceClip."+alias,
          "category":"AdultInteraction","context":"AdultInteraction","capability":"adult_interaction",
          "source_role":"FEMALE_RIG_ONLY","adult_restricted":True,
          "scene_pairing_approved":False,"gameplay_approved":False,
          "priority":0})
    if full:
        return selected
    if not PLAN.exists():
        PLAN.write_text(json.dumps({
           "schema":"noworld.gate12.original_kk_h_exact_path_id_plan.v1",
           "archive_total":len(items),"female_source_objects":len(female),
           "male_source_objects":len(male),"prop_source_objects":prop_count,
           "male_native_rig_available":False,
           "scene_pairing_approved":False,
           "motions":selected},indent=2)+"\n",encoding="utf8")
    else:
        previous=json.loads(PLAN.read_text("utf8"))["motions"]
        assert [(r["source_clip"],r["source_original_path_id"]) for r in previous] == [(r["source_clip"],r["source_original_path_id"]) for r in selected]
    return selected
def main(start,end,full=False):
    plan=createplan(full=full)
    assert 0<=start<end<=len(plan)
    core.OUT=TARGET;TARGET.mkdir(parents=True,exist_ok=True)
    rest=core.source_bone_catalog()
    report={"start":start,"end":end,"status":"IN_PROGRESS","results":[]}
    receipt=TARGET/(f"HSourceFull_{start:06d}_{end:06d}.json" if full else f"HSourceExact_{start:04d}_{end:04d}.json")
    for index in range(start,end):
        row=plan[index];out=TARGET/row["native_file"]
        try:
            if out.is_file():
                with gzip.open(out,"rt",encoding="utf8") as f:stored=json.load(f)
                assert stored["source_path_id"]==row["source_original_path_id"]
                status="PRESERVED"
            else:
                core.convert(row["original_clip_name"],rest,
                    source_row={"source_clip_name":row["original_clip_name"],
                        "source_bundle_index":row["source_unity_bundle_index"],
                        "raw_curve_path":row["source_curve_path"],
                        "source_hash":row["source_unity_bundle_hash"]},
                    output_alias=row["source_clip"])
                status="CALIBRATED_EXACT_H_SOURCE"
        except Exception as err:
            status="INCOMPATIBLE_"+str(type(err).__name__)+":"+str(err)[:120]
        report["results"].append({"index":index,"alias":row["source_clip"],"status":status})
        receipt.write_text(json.dumps(report,indent=2)+"\n",encoding="utf8")
        print("H_SOURCE",index,status,flush=True)
    report["status"]="SOURCE_BATCH_DONE"
    receipt.write_text(json.dumps(report,indent=2)+"\n",encoding="utf8")
    print("H_SOURCE_BATCH_DONE",start,end,"pass",sum(x["status"] in ("CALIBRATED_EXACT_H_SOURCE","PRESERVED") for x in report["results"]),"total_selected",len(plan),flush=True)
if __name__=="__main__":
    a=argparse.ArgumentParser();a.add_argument("--start",type=int,required=True);a.add_argument("--end",type=int,required=True)
    a.add_argument("--full",action="store_true",help="Address every exact original female source object, not eight per bundle")
    n=a.parse_args();main(n.start,n.end,full=n.full)
