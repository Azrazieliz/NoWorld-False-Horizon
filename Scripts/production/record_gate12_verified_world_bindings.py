"""Only evidence-backed partial progress, preserving all 111 master tasks unchanged."""
import json,pathlib,subprocess
r=pathlib.Path(__file__).resolve().parents[2]
p=r/"docs/production/ADVANCEMENT_PROGRESS.json"
j=json.loads(subprocess.check_output(["git","-C",str(r),"show",
    "HEAD:docs/production/ADVANCEMENT_PROGRESS.json"]).decode("utf8"))
world=next(x for x in j["tasks"] if x["id"]=="G12-0504")
assert not world["master_checked"] and world["state"] in ("OPEN","SOURCE_PARTIAL")
world["state"]="SOURCE_PARTIAL"
world["source_paths"]=[
 "Source/OfflineGame/Private/Koikatsu/OGExactOriginalLuminousParts.cpp",
 "Source/OfflineGame/Private/Koikatsu/OGLuminousExactOriginalPreviewActor.cpp",
 "Source/OfflineGame/Private/Koikatsu/OGLuminousV3PlayableCharacter.cpp",
 "Source/OfflineGame/Private/UI/OGGate12CharacterVisualCatalog.cpp",
 "docs/production/gate12/evidence/verified-original-koikatsu-native/G12_Awakened_Berserker_ExactOriginal_NativeUEImportAccepted.json"]
world["remaining"]="Native compile, input and actual 3D viewport acceptance, gameplay, combat, source animation rig derivatives, Grand model, Android build and device test."
cache=next(x for x in j["tasks"] if x["id"]=="G12-0004")
assert not cache["master_checked"] and cache["state"] in ("OPEN","SOURCE_PARTIAL")
cache["state"]="SOURCE_PARTIAL"
cache["source_paths"]=["Scripts/production/relocate_unreal_zen_cache_lossless_20261031.ps1"]
cache["remaining"]="Byte-preserving Zen data relocation to D: completed and junction checked; independently verify an actual full cook can write cache without HTTP 507, before checking this master task."
j["summary"]["source_partial"]=sum(x["state"]=="SOURCE_PARTIAL" for x in j["tasks"])
assert len(j["tasks"])==j["summary"]["master_task_count"]==111
assert j["summary"]["master_checked"]==sum(t["master_checked"] for t in j["tasks"])
assert j["summary"]["master_open"]==sum(not t["master_checked"] for t in j["tasks"])
assert not any(j["summary"][x] for x in ["build_verified","device_verified","user_accepted"])
p.write_text(json.dumps(j,indent=2,ensure_ascii=False)+"\n","utf8")
print("G12_111_CANONICAL_TASKS_NO_FALSE_ACCEPTANCE",j["summary"])
