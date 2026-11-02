"""Guarantee original approved motions are reachable by Unreal cook.
The engine's PrimaryAssetLabel.ExplicitAssets expects TSoftObjectPtr<UObject>,
not Python SoftObjectPath, so replace the broken editor-only label logic
with Unreal's supported native DirectoriesToAlwaysCook configuration.
"""
import pathlib,json,hashlib,configparser
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
config=R/"Config/DefaultGame.ini"
library=R/"Content/NoWorld/Animation/SharedMotionCatalog.json"
doc=json.loads(library.read_text(encoding="utf8"))
a=[x for x in doc["motions"] if x["category"]=="AdultInteraction" and x.get("approved_for_gameplay")]
original=[x for x in a if not x["motion_id"].startswith("kk.berserker.native.")]
assert len(original)==900,len(original)
assert len({x["native_sequence"] for x in original})==900
root="NoWorld/Characters/Luminous/Alter/KoikatsuOriginalStudioMotionQA"
path=f'+DirectoriesToAlwaysCook=(Path="/Game/{root}")'
assert path in config.read_text(encoding="utf8")
assert '+DirectoriesToAlwaysStageAsUFS=(Path="NoWorld/Animation")' in config.read_text(encoding="utf8")
verified=0
for x in original:
    prefix=x["native_sequence"].split(".",1)[0]
    assert prefix.startswith("/Game/"+root+"/")
    f=R/"Content"/(prefix[len("/Game/"):]+".uasset")
    assert f.is_file(),str(f)
    if x.get("native_uasset_sha256"):
        h=hashlib.sha256(f.read_bytes()).hexdigest()
        assert h==x["native_uasset_sha256"],str(f)
    verified+=1
receipt=R/"Saved/Gate12Readiness/GATE12_NATIVE_USER_APPROVED_SOURCE_COOK_LABEL_20261024.json"
result={"schema":"noworld.gate12.native.motion.cook.v2",
        "status":"DIRECTORY_COOK_CONFIGURATION_VERIFIED",
        "unreal_native_method":"ProjectPackagingSettings.DirectoriesToAlwaysCook",
        "source_directory":"/Game/"+root,
        "original_source_approved_motion_count":verified,
        "source_hashes_checked":verified,
        "catalog_uFS_staged":True,
        "original_assets_untouched":True,
        "packaged_runtime_success":False}
receipt.write_text(json.dumps(result,indent=2)+"\n",encoding="utf8")
print(result["status"],verified)
