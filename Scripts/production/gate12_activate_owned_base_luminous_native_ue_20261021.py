"""Production: reuse verified 121-part Base 3-wardrobe native BP; register exact
approved Base version in live Ruler CharacterVisuals. No test map, no new
character meshes, no fictional owner/Manifestation, no cook/engine build.
"""
import json,traceback,pathlib,unreal
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
SOURCE="/Game/Experimental/LuminousValentineV3/RuntimeSwitchQA_20261011/BP_LuminousValentineV3_NativeSwitchQA"
TARGET="/Game/NoWorld/Characters/Luminous/Base/BP_Luminous_Base_Production"
CATALOG="/Game/NoWorld/UI/DA_NoWorld_CharacterVisuals"
RECEIPT=R/"Saved/Gate12Readiness/Gate12_LuminousBase_ProductionCatalog_World_20261021.json"
IDENTITY="character:luminous_valentine.identity"
VERSION="character:luminous_valentine.version_ln_base"
status={"scope":"NATIVE_REAL_121_PART_EXISTING_LUMINOUS_BASE_CHARACTER",
"status":"STARTED","source_blueprint":SOURCE,"production_blueprint":TARGET,
"catalog":CATALOG,"ownership_created":False,"source_meshes_modified":False,
"engine_compiled":False,"gameplay_visual_approval":False}
try:
    assert "OfflineGame_Gate12_Koikatsu_20261010" in str(unreal.Paths.get_project_file_path())
    src=unreal.load_asset(SOURCE)
    assert isinstance(src,unreal.Blueprint),"Existing genuine 121-part native source unavailable"
    source_class=unreal.EditorAssetLibrary.load_blueprint_class(SOURCE)
    assert source_class is not None
    old=unreal.get_default_object(source_class)
    wanted={"CLOTHED":45,"UNDERWEAR":39,"NUDE":37}
    fields={"CLOTHED":"clothed_native_parts","UNDERWEAR":"underwear_native_parts","NUDE":"nude_native_parts"}
    for label,field in fields.items():
        parts=old.get_editor_property(field)
        assert len(parts)==wanted[label],(label,len(parts))
        status[label+"_native_parts"]=len(parts)
    if unreal.EditorAssetLibrary.does_asset_exist(TARGET):
        bp=unreal.load_asset(TARGET)
        assert isinstance(bp,unreal.Blueprint),"Existing production asset has wrong type"
        status["created"]=False
    else:
        unreal.EditorAssetLibrary.make_directory(
            "/Game/NoWorld/Characters/Luminous/Base")
        bp=unreal.EditorAssetLibrary.duplicate_asset(SOURCE,TARGET)
        assert bp is not None,"Cannot adopt full existing real source BP"
        status["created"]=True
    cl=unreal.EditorAssetLibrary.load_blueprint_class(TARGET)
    assert cl,"Duplicated source has no native class"
    cdo=unreal.get_default_object(cl)
    parent=unreal.load_class(None,"/Script/OfflineGame.OGLuminousV3PlayableCharacter")
    status["production_native_parent"]=str(parent.get_name())
    for label,field in fields.items():
        assert len(cdo.get_editor_property(field))==wanted[label]
    cdo.set_editor_property("canonical_luminous_identity_id",IDENTITY)
    cdo.set_editor_property("canonical_luminous_version_id",VERSION)
    for prop in ("run_non_persistent_wardrobe_qa_cycle",
                 "run_native_motion_qa_cycle",
                 "enable_native_wardrobe_qa_touch_panel",
                 "enter_world_mode_on_isolated_qa_startup"):
        cdo.set_editor_property(prop,False)
    assert unreal.EditorAssetLibrary.save_loaded_asset(bp),"Failed saving adopted production BP"
    status["real_existing_part_counts_verified"]=wanted
    status["production_exact_identity"]=IDENTITY
    status["production_exact_version"]=VERSION
    status["qa_controls_disabled"]=True
    asset=unreal.load_asset(CATALOG)
    assert asset is not None,"Game UI native character visual catalog absent"
    entries=list(asset.get_editor_property("entries"))
    exact=[]
    for i,e in enumerate(entries):
        if e.get_editor_property("identity_id").get_editor_property("value")==IDENTITY and e.get_editor_property("version_id").get_editor_property("value")==VERSION:
            exact.append(i)
    assert len(exact)<=1,"Ambiguous duplicate production Luminous version"
    entry=entries[exact[0]] if exact else unreal.OGGate12CharacterVisualEntry()
    if not exact:
        entry.set_editor_property("identity_id",unreal.OGContentId(value=IDENTITY))
        entry.set_editor_property("version_id",unreal.OGContentId(value=VERSION))
    entry.set_editor_property("wardrobe_state_json_field","luminous_v3_wardrobe")
    entry.set_editor_property("approved_wardrobe_choices",
        ["CLOTHED","UNDERWEAR","NUDE"])
    entry.set_editor_property("approved_world_playable_class",cl)
    entry.set_editor_property("genuine_character_preview_class",cl)
    if not exact:entries.append(entry)
    asset.set_editor_property("entries",entries)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
    status["catalog_entry_count"]=len(entries)
    status["catalog_base_entry_index"]=exact[0] if exact else len(entries)-1
    status["status"]="REGISTERED_EXISTING_SOURCE_AS_PRODUCTION_BASE_WORLD_PAWN"
except Exception:
    status["status"]="BLOCKED_PRECISE_FAILURE"
    status["error"]=traceback.format_exc()[-4400:]
finally:
    RECEIPT.parent.mkdir(parents=True,exist_ok=True)
    RECEIPT.write_text(json.dumps(status,indent=2)+"\n",encoding="utf8")
    unreal.log("GATE12_BASE_NATIVE_PRODUCTION="+status["status"])
