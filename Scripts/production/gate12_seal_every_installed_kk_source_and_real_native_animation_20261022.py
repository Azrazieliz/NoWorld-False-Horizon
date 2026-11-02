"""Seal every installed Koikatsu original Unity AnimationClip source
and the genuinely created native Unreal sequences. Never label compressed
Unity source clips as native UAnimSequence; preserve 8GB workstation limits.
"""
import collections,hashlib,json,pathlib,sys,os,datetime
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
A=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party\abdata")
H=R/"SourceAssets/Gate12/Animations20261021/KoikatsuHOriginalSourceCurves/GATE12_INSTALLED_H_ANIMATION_SOURCE_CURVE_INDEX_COMPLETE.json"
O=R/"SourceAssets/Gate12/Animations20261022/KoikatsuAdditionalOriginalUnityCurves"
S=R/"SourceAssets/Gate12/Animations20261021/ImportedGate09OriginalUnityCurves/GATE09_ALL_SOURCE_INTAKE_PROGRESS.json"
OUT=R/"SourceAssets/Gate12/Animations20261022/GATE12_ALL_INSTALLED_KK_ORIGINAL_ANIMATION_NATIVE_INGESTION_20261022.json"
assert not OUT.exists(),"Preserve original catalog receipt"
h=json.loads(H.read_text(encoding="utf8"))
assert h["status"]=="ALL_INSTALLED_H_SOURCE_CURVES_CAPTURED_NOT_YET_RETARGETED"
assert h["actual_native_curve_clip_count"]==59218 and len(h["clip_rows"])==59218
boundaries=((0,80),(80,500),(500,1100),(1100,1397))
additional=[];batches=[]
for start,end in boundaries:
 f=O/f"GATE12_ALL_INSTALLED_KK_ADDITIONAL_ANIMATIONS_PART_{start:04d}_{end:04d}.json"
 assert f.is_file(),f
 d=json.loads(f.read_text(encoding="utf8"))
 assert d["batch_start"]==start and d["batch_end"]==end
 assert d["progress_next_index"]==end
 assert d["status"]=="SOURCE_ARCHIVE_COMPLETE_NOT_NATIVE_UNREAL",d["counts"]
 assert d["counts"].get("BUNDLE_FAILED",0)==0 and d["counts"].get("CLIP_FAILED",0)==0
 assert sum("native_curve_path" in r for r in d["clips"])==d["clip_count"]
 for row in d["clips"]:
  if "native_curve_path" in row:
   assert pathlib.Path(row["native_curve_path"]).is_file(),row
   additional.append(row)
 batches.append({"start":start,"end":end,
  "clips":d["clip_count"],"source_manifest":str(f),
  "source_manifest_sha256":hashlib.sha256(f.read_bytes()).hexdigest()})
assert sum(x["clips"] for x in batches)==len(additional)
studio=json.loads(S.read_text(encoding="utf8"))
assert studio["status"]=="ALL_SOURCE_BUNDLES_PROCESSED"
rows=studio["source_files"]
assert len(rows)==2741
def key(p,uid):
 return (str(p).replace("\\","/").casefold(),int(uid))
hkeys={key(x["source_bundle"],x["native_path_id"])for x in h["clip_rows"]}
addedkeys={key(A/x["source_bundle"],x["path_id"])for x in additional}
inventory=json.loads((pathlib.Path(r"D:\UnrealProjects\Gate09_Protected_Production_20261016")/"G09_InstalledAnimationInventory_ReadOnly_20261020.json").read_text(encoding="utf8"))
assert len(inventory["bundles"])==47
studiokeys={key(inventory["bundles"][x["source_bundle_index"]]["path"],x["original_clip_path_id"])for x in rows}
assert len(hkeys)==len(h["clip_rows"])
assert len(addedkeys)==len(additional)
native=[]
for batch in (0,1,2):
 f=R/"Saved/Gate12Readiness"/f"GATE12_NATIVE_ORIGINAL_KOIKATSU_STUDIO_BULK_PART_{batch}_20261022.json"
 d=json.loads(f.read_text(encoding="utf8"))
 expected=(12 if batch==0 else 6)
 assert d["status"]=="BATCH_ALL_REAL_UNREAL_ANIMSEQUENCES_PRODUCED_PENDING_VISUAL"
 assert d["accepted_native_sequences"]==expected and len(d["results"])==expected
 for v in d["results"]:
  assert v["status"]=="TRUE_ORIGINAL_KK_STUDIO_CLIP_IMPORTED_AS_NATIVE_ANIMSEQUENCE_PRE_VISUAL"
  rel=v["asset"].split(".")[0].replace("/Game/","Content/")+".uasset"
  p=R/rel
  assert p.is_file() and p.stat().st_size>1000,(v,p)
  native.append({
   "clip":v["clip"],"state":v["state"],"uasset":v["asset"],
   "original_imported_tracks":v["tracks"],
   "uasset_file_size":p.stat().st_size
  })
assert len(native)==24
catalog={
"schema":"noworld.gate12.original_installed_koikatsu.animation_intake.v1",
"created_at_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
"status":"ALL_LOCAL_ORIGINAL_CLIP_SOURCES_CAPTURED_AND_24_GENUINE_NATIVE_UNREAL_PREVIEW_CLIPS_IMPORTED",
"installed_base_unity_bundles":1685,
"installed_base_h_animation_bundles":288,
"installed_additional_bundles":1397,
"installed_h_original_curve_objects":len(hkeys),
"other_installed_original_curve_objects":len(addedkeys),
"base_total_original_clip_objects":len(hkeys|addedkeys),
"gate09_preexisting_animation_mod_and_studio_objects":len(studiokeys),
"exact_source_objects_total_including_unique_mods":len(hkeys|addedkeys|studiokeys),
"overlap_of_existing_gate09_with_installed_base":len(studiokeys & (hkeys|addedkeys)),
"source_bundle_files_unmodified":True,
"source_original_gz_unmodified":True,
"original_KK_animsequence_imported_native_UE_now":len(native),
"imported_native_UE_preview_clips_distinct":len({x["clip"] for x in native}),
"rigs":["alter CLOTHED 1131","alter UNDERWEAR 1131","alter NUDE 1123"],
"native_clips_user_approved":False,
"native_animation_source_lossless_for_every_clip":False,
"studio_native_rotation_only_not_original_unity_translation_or_scale":True,
"original_h_context_not_attached_to_characters":True,
"animation_assets_not_equivalent_to_source_clip_inventory":True,
"pending_native_conversion":len(hkeys|addedkeys|studiokeys)-len({x["clip"] for x in native}),
"unapproved_dance_gameplay_binding_untouched":True,
"source_H_index_sha256":hashlib.sha256(H.read_bytes()).hexdigest(),
"original_H_source_index":str(H),"original_gate09_index":str(S),
"additional_source_batches":batches,
"native_unreal_assets":native
}
assert catalog["base_total_original_clip_objects"]==61432,catalog
OUT.write_text(json.dumps(catalog,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
print("BASE_CLIPS",catalog["base_total_original_clip_objects"],
      "GATE09_SOURCE",catalog["gate09_preexisting_animation_mod_and_studio_objects"],
      "TOTAL_UNIQUE",catalog["exact_source_objects_total_including_unique_mods"],
      "NATIVE_UE",len(native),"STATUS",catalog["status"],flush=True)
