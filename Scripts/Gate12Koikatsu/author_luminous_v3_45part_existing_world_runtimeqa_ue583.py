"""Non-destructive native Unreal full CLOTHED test pawn, inherits existing movement/camera/input/world mode.
Not Gate11 acceptance; does not substitute a standalone game or accept animation/Android.
Rerunnable for partial incomplete authorship; never deletes existing parts or reimports source.
"""
import unreal,pathlib,json,time,traceback,collections
W=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
Q=W/"Saved/Gate12Koikatsu/LuminousV3"
C=Q/"CLOTHED"
INV=json.loads((C/"source_shards_actual_nonzero_morph_inventory.json").read_text("utf-8-sig"))
NONZERO={int(x["index"]):x for x in INV["nonzero_by_shard"]}
ROOT="/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011"
PLAYER=ROOT+"/BP_LuminousValentineV3_CLOTHED_RuntimeQA"
GM=ROOT+"/BP_LuminousV3_WorldMode_RuntimeQA"
WORLD=ROOT+"/Maps/L_LuminousV3_Playable_RuntimeQA"
SOURCE_WORLD="/Game/Maps/StartingWorld"
OUT=Q/"CLOTHED"/"luminous_playable_45part_native_blueprint_ue583.json"
o={"status":"START","scope":"REAL_NATIVE_FULL_CHARACTER_CLOTHED_ENGINEERING_QA","character_identity":"Luminous Valentine V3",
   "native_part_count_required":45,"player":PLAYER,"game_mode":GM,"world":WORLD,
   "animation_state":"NOT_TESTED","runtime_switching":"NOT_TESTED","android":"NOT_TESTED",
   "gate11_artifact_ready":False,"parts":[],"warnings":[]}
