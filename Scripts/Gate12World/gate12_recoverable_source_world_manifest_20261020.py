from pathlib import Path
import json,hashlib,datetime
P=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
A=P/"Content/NoWorld/FirstWorld"
E=P/"Saved/Gate12Readiness/FirstWorldR1_MergedSource"
Q=Path(r"D:\NoWorld_Art_Production\NoWorld_FirstWorld_RegionKit_R1_20261019")
R={"schema":"noworld.gate12.source_candidate_firstworld_noncanonical_checkpoint.v1",
   "status":"SOURCE_CANDIDATE_NOT_ART_OR_ANDROID_APPROVED",
   "project":str(P),
   "engine":"Unreal Engine 5.8.3",
   "source_gate02":str(Q),
   "protected_default_world":"/Game/Maps/StartingWorld",
   "source_map":"/Game/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_SourceAssembly",
   "source_ecology_seed":1412,
   "source_scatter_count":10454,
   "files":[],"approvals":{"gate00_art":False,"android_quality":False,"playability":False}}
for f in sorted(A.rglob("*")):
 if f.is_file():
  R["files"].append({"relative_path":f.relative_to(P).as_posix(),
                     "bytes":f.stat().st_size,
                     "sha256":hashlib.sha256(f.read_bytes()).hexdigest()})
R["total_files"]=len(R["files"])
R["total_bytes"]=sum(x["bytes"] for x in R["files"])
for name in ["royal_citadel_direct_source_smoothing_receipt.json",
             "royal_citadel_smoothed_native_import_receipt.json",
             "citadel_corrected_source_anchor_placement_receipt.json",
             "source_anchor_placement_receipt.json",
             "first_world_source_level_receipt.json"]:
 f=E/name
 if not f.exists():raise RuntimeError("Required evidence missing "+str(f))
 R[name]=json.loads(f.read_text(encoding="utf-8"))
assert R["royal_citadel_smoothed_native_import_receipt.json"]["errors"]==[]
assert R["citadel_corrected_source_anchor_placement_receipt.json"]["errors"]==[]
assert R["citadel_corrected_source_anchor_placement_receipt.json"]["status"].startswith("CITADEL_CORRECTED")
assert R["royal_citadel_smoothed_native_import_receipt.json"]["actual_ue_lod0_triangles"]>1_200_000
assert (P/"Config/DefaultEngine.ini").read_text(encoding="utf-8-sig").find("GameDefaultMap=/Game/Maps/StartingWorld")>=0
assert len(R["files"])>=205
QF=Q/"anchors/royal_citadel.fbx"
assert hashlib.sha256(QF.read_bytes()).hexdigest()=="d866a6149af19f0eaa003f2c5ad097f696ee9792d67af89a2ead2d4d6114385a"
R["untouched_original_citadel_sha256"]=hashlib.sha256(QF.read_bytes()).hexdigest()
out=E/"Gate12_FirstWorld_R1_SOURCE_CHECKPOINT_20261020.json"
out.write_text(json.dumps(R,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
print("WORLD_R1_RECOVERABLE_CONTENT_MANIFEST="+str(out))
print("MAP_AND_ASSETS_FILE_COUNT="+str(R["total_files"]))
print("MAP_AND_ASSETS_MIB="+str(round(R["total_bytes"]/1048576,2)))
print("ORIGINAL_CITADEL_SOURCE_SHA256="+R["untouched_original_citadel_sha256"])
print("REPAIRED_CITADEL_NATIVE_TRIANGLES="+str(R["royal_citadel_smoothed_native_import_receipt.json"]["actual_ue_lod0_triangles"]))
print("NONCANONICAL_CANDIDATE_ONLY_NO_DEFAULT_MAP_SWITCH")
