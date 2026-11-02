"""Bind already imported genuine source-derived 21-bone CMU animations
to the original Alter owned World character's three independently approved rigs.
Respect frozen source, never enable rejected dance or invent flight choreography.
"""
import unreal,json,pathlib,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
BP="/Game/NoWorld/Characters/Luminous/Alter/BP_Luminous_Alter_Production"
ROOT="/Game/NoWorld/Characters/Luminous/Alter/NativeMocapRealCMU/"
OUT=R/"Saved/Gate12Readiness/Gate12_Alter_ThreeWardrobeRealMocapWorldBinding_20261022.json"
report={"schema":"noworld.gate12.luminous_alter.real_cmu_gameplay_motion_binding.v1",
"status":"STARTED","blueprint":BP,
"source_meshes_modified":False,"scarlet_cadence_rejected_animation_enabled":False,
"rejected_dance_enabled":False,"correct_source_wardrobe_rigs":True,
"visual_motion_approval":"REQUIRES_IN_GAME_VISUAL_INSPECTION",
"motion_array":{}}
try:
 cls=unreal.EditorAssetLibrary.load_blueprint_class(BP)
 assert cls
 cdo=unreal.get_default_object(cls)
 assert cdo.get_editor_property("canonical_luminous_version_id")=="character:luminous_valentine.version_alter_v1"
 bymotion={}
 for label in ("Walk","Run","Jump"):
  assets=[]
  paths=[]
  for state,rigbones in (("CLOTHED",1131),("UNDERWEAR",1131),("NUDE",1123)):
   receipt=R/"Saved/Gate12Readiness"/f"Gate12_Alter_{state}_RealCMU_{label}_20261022.json"
   proof=json.loads(receipt.read_text(encoding="utf8"))
   assert proof["status"]=="IMPORTED_GENUINE_CMU_MOCAP_ON_EXACT_ALTER_RIG_PENDING_VISUAL"
   assert proof["state"]==state and proof["motion"]==label
   assert proof["mapped_bone_count"]>=19
   sk=unreal.load_asset(f"/Game/NoWorld/Characters/Luminous/Alter/NativeOriginal{state.title()}/SK_Luminous_Alter_{state}_OriginalPart_00_Skeleton")
   assert isinstance(sk,unreal.Skeleton)
   assert len(sk.get_reference_pose().get_bone_names())==rigbones
   asset_path=ROOT+f"AN_G12_Alter_{state}_RealCMU_{label}"
   anim=unreal.load_asset(asset_path)
   assert isinstance(anim,unreal.AnimSequence),asset_path
   assert anim.get_editor_property("skeleton")==sk,(label,state)
   assets.append(unreal.SoftObjectPath(asset_path+"."+asset_path.rsplit("/",1)[-1]))
   paths.append(asset_path)
  bymotion[label]=assets
  report["motion_array"][label]=paths
 props={"Walk":"native_walk_by_wardrobe","Run":"native_run_by_wardrobe","Jump":"native_jump_by_wardrobe"}
 for label,prop in props.items():
  previous=list(cdo.get_editor_property(prop))
  assert not previous or len(previous)==3,("Unexpected existing motion metadata "+prop)
  if not previous:
   cdo.set_editor_property(prop,bymotion[label])
  assert len(cdo.get_editor_property(prop))==3,prop
 assert unreal.EditorAssetLibrary.save_loaded_asset(unreal.load_asset(BP))
 report["status"]="NINE_REAL_SOURCE_ALTER_ANIMSEQUENCES_NATIVE_WARDROBE_MOTION_BOUND"
except Exception:
 report["status"]="MOTION_RUNTIME_REGISTRATION_BLOCKED"
 report["error"]=traceback.format_exc()[-4500:]
finally:
 OUT.parent.mkdir(parents=True,exist_ok=True)
 OUT.write_text(json.dumps(report,indent=2,default=str)+"\n",encoding="utf8")
 unreal.log("G12_ALTER_REAL_MOCAP_GAME_BIND="+report["status"])
