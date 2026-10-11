"""Gate12 true Luminous-specific Android entry: ONE character, existing World Mode,
real nine Motion V3 clips, native touch wardrobe QA panel; no automatic QA cycles.
Original RuntimeSwitchQA source Blueprints/map unchanged. Gate11 ARTIFACT_READY not assumed.
"""
import unreal,pathlib,json,time,traceback
W=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
Q=W/"Saved/Gate12Koikatsu/LuminousV3"
SOURCE=json.loads((Q/"native_121_wardrobe_runtime_switch_ue583.json").read_text("utf-8-sig"))
MOTION=json.loads((Q/"luminous_native_three_wardrobe_motion_v3_bound_ue583.json").read_text("utf-8-sig"))
ROOT="/Game/Experimental/LuminousValentineV3/AndroidReady_20261011"
DEST={"pawn":ROOT+"/BP_LuminousV3_IsolatedAndroid",
    "game_mode":ROOT+"/BP_LuminousV3_IsolatedAndroidGameMode",
    "map":ROOT+"/Maps/L_LuminousV3_IsolatedAndroid"}
OUT=Q/"luminous_isolated_android_entry_author_receipt_ue583.json"
r={"status":"START","android_application_id":"com.azrazieliz.gate12luminousv3",
"normal_app_untouched":"com.azrazieliz.OfflineGame","canonical_persistence":"GATED_GATE11_NO_REGISTRY_ID",
"gate11_artifact_ready":False,"assets":DEST}
def save():OUT.write_text(json.dumps(r,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
save()
try:
 if SOURCE["status"]!="PASS_NATIVE_THREE_WARDROBE_BLUEPRINT_AUTHORED_PENDING_COLD_AND_RUNTIME":raise RuntimeError("No native Luminous source blueprint")
 if MOTION["status"]!="PASS_9_REAL_975_CMUMOCAP_CLIPS_BOUND_TO_NATIVE_RUNTIME_QA_NOT_VISUALLY_APPROVED":
  raise RuntimeError("Native nine Motion V3 clips not verified")
 if any(unreal.EditorAssetLibrary.does_asset_exist(x) for x in DEST.values()):
  raise RuntimeError("Refusing to overwrite an existing signed Android entry asset")
 if not unreal.EditorAssetLibrary.duplicate_asset(SOURCE["character"],DEST["pawn"]):
  raise RuntimeError("Android native character copy failed")
 bp=unreal.load_asset(DEST["pawn"])
 pawn=unreal.EditorAssetLibrary.load_blueprint_class(DEST["pawn"])
 if not isinstance(bp,unreal.Blueprint) or not pawn:raise RuntimeError("Android character class invalid")
 default=unreal.get_default_object(pawn)
 default.set_editor_property("run_non_persistent_wardrobe_qa_cycle",False)
 default.set_editor_property("run_native_motion_qa_cycle",False)
 default.set_editor_property("enable_native_wardrobe_qa_touch_panel",True)
 default.set_editor_property("enter_world_mode_on_isolated_qa_startup",True)
 for state,key,total in [("CLOTHED","clothed_native_parts",45),
                         ("UNDERWEAR","underwear_native_parts",39),
                         ("NUDE","nude_native_parts",37)]:
  if len(default.get_editor_property(key))!=total:raise RuntimeError("Android full mesh references lost "+state)
 for action,key in [("Walk","native_walk_by_wardrobe"),("Run","native_run_by_wardrobe"),
                    ("Jump","native_jump_by_wardrobe")]:
  if len(default.get_editor_property(key))!=3:raise RuntimeError("Missing genuine native mocap for "+action)
 if default.get_editor_property("canonical_luminous_identity_id") or default.get_editor_property("canonical_luminous_version_id"):
  raise RuntimeError("Do not invent canonical Gate11 identity")
 if not unreal.BlueprintEditorLibrary.compile_blueprint(bp):raise RuntimeError("Android character Blueprint compile failed")
 if not unreal.EditorAssetLibrary.save_loaded_asset(bp):raise RuntimeError("Android character Blueprint save failed")
 r["pawn"]="PASS_NATIVE_FULL_LUMINOUS_ANDROID_TEST_ENTRY";save()
 if not unreal.EditorAssetLibrary.duplicate_asset(SOURCE["game_mode"],DEST["game_mode"]):
  raise RuntimeError("Android GameMode copy failed")
 gm_bp=unreal.load_asset(DEST["game_mode"])
 gm=unreal.EditorAssetLibrary.load_blueprint_class(DEST["game_mode"])
 gm_default=unreal.get_default_object(gm)
 gm_default.set_editor_property("default_pawn_class",pawn)
 if not unreal.BlueprintEditorLibrary.compile_blueprint(gm_bp):raise RuntimeError("Android GameMode compile failed")
 if not unreal.EditorAssetLibrary.save_loaded_asset(gm_bp):raise RuntimeError("Android GameMode save failed")
 r["game_mode"]="PASS_NATIVE_INHERITED_WORLD_MODE";save()
 if not unreal.EditorAssetLibrary.duplicate_asset(SOURCE["world"],DEST["map"]):
  raise RuntimeError("Android Luminous map copy failed")
 world=unreal.EditorLoadingAndSavingUtils.load_map(DEST["map"])
 if not world:raise RuntimeError("Native isolated Android map cannot load")
 world.get_world_settings().set_editor_property("default_game_mode",gm)
 if not unreal.EditorLoadingAndSavingUtils.save_map(world,DEST["map"]):
  raise RuntimeError("Native Android map save failed")
 r["native_touch_wardrobe_panel_enabled"]=True
 r["auto_wardrobe_cycle_enabled"]=False
 r["auto_motion_cycle_enabled"]=False
 r["runtime_world_mode_entry_enabled"]=True
 r["native_assets_clothed"]=45;r["native_assets_underwear"]=39;r["native_assets_nude"]=37
 r["native_mocap_clips"]=9
 r["status"]="PASS_ISOLATED_ANDROID_LUMINOUS_NATIVE_ENTRY_AUTHORED_PRE_COLD_TEST"
except Exception as e:
 r["status"]="FAIL_ISOLATED_ANDROID_ENTRY"
 r["exception"]=str(e)
 r["trace"]=traceback.format_exc()[-6000:]
finally:
 r["updated_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())
 save()
 unreal.log("GATE12_LUMINOUS_ANDROID_ENTRY="+r["status"])
