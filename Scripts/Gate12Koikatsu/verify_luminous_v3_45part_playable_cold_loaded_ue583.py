"""Gate12 cold-load verifies saved native Luminous 45-part blueprint plus existing GameMode/map.
Read-only, no mobile or live gameplay proof, no source/binary modification.
"""
import unreal,pathlib,json,time,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu\LuminousV3")
receipt=json.loads((R/"CLOTHED/luminous_playable_45part_native_blueprint_ue583.json").read_text("utf-8-sig"))
out=R/"CLOTHED/luminous_playable_45part_cold_native_verification_ue583.json"
q={"status":"START","same_native_character_architecture":False,"native_parts_seen":0,"map_class_binding":False,
 "movement_gameplay":"NOT_TESTED","wardrobe_switch":"NOT_TESTED","motion_animation":"NOT_TESTED",
 "android":"NOT_TESTED","gate11_artifact_ready":False,"warnings":[]}
def save():out.write_text(json.dumps(q,ensure_ascii=False,indent=2,default=str)+"\n",encoding="utf8")
save()
try:
 if receipt["status"]!="PASS_NATIVE_BLUEPRINT_45_PARTS_ASSEMBLED_PENDING_LIVE_VISUAL_ANIM_ANDROID":raise RuntimeError("Native BP author receipt not PASS")
 expected={x["component"]:x for x in receipt["parts"]}
 bp=unreal.load_asset(receipt["player"])
 gm=unreal.load_asset(receipt["game_mode"])
 if not isinstance(bp,unreal.Blueprint) or not isinstance(gm,unreal.Blueprint):raise RuntimeError("Saved native Blueprint missing")
 bpcls=unreal.EditorAssetLibrary.load_blueprint_class(receipt["player"])
 gmcls=unreal.EditorAssetLibrary.load_blueprint_class(receipt["game_mode"])
 if not bpcls or not gmcls:raise RuntimeError("Cold loaded class missing")
 parent=unreal.load_class(None,"/Script/OfflineGame.OGWorldPrototypeCharacter")
 native_mode=unreal.load_class(None,"/Script/OfflineGame.OGWorldPresentationGameMode")
 if not parent or not native_mode:raise RuntimeError("Existing game runtime classes absent")
 cdo=unreal.get_default_object(gmcls)
 if cdo.get_editor_property("default_pawn_class")!=bpcls:raise RuntimeError("GameMode not selecting Luminous character")
 q["game_mode_default_pawn_class"]=str(cdo.get_editor_property("default_pawn_class"))
 subsys=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
 handles=subsys.k2_gather_subobject_data_for_blueprint(bp)
 if not handles:raise RuntimeError("Compiled blueprint has no object handles")
 lib=unreal.SubobjectDataBlueprintFunctionLibrary
 found={}
 for h in handles:
  data=subsys.k2_find_subobject_data_from_handle(h)
  x=lib.get_object(data)
  if x and isinstance(x,unreal.SkeletalMeshComponent) and x.get_name().startswith("LV3_C_M"):
   label=x.get_name().removesuffix("_GEN_VARIABLE")
   if label in found:raise RuntimeError("Duplicate generated Blueprint component "+label)
   found[label]=x
 if len(found)!=45:raise RuntimeError("Only %d/45 compiled native skeletal components found"%(len(found)))
 for label,x in found.items():
  if label not in expected:raise RuntimeError("Unrecognized component: "+label)
  mesh=x.get_skeletal_mesh_asset()
  if not isinstance(mesh,unreal.SkeletalMesh) or mesh.get_path_name()!=expected[label]["path"]:
   raise RuntimeError("Wrong native imported mesh on "+label)
  q["native_parts_seen"]+=1
 q["same_native_character_architecture"]=True
 world=unreal.EditorLoadingAndSavingUtils.load_map(receipt["world"])
 if not world:raise RuntimeError("Saved isolated level failed cold load")
 settings=world.get_world_settings()
 if settings.get_editor_property("default_game_mode")!=gmcls:
  raise RuntimeError("Saved test world does not use Luminous native GameMode")
 q["map_class_binding"]=True
 q["world"]=receipt["world"]
 q["player"]=receipt["player"]
 q["game_mode"]=receipt["game_mode"]
 q["status"]="PASS_COLD_NATIVE_45PART_CHARACTER_GAMEMODE_AND_WORLD_PENDING_PIE"
except Exception as err:
 q["status"]="FAIL_COLD_NATIVE_45PART_CHARACTER_MAP"
 q["exception"]=str(err)
 q["trace"]=traceback.format_exc()[-5000:]
finally:
 q["updated_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime());save()
 unreal.log("G12_LUMINOUS_COLD_PLAYABLE="+q["status"])
