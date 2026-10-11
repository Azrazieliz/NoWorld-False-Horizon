"""Bind nine genuine, exact-native Luminous 975-joint UE CMU mocap clips
to one isolated real World Mode character Blueprint. All source and prior QA
assets remain immutable except the designated RuntimeSwitchQA Blueprint.
"""
import unreal,pathlib,json,time,traceback
W=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
Q=W/"Saved/Gate12Koikatsu/LuminousV3"
A=json.loads((Q/"native_121_wardrobe_runtime_switch_ue583.json").read_text("utf-8-sig"))
OUT=Q/"luminous_native_three_wardrobe_motion_v3_bound_ue583.json"
r={"status":"START","result_class":"ENGINE_NATIVE_ANIM_SEQUENCE_BINDING",
 "parts_unchanged":True,"morph_deficits_remain":45,"runtime_visual_approval":False,
 "android":"NOT_TESTED","gate11_artifact_ready":False,"native_clips":[]}
def save():OUT.write_text(json.dumps(r,indent=2,default=str)+"\n",encoding="utf8")
save()
try:
 if A["status"]!="PASS_NATIVE_THREE_WARDROBE_BLUEPRINT_AUTHORED_PENDING_COLD_AND_RUNTIME":raise RuntimeError("No native playable character authority")
 bp=unreal.load_asset(A["character"])
 if not isinstance(bp,unreal.Blueprint):raise RuntimeError("Native QA Blueprint missing")
 klass=unreal.EditorAssetLibrary.load_blueprint_class(A["character"])
 cdo=unreal.get_default_object(klass)
 motion_receipts={}
 for state in ("CLOTHED","UNDERWEAR","NUDE"):
  f=Q/"MotionV3"/("luminous_%s_native_real_cmu_three_clip_ue583.json"%state)
  j=json.loads(f.read_text("utf8"))
  if j["status"]!="PASS_3_NATIVE_LUMINOUS_CMUMOCAP_CLIPS_PRE_VISUAL" or len(j["native_sequences"])!=3:
   raise RuntimeError("Missing authoritative native 975-source mocap "+state)
  if j["native_bones"]!=975 or j["native_tracks"]!=22:raise RuntimeError("Unexpected 975 native author spec")
  motion_receipts[state]={x["motion"]:x for x in j["native_sequences"]}
 for motion,prop in (("Walk","native_walk_by_wardrobe"),("Run","native_run_by_wardrobe"),("Jump","native_jump_by_wardrobe")):
  resolved=[]
  for state in ("CLOTHED","UNDERWEAR","NUDE"):
   item=motion_receipts[state][motion]
   obj=unreal.load_asset(item["native_animation"])
   if not isinstance(obj,unreal.AnimSequence):raise RuntimeError("Native animation .uasset absent "+state+"/"+motion)
   if obj.get_editor_property("skeleton").get_path_name()!=item["exact_skeleton"]:
    raise RuntimeError("Native CMU incompatible source skeleton "+state+"/"+motion)
   names={str(x).casefold() for x in obj.get_editor_property("controller").get_model_interface().get_bone_track_names()}
   if len(names)!=22:raise RuntimeError("Wrong native 22 bone tracks "+state+"/"+motion)
   if abs(obj.get_play_length()-item["duration_sec"])>0.02:raise RuntimeError("Native UE mocap duration changed")
   resolved.append(unreal.SoftObjectPath(item["native_animation"]))
   r["native_clips"].append({"state":state,"motion":motion,"native_animation":item["native_animation"],
     "skeleton":item["exact_skeleton"],"confirmed_ue_native_tracks":len(names),
     "source_real_mocap":True,"native_cold_asset_loaded":True})
  cdo.set_editor_property(prop,resolved)
  if len(cdo.get_editor_property(prop))!=3:raise RuntimeError("Native CDO motion array lost "+motion)
 # Enable motion proof only on the isolated existing QA Blueprint, NOT production.
 cdo.set_editor_property("run_native_motion_qa_cycle",True)
 cdo.set_editor_property("run_non_persistent_wardrobe_qa_cycle",True)
 if not unreal.BlueprintEditorLibrary.compile_blueprint(bp):raise RuntimeError("Native Blueprint compile failed")
 if not unreal.EditorAssetLibrary.save_loaded_asset(bp):raise RuntimeError("Native Blueprint save failed")
 r["native_gameplay_class"]=A["character"]
 r["wardrobe_state_source_identity"]="ONE_LUMINOUS_CHARACTER_VERSION"
 r["native_animation_clips"]=len(r["native_clips"])
 r["status"]="PASS_9_REAL_975_CMUMOCAP_CLIPS_BOUND_TO_NATIVE_RUNTIME_QA_NOT_VISUALLY_APPROVED"
except Exception as e:
 r["status"]="FAIL_NATIVE_975_CMUMOCAP_BIND"
 r["exception"]=str(e);r["trace"]=traceback.format_exc()[-5500:]
finally:
 r["updated_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime());save()
 unreal.log("GATE12_LUMINOUS_MOTION_V3_RUNTIME_BIND="+r["status"])
