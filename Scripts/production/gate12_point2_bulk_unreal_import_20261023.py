"""Produce genuine native Koikatsu AnimSequences in bounded Unreal Editor commandlets.
Original runtime skeletons and source curve/Unity identities are immutable.
Resumes existing native assets; no QA approval, no generated substitute animation.
"""
import os,json,pathlib,runpy,traceback,unreal
ROOT=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
SRC=ROOT/"SourceAssets/Gate12/Animations20261023/KoikatsuCalibratedNativeTracks"
PLAN=ROOT/"SourceAssets/Gate12/Animations20261023/KoikatsuBulkNativeConversionPlan.json"
S=int(os.environ["G12_BULK_START"]);E=int(os.environ["G12_BULK_END"])
rows=json.loads(PLAN.read_text(encoding="utf8"))["motions"]
assert 0<=S<E<=len(rows) and E-S<=16
IMPORTER=ROOT/"Scripts/production/gate12_import_real_koikatsu_studio_anim_on_alter_native_ue583_20261022.py"
REPORT=ROOT/"Saved/Gate12Readiness"/f"G12_NATIVE_BULK_{S:04d}_{E:04d}.json"
doc={"schema":"noworld.gate12.true_koikatsu_native_batch.v1",
"start":S,"end":E,"status":"IN_PROGRESS",
"native_created_or_preserved":0,"unreal_native_sequences":[],"failures":[],
"approved_for_gameplay":False,"source_bundle_unchanged":True}
def save():
    REPORT.parent.mkdir(parents=True,exist_ok=True)
    tmp=REPORT.with_suffix(".tmp")
    tmp.write_text(json.dumps(doc,indent=2,default=str)+"\n",encoding="utf8")
    tmp.replace(REPORT)
save()
for idx in range(S,E):
    row=rows[idx];name=row["source_clip"]
    track=SRC/row["native_file"]
    if not track.exists():
        doc["failures"].append({"index":idx,"clip":name,"reason":"SOURCE_TRACKS_UNAVAILABLE"})
        save();continue
    for outfit in ("CLOTHED","UNDERWEAR","NUDE"):
        native=f"/Game/NoWorld/Characters/Luminous/Alter/KoikatsuOriginalStudioMotionQA/AN_G12_KK_Original_{outfit}_{name}"
        receipt=ROOT/"Saved/Gate12Readiness"/f"GATE12_NATIVE_KOIKATSU_STUDIO_{outfit}_{name}_20261022.json"
        try:
            asset=unreal.load_asset(native) if unreal.EditorAssetLibrary.does_asset_exist(native) else None
            if not isinstance(asset,unreal.AnimSequence):
                os.environ["G12_KK_SOURCE_STATE"]=outfit
                os.environ["G12_KK_SOURCE_CLIP"]=name
                runpy.run_path(str(IMPORTER),run_name="__main__")
                asset=unreal.load_asset(native)
            if not isinstance(asset,unreal.AnimSequence):
                raise RuntimeError("NATIVE_ASSET_NOT_BUILT")
            if not receipt.exists():raise RuntimeError("NATIVE_RECEIPT_MISSING")
            result=json.loads(receipt.read_text(encoding="utf8"))
            if result.get("status")!="TRUE_ORIGINAL_KK_STUDIO_CLIP_IMPORTED_AS_NATIVE_ANIMSEQUENCE_PRE_VISUAL":
                raise RuntimeError("NATIVE_ASSET_NOT_ACCEPTED:"+result.get("status","?"))
            if asset.get_editor_property("skeleton") is None:
                raise RuntimeError("NATIVE_SKELETON_MISSING")
            doc["unreal_native_sequences"].append({"index":idx,"clip":name,
                "wardrobe":outfit,"asset":asset.get_path_name(),
                "source_id":row["source_original_path_id"],
                "moving_bones":result.get("moving_bone_tracks"),"tracks":result.get("model_track_count")})
            doc["native_created_or_preserved"]+=1
        except Exception as exc:
            doc["failures"].append({"index":idx,"clip":name,"outfit":outfit,
                "error":str(exc)[:450],"trace":traceback.format_exc(limit=3)[-1000:]})
        save()
    if (idx-S)%2==1:
        unreal.SystemLibrary.collect_garbage()
    unreal.log("G12_BULK_NATIVE_PROGRESS %s/%s accepted=%s failed=%s" %
        (idx+1-S,E-S,doc["native_created_or_preserved"],len(doc["failures"])))
doc["status"]="NATIVE_BATCH_QA_GATED_COMPLETE" if not doc["failures"] else "NATIVE_BATCH_PARTIAL_OR_UNAVAILABLE"
save()
unreal.log("G12_BULK_NATIVE_DONE "+doc["status"]+" "+str(doc["native_created_or_preserved"]))
