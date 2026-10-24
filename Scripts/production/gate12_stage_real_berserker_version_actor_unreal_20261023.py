"""Stage real 6-part Koikatsu Berserker as its own Unreal native character,
without replacing approved Alter, inventing missing wardrobes, or granting a
canonical Manifestation. Actor becomes truly installable after source pairing
and complete wardrobe/artistic acceptance.
"""
import unreal,json,traceback,pathlib
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
SRC="/Game/NoWorld/Characters/Luminous/Alter/BP_Luminous_Alter_Production"
DST="/Game/NoWorld/Characters/Luminous/Berserker/BP_Luminous_Berserker_OriginalSourceNative"
V="character:luminous_valentine.version_berserker_v4"
STATE="UNDERWEAR"
FOLDER="/Game/NoWorld/Characters/Luminous/Berserker/NativeOriginalUnderwear/"
RECEIPT=R/"Saved/Gate12Readiness/GATE12_BERSERKER_ORIGINAL_KOIKATSU_UNREAL_CHARACTER_20261023.json"
doc={"schema":"noworld.gate12.native.luminous.berserker.source_actor.v1",
 "status":"STARTED","version":V,"actor":DST,"base_or_alter_modified":False,
 "canonical_ownership_granted":False,
 "visual_source_card_to_export_hash":"PENDING_REAL_KKBP_MAKER_PAIRING",
 "wardrobes_implemented":["UNDERWEAR"],"missing":["CLOTHED","NUDE"]}
try:
 proof=[]
 for i in range(6):
  r=R/"Saved/Gate12Readiness"/f"GATE12_Berserker_UNDERWEAR_NATIVE_UE_PART_{i:02d}_20261023.json"
  data=json.loads(r.read_text(encoding="utf8"))
  assert data["status"]=="REAL_SOURCE_NATIVE_UNREAL_SKELETAL_PART_IMPORTED"
  assert data["unreal_bones"]==886
  proof.append(data)
 assert unreal.EditorAssetLibrary.does_asset_exist(SRC)
 assert not unreal.EditorAssetLibrary.does_asset_exist(DST)
 unreal.EditorAssetLibrary.make_directory("/Game/NoWorld/Characters/Luminous/Berserker")
 # New Version receives the established native actor class, not Alter's
 # copied Blueprint graph, wardrobe animations, or material substitutions.
 factory=unreal.BlueprintFactory()
 factory.set_editor_property("parent_class",unreal.OGLuminousV3PlayableCharacter)
 bp=unreal.AssetToolsHelpers.get_asset_tools().create_asset(
  "BP_Luminous_Berserker_OriginalSourceNative",
  "/Game/NoWorld/Characters/Luminous/Berserker",
  unreal.Blueprint,factory)
 assert bp
 cls=unreal.EditorAssetLibrary.load_blueprint_class(DST)
 assert cls
 cdo=unreal.get_default_object(cls)
 cdo.set_editor_property("canonical_luminous_identity_id","character:luminous_valentine.identity")
 cdo.set_editor_property("canonical_luminous_version_id",V)
 cdo.set_editor_property("clothed_native_parts",[])
 cdo.set_editor_property("nude_native_parts",[])
 parts=[]
 skel=unreal.load_asset(FOLDER+"SK_Luminous_Berserker_UNDERWEAR_OriginalPart_00_Skeleton")
 assert isinstance(skel,unreal.Skeleton)
 for i in range(6):
  name=f"SK_Luminous_Berserker_UNDERWEAR_OriginalPart_{i:02d}"
  path=FOLDER+name
  mesh=unreal.load_asset(path)
  assert isinstance(mesh,unreal.SkeletalMesh)
  assert mesh.get_editor_property("skeleton")==skel
  parts.append(unreal.SoftObjectPath(path+"."+name))
 cdo.set_editor_property("underwear_native_parts",parts)
 cdo.set_editor_property("additional_version_native_part_counts",[0,6,0])
 cdo.set_editor_property("additional_version_native_skeleton_bones",[0,886,0])
 cdo.set_editor_property("native_walk_by_wardrobe",[])
 cdo.set_editor_property("native_run_by_wardrobe",[])
 cdo.set_editor_property("native_jump_by_wardrobe",[])
 cdo.set_editor_property("run_non_persistent_wardrobe_qa_cycle",False)
 cdo.set_editor_property("run_native_motion_qa_cycle",False)
 cdo.set_editor_property("enable_native_wardrobe_qa_touch_panel",False)
 cdo.set_editor_property("enter_world_mode_on_isolated_qa_startup",False)
 cdo.set_editor_property("initial_wardrobe",unreal.OGLuminousV3Wardrobe.UNDERWEAR)
 obj=unreal.load_asset(DST)
 assert unreal.EditorAssetLibrary.save_loaded_asset(obj)
 assert len(cdo.get_editor_property("underwear_native_parts"))==6
 doc["native_skeleton"]=skel.get_path_name()
 doc["source_original_native_part_count"]=6
 doc["native_skeletal_bones"]=886
 doc["status"]="REAL_KKBP_SIXPART_CHARACTER_BLUEPRINT_STAGED_UNREGISTERED"
except Exception:
 doc["status"]="BLUEPRINT_STAGING_FAILED"
 doc["error"]=traceback.format_exc()[-5500:]
finally:
 RECEIPT.parent.mkdir(parents=True,exist_ok=True)
 RECEIPT.write_text(json.dumps(doc,indent=2)+"\n",encoding="utf8")
 unreal.log("G12_NATIVE_BERSERKER_ACTOR="+doc["status"])
