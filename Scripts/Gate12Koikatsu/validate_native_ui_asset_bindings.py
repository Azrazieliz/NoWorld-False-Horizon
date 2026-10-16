"""Headless editor verification for native Widget Blueprint and canonical catalog.
No cook, package install, synthetic owner, or content-acceptance inference.
"""
import json
import os
import unreal

ROOT="/Game/NoWorld/UI/"
out=unreal.Paths.project_saved_dir()+"Gate12Readiness/native_ui_asset_validation.json"
receipt={"schema":"GATE12_NATIVE_UI_ASSET_VALIDATION_V1","cook":False,
         "valid":False,"checks":{},"errors":[]}
try:
    native=unreal.load_class(None,"/Script/OfflineGame.OGGate12NativeRulerWidget")
    receipt["checks"]["native_ruler_class_loaded"]=bool(native)
    generated=unreal.load_class(None,
        ROOT+"WBP_NoWorld_RulerShell.WBP_NoWorld_RulerShell_C")
    receipt["checks"]["widget_blueprint_generated_class_loaded"]=bool(generated)
    catalog=unreal.EditorAssetLibrary.load_asset(
        ROOT+"DA_NoWorld_CharacterVisuals")
    receipt["checks"]["catalog_exists"]=bool(catalog)
    if catalog:
        receipt["checks"]["catalog_is_correct_native_class"]=(
            catalog.get_class().get_name()=="OGGate12CharacterVisualCatalog")
        receipt["checks"]["catalog_entry_count"]=len(catalog.get_editor_property("Entries"))
        if len(catalog.get_editor_property("Entries")) != 0:
            receipt["errors"].append("An authored catalog requires Gate11 provenance review.")
    receipt["valid"]=all([
        receipt["checks"].get("native_ruler_class_loaded"),
        receipt["checks"].get("widget_blueprint_generated_class_loaded"),
        receipt["checks"].get("catalog_is_correct_native_class")
    ]) and not receipt["errors"]
except Exception as exc:
    receipt["errors"].append(str(exc))
finally:
    os.makedirs(os.path.dirname(out),exist_ok=True)
    with open(out,"w",encoding="utf-8") as f:
        json.dump(receipt,f,indent=2)
    unreal.log("GATE12_ASSET_VALIDATE "+json.dumps(receipt))
