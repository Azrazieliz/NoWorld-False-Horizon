"""Source-authentic exact-H-bundle native-rig candidate decoding.
Keeps original bundle hash / clip path_id; no participant choreography
or age/consent authorization inferred from source names. QA-only.
Uses established 20261022 Unity streamed native joint decoder.
"""
import json,gzip,pathlib,sys,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
P=R/"Scripts/production"
sys.path.insert(0,str(P))
import gate12_decode_genuine_koikatsu_studio_to_native_joint_curves_20261022 as core
source=R/"SourceAssets/Gate12/Animations20261021/KoikatsuHOriginalSourceCurves/GATE12_INSTALLED_H_ANIMATION_SOURCE_CURVE_INDEX.json"
idx=json.loads(source.read_text(encoding="utf8"))
byname={}
for row in idx["clips"]:
 if row.get("bundle_index")==0 and row.get("curve_file"):
  byname.setdefault(row["clip_name"],[]).append(row)
target=("M_M_Idle","M_K_Loop")
rest=core.source_bone_catalog()
for name in target:
 matches=byname.get(name,[])
 if not matches:
  print("H_QA_DECODE_SOURCE_MISSING",name,flush=True)
  continue
 # A bundle contains many distinct same-name native objects. Choose an exact
 # path ID for an isolated QA conversion; never claim this is a scene pair.
 row=min(matches,key=lambda x:int(x["native_path_id"]))
 print("H_QA_SELECTED_EXACT_OBJECT",name,"path_id",row["native_path_id"],"same_name_sources",len(matches),flush=True)
 raw=pathlib.Path(row["curve_file"])
 with gzip.open(raw,"rt",encoding="utf8") as f:datum=json.load(f)
 if (datum["original_unity_path_id"]!=row["native_path_id"] or
     datum["name"]!=name or datum.get("adult_or_paired_context_authorized") is True):
  raise RuntimeError("H source identity or eligibility unexpectedly changed")
 core.INDEX={"source_files":[{
  "source_clip_name":name,"source_bundle_index":0,
  "raw_curve_path":str(raw),
  "source_hash":datum["source_bundle_data_sha256"]}]}
 try:
  core.convert(name,rest)
 except Exception as e:
  print("H_QA_SOURCE_NOT_NATIVE_READY",name,repr(e),traceback.format_exc()[-600:],flush=True)
print("H_QA_EXACT_SOURCE_ATTEMPTED",len(target),flush=True)
