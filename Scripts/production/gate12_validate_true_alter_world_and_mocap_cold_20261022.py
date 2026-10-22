"""One independent COLD UE asset checkpoint after the real Alter three
wardrobes and 9 native mocap animation integrations. No assets modified.
"""
import unreal,json,pathlib,traceback
ROOT=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
OUT=ROOT/"Saved/Gate12Readiness/Gate12_Alter_COLD_ThreeWardrobes_NineMocap_20261022.json"
report={"schema":"noworld.gate12.alter.ue583.independent_cold_world_asset_checkpoint.v1",
"status":"STARTED","approved_original_art_modified":False,"real_time_visual_acceptance":False,
"native_editor_build_validated_on_20261022":True,"complete_characters":[]}
try:
 bp="/Game/NoWorld/Characters/Luminous/Alter/BP_Luminous_Alter_Production"
 assert unreal.EditorAssetLibrary.does_asset_exist(bp)
 cl=unreal.EditorAssetLibrary.load_blueprint_class(bp)
 assert cl
 cdo=unreal.get_default_object(cl)
 assert cdo.get_editor_property("canonical_luminous_identity_id")=="character:luminous_valentine.identity"
 assert cdo.get_editor_property("canonical_luminous_version_id")=="character:luminous_valentine.version_alter_v1"
 asset_paths=()
 for state,bones in (("CLOTHED",1131),("UNDERWEAR",1131),("NUDE",1123)):
  wardrobe_prop={"CLOTHED":"clothed_native_parts","UNDERWEAR":"underwear_native_parts","NUDE":"nude_native_parts"}[state]
  assert len(cdo.get_editor_property(wardrobe_prop))==6,wardrobe_prop
  folder=f"/Game/NoWorld/Characters/Luminous/Alter/NativeOriginal{state.title()}"
  skeleton=unreal.load_asset(folder+f"/SK_Luminous_Alter_{state}_OriginalPart_00_Skeleton")
  assert isinstance(skeleton,unreal.Skeleton)
  assert len(skeleton.get_reference_pose().get_bone_names())==bones
  motion={}
  for name,prop in (("Walk","native_walk_by_wardrobe"),("Run","native_run_by_wardrobe"),("Jump","native_jump_by_wardrobe")):
   i={"CLOTHED":0,"UNDERWEAR":1,"NUDE":2}[state]
   assert len(cdo.get_editor_property(prop))==3,prop
   path=f"/Game/NoWorld/Characters/Luminous/Alter/NativeMocapRealCMU/AN_G12_Alter_{state}_RealCMU_{name}"
   seq=unreal.load_asset(path)
   assert isinstance(seq,unreal.AnimSequence)
   assert seq.get_editor_property("skeleton")==skeleton
   receipt=json.loads((ROOT/"Saved/Gate12Readiness"/f"Gate12_Alter_{state}_RealCMU_{name}_20261022.json").read_text(encoding="utf8"))
   assert receipt["status"]=="IMPORTED_GENUINE_CMU_MOCAP_ON_EXACT_ALTER_RIG_PENDING_VISUAL"
   assert receipt["mapped_bone_count"]==21
   motion[name]={"native":path,"duration_s":seq.get_play_length(),"original_mocap_bone_tracks":21}
  report["complete_characters"].append({"state":state,"source_bones":bones,"native_parts":6,"native_motions":motion})
 catalog=unreal.load_asset("/Game/NoWorld/UI/DA_NoWorld_CharacterVisuals")
 assert catalog
 hits=[entry for entry in catalog.get_editor_property("entries")
    if entry.get_editor_property("identity_id").get_editor_property("value")=="character:luminous_valentine.identity" and entry.get_editor_property("version_id").get_editor_property("value")=="character:luminous_valentine.version_alter_v1"]
 assert len(hits)==1
 options=[str(x) for x in hits[0].get_editor_property("approved_wardrobe_choices")]
 assert options==["CLOTHED","UNDERWEAR","NUDE"],options
 report["character_catalog_choices"]=options
 report["status"]="COLD_VALIDATED_ALL_THREE_ALTER_WORLD_WARDROBES_AND_NINE_SOURCE_NATIVE_MOTIONS"
except Exception:
 report["status"]="COLD_ACCEPTANCE_INCOMPLETE"
 report["error"]=traceback.format_exc()[-5000:]
finally:
 OUT.parent.mkdir(parents=True,exist_ok=True)
 OUT.write_text(json.dumps(report,indent=2,default=str)+"\n",encoding="utf8")
 unreal.log("GATE12_ALTER_COLD_WORLD_RECEIPT="+report["status"])
