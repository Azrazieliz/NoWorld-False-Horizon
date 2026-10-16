"""Create editable native UMG shell and version-exact visual catalog.
Editor-only import/asset authoring. NO COOK, NO APK, NO ownership mutation.
Run after successful UE C++ build with UnrealEditor-Cmd -run=pythonscript.
"""
import json
import os
import unreal

ROOT="/Game/NoWorld/UI"
RECEIPT=unreal.Paths.project_saved_dir()+"Gate12Readiness/native_umg_authoring.json"
status={"schema":"GATE12_NATIVE_UMG_AUTHORING_V1","cook":False,
        "assets":[],"errors":[]}
tools=unreal.AssetToolsHelpers.get_asset_tools()

def native(name):
    result=unreal.load_class(None,"/Script/OfflineGame."+name)
    if not result:
        raise RuntimeError("Native C++ reflection class unavailable: "+name)
    return result

def create(name, cls, factory, path=ROOT):
    target=path+"/"+name
    if unreal.EditorAssetLibrary.does_asset_exist(target):
        asset=unreal.EditorAssetLibrary.load_asset(target)
        if not asset: raise RuntimeError("Existing asset could not load: "+target)
        status["assets"].append({"path":target,"created":False,
            "existing_class":asset.get_class().get_name()})
        return asset
    asset=tools.create_asset(name,path,cls,factory)
    if not asset: raise RuntimeError("Creation returned null: "+target)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError("Asset could not be saved: "+target)
    status["assets"].append({"path":target,"created":True,
        "class":asset.get_class().get_name()})
    return asset

try:
    unreal.EditorAssetLibrary.make_directory(ROOT)
    catalog_class=native("OGGate12CharacterVisualCatalog")
    catalog_factory=unreal.DataAssetFactory()
    catalog_factory.set_editor_property("DataAssetClass",catalog_class)
    create("DA_NoWorld_CharacterVisuals",catalog_class,catalog_factory)

    ruler_class=native("OGGate12NativeRulerWidget")
    widget_factory=unreal.WidgetBlueprintFactory()
    widget_factory.set_editor_property("ParentClass",ruler_class)
    create("WBP_NoWorld_RulerShell",unreal.WidgetBlueprint,widget_factory)

    # Source-owned content remains unpopulated pending Gate11 approvals.
    catalog=unreal.EditorAssetLibrary.load_asset(
        ROOT+"/DA_NoWorld_CharacterVisuals")
    if len(catalog.get_editor_property("Entries")):
        status["errors"].append("Catalog was already populated; audit its owner IDs.")
    status["ready_for_approval"]=not status["errors"]
except Exception as exc:
    status["errors"].append(str(exc))
    status["ready_for_approval"]=False
finally:
    os.makedirs(os.path.dirname(RECEIPT),exist_ok=True)
    with open(RECEIPT,"w",encoding="utf-8") as stream:
        json.dump(status,stream,indent=2)
    unreal.log("GATE12_NATIVE_UMG_AUTHORING: "+json.dumps(status))
