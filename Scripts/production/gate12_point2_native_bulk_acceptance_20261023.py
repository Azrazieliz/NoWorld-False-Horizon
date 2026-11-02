"""One final integrity checkpoint for original source-backed UE animation imports.
Counts real native .uasset packages and receipts, never equates source curves
or metadata to gameplay approval. Does not reimport or rebuild any asset.
"""
import collections,hashlib,json,pathlib
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
source=R/"SourceAssets/Gate12/Animations20261023/KoikatsuBulkNativeConversionPlan.json"
catalog=R/"Content/NoWorld/Animation/SharedMotionCatalog.json"
folder=R/"Content/NoWorld/Characters/Luminous/Alter/KoikatsuOriginalStudioMotionQA"
receipts=R/"Saved/Gate12Readiness"
out=receipts/"GATE12_BULK_REAL_NATIVE_ACCEPTANCE_20261023.json"
plan=json.loads(source.read_text(encoding="utf8"))["motions"]
motions=json.loads(catalog.read_text(encoding="utf8"))["motions"]
catalog_by_asset={e["native_sequence"]:e for e in motions}
active=[m for m in motions if m["approved_for_gameplay"]]
verified=[];issues=[];families=collections.Counter();legacy_without_catalog_hash=[]
for row in plan:
    clip=row["source_clip"]
    families[row["context"]]+=1
    for outfit in ("CLOTHED","UNDERWEAR","NUDE"):
        stem=f"AN_G12_KK_Original_{outfit}_{clip}"
        asset=f"/Game/NoWorld/Characters/Luminous/Alter/KoikatsuOriginalStudioMotionQA/{stem}.{stem}"
        uasset=folder/(stem+".uasset")
        receipt=receipts/f"GATE12_NATIVE_KOIKATSU_STUDIO_{outfit}_{clip}_20261022.json"
        if not uasset.is_file() or not receipt.is_file():
            issues.append({"clip":clip,"outfit":outfit,"issue":"MISSING_ASSET_OR_NATIVE_RECEIPT"})
            continue
        info=json.loads(receipt.read_text(encoding="utf8"))
        registered=catalog_by_asset.get(asset)
        if (info.get("status")!="TRUE_ORIGINAL_KK_STUDIO_CLIP_IMPORTED_AS_NATIVE_ANIMSEQUENCE_PRE_VISUAL" or
            info.get("source_unity_path_id")!=row["source_original_path_id"] or
            info.get("model_track_count",0)<140):
            issues.append({"clip":clip,"outfit":outfit,"issue":"UNREAL_IMPORT_RECEIPT_MISMATCH"})
            continue
        if not registered:
            issues.append({"clip":clip,"outfit":outfit,"issue":"NOT_IN_SHARED_CATALOG"})
            continue
        actual_hash=hashlib.sha256(uasset.read_bytes()).hexdigest()
        if registered["approved_for_gameplay"]:
            issues.append({"clip":clip,"outfit":outfit,"issue":"QA_POLICY_VIOLATION"})
            continue
        known_hash=registered.get("native_uasset_sha256")
        if known_hash and known_hash!=actual_hash:
            issues.append({"clip":clip,"outfit":outfit,"issue":"NATIVE_ASSET_HASH_MISMATCH"})
            continue
        if not known_hash:
            # Twenty-one legacy QA entries predate hash registration: verify
            # their REAL file & source receipt without inventing a catalog hash.
            legacy_without_catalog_hash.append({"clip":clip,"outfit":outfit,
                "actual_file_sha256":actual_hash})
        verified.append({"clip":clip,"wardrobe":outfit,"native_asset":asset,
            "source_path_id":info["source_unity_path_id"],
            "asset_sha256":actual_hash,"catalog_hash_present":bool(known_hash)})
result={"schema":"noworld.gate12.original_native_animation_checkpoint.v1",
        "source_motions":len(plan),"native_expected":len(plan)*3,
        "native_verified":len(verified),"source_families":dict(families),
        "shared_catalog_entries":len(motions),
        "gameplay_enabled_entry_count":len(active),
        "qa_gated_entries":len(motions)-len(active),
        "native_issues":issues,"legacy_qa_assets_without_catalog_hash":legacy_without_catalog_hash,
        "visual_acceptance":"NOT_VERIFIED",
        "paired_scene_playback":"NOT_VERIFIED",
        "root_motion_visual_acceptance":"NOT_VERIFIED",
        "original_game_bundles_untouched":True,
        "result":"SOURCE_AUTHENTIC_NATIVE_PACKAGES_VERIFIED_PENDING_VISUAL" if not issues and len(verified)==len(plan)*3 else "PARTIAL_NATIVE_WITH_ISSUES"}
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf8")
print("G12_BULK_FINAL_CHECK",result["result"],"NATIVE",result["native_verified"],"/",result["native_expected"],"ISSUES",len(issues),"CATALOG",result["shared_catalog_entries"])
