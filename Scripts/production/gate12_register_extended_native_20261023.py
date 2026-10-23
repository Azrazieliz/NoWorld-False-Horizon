"""Register only genuinely created native Unreal sequences from the installed
Mixamo CharaStudio and exact-ID H sources. Never auto-approve them.
"""
import argparse,gzip,hashlib,json,pathlib,os,sys
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
catalog=R/"Content/NoWorld/Animation/SharedMotionCatalog.json"
folder=R/"Content/NoWorld/Characters/Luminous/Alter/KoikatsuOriginalStudioMotionQA"
curves=R/"SourceAssets/Gate12/Animations20261023/KoikatsuCalibratedNativeTracks"
saved=R/"Saved/Gate12Readiness"
def run(family,start,end):
    plans={
      "mixamo":R/"SourceAssets/Gate12/Animations20261023/KoikatsuExtendedSourcePlan.json",
      "h_exact":R/"SourceAssets/Gate12/Animations20261023/KoikatsuHExactSourcePlan.json"}
    assert family in plans
    motions=json.loads(plans[family].read_text("utf8"))["motions"]
    if family=="h_exact" and os.environ.get("G12_H_FULL_ARCHIVE")=="1":
      sys.path.insert(0,str(R/"Scripts/production"))
      from gate12_h_original_exact_id_source_batch import createplan
      motions=createplan(full=True)
    assert 0<=start<end<=len(motions)
    doc=json.loads(catalog.read_text("utf8"))
    assert doc["schema"]=="noworld.shared_motion_catalog.v1"
    by_native={x["native_sequence"]:x for x in doc["motions"]}
    occupied=set(by_native)
    ids={x["motion_id"] for x in doc["motions"]}
    stats={"native_new":0,"existing":0,"not_native":0,"receipt_rejected":0,"source_loop_metadata_corrected":0}
    for row in motions[start:end]:
      alias=row["source_clip"]
      src=curves/row["native_file"]
      if not src.is_file():
        stats["not_native"]+=3;continue
      with gzip.open(src,"rt",encoding="utf8")as f:data=json.load(f)
      assert data["clip"]==alias and data["source_path_id"]==row["source_original_path_id"]
      with gzip.open(row["source_curve_path"],"rt",encoding="utf8") as raw_file:
        original_unity=json.load(raw_file)
      assert original_unity["original_unity_path_id"]==row["source_original_path_id"]
      source_loop=original_unity.get("unity_clip_attributes",{}).get("m_WrapMode")==2
      for outfit in ("CLOTHED","UNDERWEAR","NUDE"):
        name=f"AN_G12_KK_Original_{outfit}_{alias}"
        asset=f"/Game/NoWorld/Characters/Luminous/Alter/KoikatsuOriginalStudioMotionQA/{name}.{name}"
        binary=folder/(name+".uasset")
        receipt=saved/f"GATE12_NATIVE_KOIKATSU_STUDIO_{outfit}_{alias}_20261022.json"
        if not binary.is_file() or not receipt.is_file():
          stats["not_native"]+=1;continue
        rep=json.loads(receipt.read_text("utf8"))
        if rep.get("status")!="TRUE_ORIGINAL_KK_STUDIO_CLIP_IMPORTED_AS_NATIVE_ANIMSEQUENCE_PRE_VISUAL" or rep.get("source_unity_path_id")!=row["source_original_path_id"]:
          stats["receipt_rejected"]+=1;continue
        if asset in occupied:
          old=by_native[asset]
          assert not old["approved_for_gameplay"]
          if old.get("loop")!=source_loop:
            old["loop"]=source_loop
            stats["source_loop_metadata_corrected"]+=1
          stats["existing"]+=1
          continue
        key=f"kk.{family}.qa.{outfit}.{alias}"
        assert key not in ids
        action=row["action"]
        context=row["context"]
        category="AdultInteraction" if family=="h_exact" else context
        entry={
          "motion_id":key,
          "source_clip_id":f"koikatsu.installed.{family}.{row['source_unity_bundle_index']}.{row['source_original_path_id']}",
          "source_hash":data["source_clip_sha256"],
          "source_bundle":data["source_bundle"],
          "source_bundle_sha256":data["source_sha256"],
          "source_unity_path_id":row["source_original_path_id"],
          "source_original_name":row["original_clip_name"],
          "native_sequence":asset,
          "native_uasset_sha256":hashlib.sha256(binary.read_bytes()).hexdigest(),
          "native_bone_tracks":rep["model_track_count"],
          "native_duration_seconds":rep["duration_s"],
          "action":action,"category":category,"context":context,
          "rig":"koikatsu_original","anatomy":"humanoid","outfit":outfit,
          "capabilities":[row["capability"]],
          "loop":source_loop,"participants":1,
          "approved_for_gameplay":False,
          "adult_restricted":family=="h_exact",
          "review_state":"EXACT_H_SOURCE_REQUIRES_PAIR_SCENE_CONSENT_RIG_AND_VISUAL_QA" if family=="h_exact" else "MIXAMO_SOURCED_REDUCED_RIG_REQUIRES_VISUAL_QA",
          "priority_if_approved":1}
        doc["motions"].append(entry);ids.add(key);occupied.add(asset);by_native[asset]=entry
        stats["native_new"]+=1
    assert len(ids)==len(doc["motions"])
    target=catalog.with_suffix(".json.tmp")
    target.write_text(json.dumps(doc,ensure_ascii=False,indent=2)+"\n",encoding="utf8")
    json.loads(target.read_text("utf8"))
    target.replace(catalog)
    print("EXTENDED_NATIVE_CATALOG",family,start,end,stats,"TOTAL",len(doc["motions"]),flush=True)
if __name__=="__main__":
    a=argparse.ArgumentParser();a.add_argument("--family",choices=("mixamo","h_exact"),required=True)
    a.add_argument("--start",type=int,required=True);a.add_argument("--end",type=int,required=True)
    args=a.parse_args();run(args.family,args.start,args.end)
