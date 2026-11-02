"""Actual Unreal PrimaryAssetLabel for exactly 900 source-hash-verified
user-approved native interaction performances. NEVER cook 2301 QA-only
clips merely because they share the same source directory.
Fixes previous SoftObjectPath vs TSoftObjectPtr UObject Python type error.
"""
import unreal,pathlib,json,traceback,hashlib
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
data=json.loads((R/"Content/NoWorld/Animation/SharedMotionCatalog.json").read_text(encoding="utf8"))["motions"]
approved=[x for x in data if x.get("category")=="AdultInteraction" and x.get("approved_for_gameplay")
          and not x["motion_id"].startswith("kk.berserker.native.")]
assert len(approved)==900
paths=sorted({x["native_sequence"] for x in approved})
assert len(paths)==900
dest="/Game/NoWorld/Animation"
name="PAL_Gate12_UserApprovedOriginalNativeInteractions"
full=f"{dest}/{name}"
out=R/"Saved/Gate12Readiness/GATE12_EXACT_900_NATIVE_ASSET_COOK_LABEL_20261024.json"
receipt={"source_count":len(paths),"label_path":full,
         "method":"Unreal PrimaryAssetLabel ExplicitAssets Array[Object]",
         "status":"STARTED","original_native_assets_modified":False}
try:
    a=unreal.AssetToolsHelpers.get_asset_tools()
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        label=unreal.EditorAssetLibrary.load_asset(full)
        if not isinstance(label,unreal.PrimaryAssetLabel):
            raise RuntimeError("Existing asset at label path is not label")
    else:
        factory=unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class",unreal.PrimaryAssetLabel)
        label=a.create_asset(name,dest,unreal.PrimaryAssetLabel,factory)
        assert label,"PrimaryAssetLabel factory failed"
    first=unreal.load_asset(paths[0])
    assert isinstance(first,unreal.AnimSequence),"Native first source is not AnimSequence"
    # TSoftObjectPtr<UObject> accepts loaded UObject, NOT Python SoftObjectPath.
    label.set_editor_property("explicit_assets",[first])
    assert len(label.get_editor_property("explicit_assets"))==1
    all_assets=[first]
    for idx,path in enumerate(paths[1:],start=1):
        asset=unreal.load_asset(path)
        if not isinstance(asset,unreal.AnimSequence):
            raise RuntimeError(f"Native source missing at index {idx}: {path}")
        all_assets.append(asset)
        if idx%100==0:unreal.log(f"G12_ORIGINAL_NATIVE_EXPLICIT_COOK_LINKED {idx+1}/900")
    label.set_editor_property("explicit_assets",all_assets)
    assert len(label.get_editor_property("explicit_assets"))==900
    label.set_editor_property("is_runtime_label",True)
    rules=label.get_editor_property("rules")
    rules.set_editor_property("cook_rule",unreal.PrimaryAssetCookRule.ALWAYS_COOK)
    label.set_editor_property("rules",rules)
    assert unreal.EditorAssetLibrary.save_loaded_asset(label)
    assert unreal.EditorAssetLibrary.does_asset_exist(full)
    receipt["status"]="PASS_REAL_900_SOURCE_NATIVE_ASSET_LABEL_SAVED"
    receipt["actual_native_clips"]=len(label.get_editor_property("explicit_assets"))
    receipt["cook_rule"]="AlwaysCook"
except Exception:
    receipt["status"]="FAILED"
    receipt["error"]=traceback.format_exc()[-3900:]
finally:
    out.write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf8")
    unreal.log("G12_EXACT_NATIVE_LABEL "+receipt["status"])
