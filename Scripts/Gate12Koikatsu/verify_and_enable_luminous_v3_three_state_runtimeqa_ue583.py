"""Cold-load native 121-soft-reference wardrobe class and activate isolated 3-step QA cycle.
Edits ONLY dedicated RuntimeSwitchQA Luminous BP, never Foundation or any source asset.
"""
import unreal,json,pathlib,traceback,time
W=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
Q=W/"Saved/Gate12Koikatsu/LuminousV3"
src=json.loads((Q/"native_121_wardrobe_runtime_switch_ue583.json").read_text("utf-8-sig"))
out=Q/"luminous_wardrobe_121_soft_native_cold_ue583.json"
report={"status":"START","validation":"BLUEPRINT_CDO_AND_GAMEMODE_NATIVE_COLD_LOAD",
        "runtime_actor_execution":"NOT_TESTED","android":"NOT_TESTED",
        "gate11_artifact_ready":False,"wardrobes":{}}
def save():out.write_text(json.dumps(report,indent=2,default=str)+"\n",encoding="utf8")
save()
try:
 if src["status"]!="PASS_NATIVE_THREE_WARDROBE_BLUEPRINT_AUTHORED_PENDING_COLD_AND_RUNTIME":
  raise RuntimeError("No pass authoring receipt")
 bp=unreal.load_asset(src["character"])
 gm=unreal.load_asset(src["game_mode"])
 if not isinstance(bp,unreal.Blueprint) or not isinstance(gm,unreal.Blueprint):raise RuntimeError("Missing native BP assets")
 c=unreal.EditorAssetLibrary.load_blueprint_class(src["character"])
 g=unreal.EditorAssetLibrary.load_blueprint_class(src["game_mode"])
 if not c or not g:raise RuntimeError("Native classes did not survive cold restart")
 native=unreal.load_class(None,"/Script/OfflineGame.OGLuminousV3PlayableCharacter")
 if not native:raise RuntimeError("C++ class not present after native build")
 d=unreal.get_default_object(c)
 for label,prop,count in [("CLOTHED","clothed_native_parts",45),
                          ("UNDERWEAR","underwear_native_parts",39),("NUDE","nude_native_parts",37)]:
  current=d.get_editor_property(prop)
  expected=src["parts"][label]["source_paths"]
  if len(current)!=count:raise RuntimeError("Lost soft references "+label)
  urls=[x.export_text() for x in current]
  if urls!=expected:
   raise RuntimeError("Native wardrobe soft path order drift "+label+" example="+str(urls[:1]))
  report["wardrobes"][label]={"native_soft_references":len(current),"expected":count,"exact_order_sha_receipt":True}
 if d.get_editor_property("canonical_luminous_identity_id") or d.get_editor_property("canonical_luminous_version_id"):
  raise RuntimeError("Canonical Luminous identity prematurely assigned; keep fail-closed")
 if unreal.get_default_object(g).get_editor_property("default_pawn_class")!=c:
  raise RuntimeError("WorldMode default pawn not native Luminous")
 world=unreal.EditorLoadingAndSavingUtils.load_map(src["world"])
 if not world or world.get_world_settings().get_editor_property("default_game_mode")!=g:
  raise RuntimeError("Native test world default GM drift")
 d.set_editor_property("run_non_persistent_wardrobe_qa_cycle",True)
 if not unreal.BlueprintEditorLibrary.compile_blueprint(bp):raise RuntimeError("Native QA Blueprint compile failure")
 if not unreal.EditorAssetLibrary.save_loaded_asset(bp):raise RuntimeError("Cannot save isolated runtime QA opt-in")
 if not d.get_editor_property("run_non_persistent_wardrobe_qa_cycle"):
  raise RuntimeError("QA auto-cycle opt-in didn't stick")
 report["qa_runtime_cycle_opt_in"]=True
 report["full_character_components_runtime_pool_max"]=45
 report["canonical_save_safety"]="BLOCKED_UNTIL_GATE11_IDENTITY_ASSIGNED"
 report["status"]="PASS_COLD_FULL_121_SOFT_REFS_QA_RUNTIME_CYCLE_AUTHORED_PENDING_ACTUAL_GAME"
except Exception as e:
 report["status"]="FAIL_NATIVE_COLD_AND_QA_RUNTIME"
 report["exception"]=str(e);report["trace"]=traceback.format_exc()[-4000:]
finally:
 report["updated_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime());save()
 unreal.log("GATE12_LUMINOUS_COLD_WARDROBE="+report["status"])
