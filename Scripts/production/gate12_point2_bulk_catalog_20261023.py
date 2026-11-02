"""Registers ONLY genuine already-authored UE AnimSequences in shared catalog.
Exact original source identity + native file hash; no inferred visual approval.
All new entries stay QA-inactive until actual in-game visual acceptance.
"""
import argparse,hashlib,json,pathlib,gzip
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
PLAN=R/"SourceAssets/Gate12/Animations20261023/KoikatsuBulkNativeConversionPlan.json"
CAT=R/"Content/NoWorld/Animation/SharedMotionCatalog.json"
NATIVE=R/"Content/NoWorld/Characters/Luminous/Alter/KoikatsuOriginalStudioMotionQA"
CURVES=R/"SourceAssets/Gate12/Animations20261023/KoikatsuCalibratedNativeTracks"
RECEIPTS=R/"Saved/Gate12Readiness"
def main(start,end):
    plan=json.loads(PLAN.read_text(encoding="utf8"))["motions"]
    assert 0<=start<end<=len(plan)
    data=json.loads(CAT.read_text(encoding="utf8"))
    assert data["schema"]=="noworld.shared_motion_catalog.v1"
    existing={x["motion_id"]:x for x in data["motions"]}
    byasset={x.get("native_sequence") for x in data["motions"]}
    stats={"new":0,"already":0,"not_native":0,"unsafe":0}
    for i in range(start,end):
        e=plan[i];clip=e["source_clip"]
        tracks=CURVES/e["native_file"]
        if not tracks.is_file():
            stats["not_native"]+=3
            continue
        with gzip.open(tracks,"rt",encoding="utf8") as f: body=json.load(f)
        if body["source_path_id"]!=e["source_original_path_id"] or body["clip"]!=clip or body.get("animated_rotation_bone_count",0)<12:
            stats["unsafe"]+=3;continue
        for state in ("CLOTHED","UNDERWEAR","NUDE"):
            filename=f"AN_G12_KK_Original_{state}_{clip}"
            asset=f"/Game/NoWorld/Characters/Luminous/Alter/KoikatsuOriginalStudioMotionQA/{filename}.{filename}"
            file=NATIVE/(filename+".uasset")
            receipt=RECEIPTS/f"GATE12_NATIVE_KOIKATSU_STUDIO_{state}_{clip}_20261022.json"
            if not file.is_file() or not receipt.is_file():
                stats["not_native"]+=1;continue
            info=json.loads(receipt.read_text(encoding="utf8"))
            if (info.get("status")!="TRUE_ORIGINAL_KK_STUDIO_CLIP_IMPORTED_AS_NATIVE_ANIMSEQUENCE_PRE_VISUAL" or
                info.get("native_animsequence")!=asset or
                info.get("source_unity_path_id")!=e["source_original_path_id"] or
                info.get("model_track_count",0)<140):
                stats["unsafe"]+=1;continue
            id="kk.source."+state+"."+clip
            if asset in byasset:
                stats["already"]+=1;continue
            if id in existing:raise RuntimeError("Source MotionId already occupied: "+id)
            entry={
                "motion_id":id,"source_clip_id":"koikatsu.unity."+str(e["source_original_path_id"]),
                "source_hash":hashlib.sha256(tracks.read_bytes()).hexdigest(),
                "source_bundle":body["source_bundle"],
                "source_unity_path_id":e["source_original_path_id"],
                "source_bundle_sha256":e["source_unity_bundle_hash"],
                "action":e["action"],"category":e["context"],"context":e["context"],
                "rig":"koikatsu_original","anatomy":"humanoid","outfit":state,
                "capabilities":[e["capability"]],"native_sequence":asset,
                "native_uasset_sha256":hashlib.sha256(file.read_bytes()).hexdigest(),
                "native_bone_tracks":info["model_track_count"],
                "native_duration_seconds":info["duration_s"],
                "loop":e["action"].startswith(("Emote.Idle.","Interaction.Sleep.","Traversal.Swim.")),
                "participants":1,"play_rate":1,
                "approved_for_gameplay":False,
                "adult_restricted":False,
                "review_state":"ORIGINAL_NATIVE_UNREAL_ASSET_IMPORTED_VISUAL_QA_PENDING"}
            data["motions"].append(entry);existing[id]=entry;byasset.add(asset);stats["new"]+=1
    tmp=CAT.with_suffix(".json.tmp")
    tmp.write_text(json.dumps(data,ensure_ascii=False,indent=2)+"\n",encoding="utf8")
    assert len({x["motion_id"] for x in data["motions"]})==len(data["motions"])
    json.loads(tmp.read_text(encoding="utf8"))
    tmp.replace(CAT)
    print("POINT2_BULK_CATALOG",stats,"ALL_CATALOG",len(data["motions"]),flush=True)

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--start",type=int,required=True)
    p.add_argument("--end",type=int,required=True)
    a=p.parse_args()
    main(a.start,a.end)