def save():OUT.write_text(json.dumps(o,ensure_ascii=False,indent=2,default=str)+"\n",encoding="utf8")
save()
try:
 if "OfflineGame_Gate12_Koikatsu_20261010" not in str(unreal.Paths.get_project_file_path()):raise RuntimeError("Wrong UE project")
 parts={};listfiles=list(C.glob("native_shards_*_ue583.json"))
 for f in listfiles:
  r=json.loads(f.read_text("utf-8-sig"))
  for p in r["native_parts"]:
   i=int(p["index"])
   if i in parts:raise RuntimeError("Duplicate source index "+str(i))
   meshes=[x["path"] for x in p["assets"] if x["class"]=="SkeletalMesh"]
   if len(meshes)!=1:raise RuntimeError("No single skeletal mesh index "+str(i))
   parts[i]={"path":meshes[0],"mesh":int(p["mesh"]),"prim":int(p["primitive"]),
      "source_sha256":p["sha256"],"ue_native_morphs":int(p["ue_total_native_morph_targets"]),
      "source_nonzero_morphs":int(NONZERO[i]["nonzero_position_morphs"])}
 if sorted(parts)!=list(range(45)):raise RuntimeError("Not all CLOTHED sources imported/receipted")
 o["source_receipts"]=len(listfiles)
 if not unreal.EditorAssetLibrary.does_asset_exist(SOURCE_WORLD):raise RuntimeError("Frozen world map missing")
 parent=unreal.load_class(None,"/Script/OfflineGame.OGWorldPrototypeCharacter")
 game=unreal.load_class(None,"/Script/OfflineGame.OGWorldPresentationGameMode")
 if not parent or not game:raise RuntimeError("Existing game movement architecture missing")
 bp=unreal.load_asset(PLAYER) if unreal.EditorAssetLibrary.does_asset_exist(PLAYER) else unreal.BlueprintEditorLibrary.create_blueprint_asset_with_parent(PLAYER,parent)
 if not isinstance(bp,unreal.Blueprint):raise RuntimeError("Could not create/adopt native character BP")
 subsys=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
 lib=unreal.SubobjectDataBlueprintFunctionLibrary
 handles=subsys.k2_gather_subobject_data_for_blueprint(bp)
 if not handles:raise RuntimeError("No SCS root")
 root=handles[0]
 existing={}
 for handle in handles:
  try:
   d=subsys.k2_find_subobject_data_from_handle(handle)
   x=lib.get_object(d)
   if x:
    existing[x.get_name()]=x
    if x.get_name().lower() in ("mesh","character_mesh0","prototypebody") and isinstance(x,unreal.SkeletalMeshComponent):
     x.set_editor_property("hidden_in_game",True)
  except Exception as err:o["warnings"].append("ExistingComponent:"+str(err)[:200])
 o["inherited_existing_parts"]=sorted(existing)
 save()
 for i in range(45):
  p=parts[i];mesh=unreal.load_asset(p["path"])
  if not isinstance(mesh,unreal.SkeletalMesh):raise RuntimeError("Native imported mesh asset unavailable "+str(i))
  sk=mesh.get_editor_property("skeleton")
  bones=len(sk.get_editor_property("bone_tree")) if sk else -1
  if bones!=975:raise RuntimeError("Wrong native bone count index=%s bones=%s"%(i,bones))
  label="LV3_C_M%02dP%02d"%(p["mesh"],p["prim"])
  if label in existing: comp=existing[label]
  else:
   handle,reason=subsys.add_new_subobject(params=unreal.AddNewSubobjectParams(
      parent_handle=root,new_class=unreal.SkeletalMeshComponent,blueprint_context=bp))
   if not handle:raise RuntimeError("Subobject creation index="+str(i)+" reason="+str(reason))
   subsys.rename_subobject(handle=handle,new_name=unreal.Text(label))
   d=subsys.k2_find_subobject_data_from_handle(handle)
   comp=lib.get_object(d)
  if not isinstance(comp,unreal.SkeletalMeshComponent):raise RuntimeError("Not a skeletal template at "+label)
  if comp.get_skeletal_mesh_asset() and comp.get_skeletal_mesh_asset().get_path_name()!=mesh.get_path_name():
   raise RuntimeError("Existing component points to wrong asset; no overwrite "+label)
  comp.set_skeletal_mesh_asset(mesh)
  # Geometry lives in glTF source coordinates, no guess at personal Luminous forward-axis correction.
  comp.set_editor_property("relative_location",unreal.Vector(0,0,-96))
  comp.set_editor_property("relative_rotation",unreal.Rotator(pitch=0,yaw=0,roll=0))
  comp.set_editor_property("relative_scale3d",unreal.Vector(1,1,1))
  comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
  comp.set_editor_property("cast_shadow",True)
  # Do not falsely bind incompatible CMU fixture animation or rejected V1/V2.
  o["parts"].append({"index":i,"component":label,"path":mesh.get_path_name(),"bones":bones,
    "skeleton":sk.get_path_name(),"native_morphs":p["ue_native_morphs"],
    "source_morphs":p["source_nonzero_morphs"],"sha256":p["source_sha256"]})
  if i % 5 ==4:save()
 if len(o["parts"])!=45:raise RuntimeError("Incomplete assembled full character")
 if not unreal.BlueprintEditorLibrary.compile_blueprint(bp):raise RuntimeError("Player BP native compilation failed")
 if not unreal.EditorAssetLibrary.save_loaded_asset(bp):raise RuntimeError("Player BP save failed")
 o["player_saved"]=True;save()
 gm=unreal.load_asset(GM) if unreal.EditorAssetLibrary.does_asset_exist(GM) else unreal.BlueprintEditorLibrary.create_blueprint_asset_with_parent(GM,game)
 if not isinstance(gm,unreal.Blueprint):raise RuntimeError("Existing game-mode child missing")
 player_cls=unreal.EditorAssetLibrary.load_blueprint_class(PLAYER)
 gm_cls=unreal.EditorAssetLibrary.load_blueprint_class(GM)
 if not player_cls or not gm_cls:raise RuntimeError("Native generated class missing")
 gm_cdo=unreal.get_default_object(gm_cls)
 if gm_cdo.get_editor_property("default_pawn_class") not in (None,player_cls):
  # Existing map under our isolated namespace belongs to same authored QA script, ensure no wrong pawn.
  o["warnings"].append("Existing game mode pawn class will be set to generated class")
 gm_cdo.set_editor_property("default_pawn_class",player_cls)
 if not unreal.BlueprintEditorLibrary.compile_blueprint(gm):raise RuntimeError("GameMode Blueprint compilation failed")
 if not unreal.EditorAssetLibrary.save_loaded_asset(gm):raise RuntimeError("GameMode BP save failed")
 o["world_mode_default_pawn"]=str(gm_cdo.get_editor_property("default_pawn_class"))
 save()
 if not unreal.EditorAssetLibrary.does_asset_exist(WORLD):
  if not unreal.EditorAssetLibrary.duplicate_asset(SOURCE_WORLD,WORLD):raise RuntimeError("Failed to duplicate existing native StartingWorld")
 world=unreal.EditorLoadingAndSavingUtils.load_map(WORLD)
 if not world:raise RuntimeError("Cannot load native isolated map")
 world.get_world_settings().set_editor_property("default_game_mode",gm_cls)
 if not unreal.EditorLoadingAndSavingUtils.save_map(world,WORLD):raise RuntimeError("Map save failed")
 o["status"]="PASS_NATIVE_BLUEPRINT_45_PARTS_ASSEMBLED_PENDING_LIVE_VISUAL_ANIM_ANDROID"
 o["native_world_mode_gameplay_binding"]="COMPILED_BLUEPRINT_NOT_RUNTIME_TESTED"
except Exception as e:
 o["status"]="FAIL_AUTHORING_NONDESTRUCTIVE"
 o["exception"]=str(e)
 o["trace"]=traceback.format_exc()[-6000:]
finally:
 o["updated_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())
 save()
 unreal.log("G12_LUMINOUS_NATIVE_PLAYABLE_QA="+o["status"])
