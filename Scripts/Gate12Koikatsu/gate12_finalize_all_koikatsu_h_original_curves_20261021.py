"""Join original Koikatsu H source-curve intake manifests after bounded,
non-overlapping parallel capture. No game simulation or content fabrication.
"""
import pathlib,json,collections,os
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
D=R/"SourceAssets/Gate12/Animations20261021/KoikatsuHOriginalSourceCurves"
BASE="GATE12_INSTALLED_H_ANIMATION_SOURCE_CURVE_INDEX"
SHARDS=[("",0,94),("_A",94,146),("_B",146,228),("_C",228,288)]
out=D/(BASE+"_COMPLETE.json")
report={"schema":"noworld.gate12.kkh_local_source_manifest_join.v1",
    "expected_original_bundles":288,"source_bundle_paths":[],
    "clip_rows":[],"actual_native_curve_clip_count":0,
    "unreal_retarged_asset_count":0,
    "source_clip_path_collision_count":0,"status":"NOT_COMPLETE"}
seen=set();source_paths={}
for suffix,start,end in SHARDS:
    file=D/(BASE+suffix+".json")
    if not file.is_file():raise SystemExit("Missing finished shard: "+str(file))
    doc=json.loads(file.read_text(encoding="utf8"))
    if doc["bundles_processed"]!=end:
        raise SystemExit("Shard incomplete: "+suffix+" expected end "+str(end))
    if doc.get("shard_start_index",start)!=start or doc.get("shard_end_index",end)!=end:
        raise SystemExit("Shard split differs from authorized [start,end): "+suffix)
    if doc["counts"].get("BUNDLE_FAILED",0):
        raise SystemExit("A source bundle failed; captured entries preserved: "+suffix)
    for row in doc["clips"]:
        idx=row["bundle_index"]
        if idx<start or idx>=end:raise SystemExit("Cross-shard bundle index "+str(idx))
        if "curve_file" not in row:raise SystemExit("Source clip failed: "+str(row))
        key=(str(row["source_bundle"]),int(row["native_path_id"]))
        if key in seen:raise SystemExit("Original Unity path ID duplicated across shards")
        seen.add(key)
        if not pathlib.Path(row["curve_file"]).is_file():
            raise SystemExit("Missing native source curve file: "+row["curve_file"])
        source_paths[idx]=row["source_bundle"]
        report["clip_rows"].append(row)
assert report["clip_rows"],"No original native source curves imported"
# Unity bundles lacking AnimationClip are legal, so directory presence is checked
# against the actual indexed installed 288 files rather than inventing clips.
source_root=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party\abdata\h\anim")
originals=sorted(source_root.rglob("*.unity3d"),key=lambda p:str(p).lower())
if len(originals)!=288:raise SystemExit("Original installed source inventory changed")
for idx,path in enumerate(originals):
    if idx in source_paths and pathlib.Path(source_paths[idx])!=path:
        raise SystemExit("Source path differs from installed Unity bundle at index "+str(idx))
report["source_bundle_paths"]=[str(p) for p in originals]
report["actual_native_curve_clip_count"]=len(report["clip_rows"])
report["native_source_bundles"]=len(originals)
report["native_unreal_sequences_created"]=0
report["original_source_modified"]=False
report["status"]="ALL_INSTALLED_H_SOURCE_CURVES_CAPTURED_NOT_YET_RETARGETED"
temp=out.with_suffix(".tmp")
temp.write_text(json.dumps(report,ensure_ascii=False,indent=1)+"\n",encoding="utf8")
os.replace(temp,out)
print("GATE12_H_SOURCE_CAPTURE_DONE",report["native_source_bundles"],
      report["actual_native_curve_clip_count"],str(out),flush=True)
