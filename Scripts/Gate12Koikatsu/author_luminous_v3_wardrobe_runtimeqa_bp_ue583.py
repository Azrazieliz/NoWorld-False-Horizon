"""Author real full-body 3-wardrobe Unreal pawn, not a static rendering.
Stores all 121 original full-resolution imported native skeletal-mesh paths as soft
cooking references. Only the ACTIVE wardrobe is instantiated by the C++ pawn.
No existing QA Blueprints or assets are overwritten. No Gate11 content authority.
"""
import unreal,pathlib,json,traceback,time
W=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
Q=W/"Saved/Gate12Koikatsu/LuminousV3"
ROOT="/Game/Experimental/LuminousValentineV3/RuntimeSwitchQA_20261011"
CHAR=ROOT+"/BP_LuminousValentineV3_NativeSwitchQA"
GAMEMODE=ROOT+"/BP_LuminousV3_WorldSwitchQA"
WORLD=ROOT+"/Maps/L_LuminousV3_RuntimeSwitchQA"
SRCWORLD="/Game/Maps/StartingWorld"
R=Q/"native_121_wardrobe_runtime_switch_ue583.json"
state={"status":"START","character":CHAR,"game_mode":GAMEMODE,"world":WORLD,
       "canonical_identity_authority":"NOT_REGISTERED_GATE11","canonical_persistence":"NOT_TESTED",
       "animation_source_authority":"NOT_TESTED","live_runtime":"NOT_TESTED","android":"NOT_TESTED",
       "gate11_artifact_ready":False,"parts":{}}
def save():R.write_text(json.dumps(state,ensure_ascii=False,indent=2,default=str)+"\n",encoding="utf8")
save()
try:
 if "OfflineGame_Gate12_Koikatsu_20261010" not in str(unreal.Paths.get_project_file_path()):raise RuntimeError("Wrong native UE worktree")
 counts={"CLOTHED":45,"UNDERWEAR":39,"NUDE":37}
 assets={}
 for label,total in counts.items():
  d=Q/label
  source=json.loads((d/"source_shards_actual_nonzero_morph_inventory.json").read_text("utf-8-sig"))
  source_by_index={int(x["index"]):x for x in source["nonzero_by_shard"]}
  found={}
  for f in d.glob("native_shards_*_ue583.json"):
   journal=json.loads(f.read_text("utf-8-sig"))
   for p in journal["native_parts"]:
    i=int(p["index"])
    if i in found:raise RuntimeError("Duplicate source index "+label+"/"+str(i))
    meshes=[x["path"] for x in p["assets"] if x["class"]=="SkeletalMesh"]
    if len(meshes)!=1:raise RuntimeError("Invalid skeletal source "+label+"/"+str(i))
    if p["sha256"]!=source_by_index[i]["source_sha256"]:raise RuntimeError("Source SHA mismatch "+label+"/"+str(i))
    if p.get("ue_skeletal_mesh_count")!=1:raise RuntimeError("Unbuilt native component "+label+"/"+str(i))
    found[i]=meshes[0]
  if sorted(found)!=list(range(total)):raise RuntimeError("Missing native source indices "+label)
  assets[label]=[found[i] for i in range(total)]
  if len(set(assets[label]))!=total:raise RuntimeError("Duplicate imported native mesh paths "+label)
  state["parts"][label]={"expected":total,"source_paths":assets[label],
     "native_skeletal_meshes":len(assets[label]),"source_sha_receipts_checked":total}
  save()
 parent=unreal.load_class(None,"/Script/OfflineGame.OGLuminousV3PlayableCharacter")
 gamemode_parent=unreal.load_class(None,"/Script/OfflineGame.OGWorldPresentationGameMode")
 if not parent or not gamemode_parent:raise RuntimeError("Native subclass has not been successfully built")
 if any(unreal.EditorAssetLibrary.does_asset_exist(x) for x in [CHAR,GAMEMODE,WORLD]):
  raise RuntimeError("Refusing to overwrite existing runtime authored QA assets")
 bp=unreal.BlueprintEditorLibrary.create_blueprint_asset_with_parent(CHAR,parent)
 if not isinstance(bp,unreal.Blueprint):raise RuntimeError("Native Luminous BP could not be authored")
 klass=unreal.EditorAssetLibrary.load_blueprint_class(CHAR)
 if not klass:raise RuntimeError("Generated Luminous BP class missing")
 cdo=unreal.get_default_object(klass)
 for label,field in [("CLOTHED","clothed_native_parts"),("UNDERWEAR","underwear_native_parts"),("NUDE","nude_native_parts")]:
  values=[unreal.SoftObjectPath(path) for path in assets[label]]
  cdo.set_editor_property(field,values)
  length=len(cdo.get_editor_property(field))
  if length!=counts[label]:raise RuntimeError("Native UPROPERTY soft asset array not set "+label)
 # Gate11 has not approved a canonical Luminous content identifier yet:
 # leave both blank to fail-closed rather than writing another identity's save.
 cdo.set_editor_property("canonical_luminous_identity_id","")
 cdo.set_editor_property("canonical_luminous_version_id","")
 if not unreal.BlueprintEditorLibrary.compile_blueprint(bp):raise RuntimeError("Luminous Blueprint compile failed")
 if not unreal.EditorAssetLibrary.save_loaded_asset(bp):raise RuntimeError("Luminous Blueprint save failed")
 state["character_saved"]=True;save()
 gm=unreal.BlueprintEditorLibrary.create_blueprint_asset_with_parent(GAMEMODE,gamemode_parent)
 if not isinstance(gm,unreal.Blueprint):raise RuntimeError("Native WorldMode GameMode BP creation failed")
 gm_cls=unreal.EditorAssetLibrary.load_blueprint_class(GAMEMODE)
 if not gm_cls:raise RuntimeError("GameMode class unavailable")
 gm_cdo=unreal.get_default_object(gm_cls)
 gm_cdo.set_editor_property("default_pawn_class",klass)
 if not unreal.BlueprintEditorLibrary.compile_blueprint(gm):raise RuntimeError("WorldMode GM compile failed")
 if not unreal.EditorAssetLibrary.save_loaded_asset(gm):raise RuntimeError("WorldMode GM save failed")
 state["game_mode_saved"]=True;save()
 if not unreal.EditorAssetLibrary.duplicate_asset(SRCWORLD,WORLD):raise RuntimeError("Isolated StartingWorld duplication failed")
 world=unreal.EditorLoadingAndSavingUtils.load_map(WORLD)
 if not world:raise RuntimeError("Isolated map did not load")
 world.get_world_settings().set_editor_property("default_game_mode",gm_cls)
 if not unreal.EditorLoadingAndSavingUtils.save_map(world,WORLD):raise RuntimeError("Isolated map save failed")
 state["world_saved"]=True
 state["status"]="PASS_NATIVE_THREE_WARDROBE_BLUEPRINT_AUTHORED_PENDING_COLD_AND_RUNTIME"
except Exception as e:
 state["status"]="FAIL_NONDESTRUCTIVE_NATIVE_BP_AUTHORING"
 state["exception"]=str(e)
 state["trace"]=traceback.format_exc()[-6000:]
finally:
 state["updated_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())
 save()
 unreal.log("GATE12_LUMINOUS_RUNTIME_WARDROBE_BP="+state["status"])
