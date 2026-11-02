"""Independent UE 5.8.3 cold verification of installed genuine Koikatsu
Studio AnimationClip conversions. QA asset presence != gameplay approval.
"""
import unreal,pathlib,json,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
CAT=R/"SourceAssets/Gate12/Animations20261022/GATE12_ALL_INSTALLED_KK_ORIGINAL_ANIMATION_NATIVE_INGESTION_20261022.json"
OUT=R/"Saved/Gate12Readiness/GATE12_COLD_24_KK_ORIGINAL_NATIVE_UNREAL_CLIPS_20261022.json"
d=json.loads(CAT.read_text(encoding="utf8"))
assert d["original_KK_animsequence_imported_native_UE_now"]==24
r={"schema":"noworld.gate12.native.koikatsu.studio.animsequence.cold.v1",
"status":"STARTED","source_original_modified":False,"gameplay_approval":False,
"original_user_approved_character_visuals_overwritten":False,"clips":[]}
try:
 assert R.name in unreal.Paths.get_project_file_path()
 rigs={}
 for state,expected in (("CLOTHED",1131),("UNDERWEAR",1131),("NUDE",1123)):
  path=f"/Game/NoWorld/Characters/Luminous/Alter/NativeOriginal{state.title()}/SK_Luminous_Alter_{state}_OriginalPart_00_Skeleton"
  obj=unreal.load_asset(path)
  assert isinstance(obj,unreal.Skeleton),path
  assert len(obj.get_reference_pose().get_bone_names())==expected
  rigs[state]=obj
 for row in d["native_unreal_assets"]:
  asset=unreal.load_asset(row["uasset"])
  assert isinstance(asset,unreal.AnimSequence),row["uasset"]
  assert asset.get_editor_property("skeleton")==rigs[row["state"]]
  original=R/"Saved/Gate12Readiness"/f"GATE12_NATIVE_KOIKATSU_STUDIO_{row['state']}_{row['clip']}_20261022.json"
  detail=json.loads(original.read_text(encoding="utf8"))
  assert detail["status"]=="TRUE_ORIGINAL_KK_STUDIO_CLIP_IMPORTED_AS_NATIVE_ANIMSEQUENCE_PRE_VISUAL"
  assert detail["model_track_count"]>=140
  assert detail["model_track_count"]==detail["written_track_count"]
  assert abs(asset.get_play_length()-detail["duration_s"])<.07
  r["clips"].append({"original_clip_name":row["clip"],"state":row["state"],
   "original_unity_source_path_id":detail["source_unity_path_id"],
   "real_UE_skeleton":rigs[row["state"]].get_path_name(),
   "native_asset":asset.get_path_name(),
   "verified_bone_tracks":detail["model_track_count"],
   "duration_s":asset.get_play_length(),
   "visual_approval":"PENDING"})
 assert len(r["clips"])==24
 assert len({(x["original_clip_name"],x["state"])for x in r["clips"]})==24
 r["status"]="COLD_VALIDATED_24_GENUINE_KOIKATSU_STUDIO_NATIVE_UNREAL_ANIMSEQUENCES"
except Exception:
 r["status"]="COLD_KK_STUDIO_NATIVE_INTAKE_INCOMPLETE"
 r["error"]=traceback.format_exc()[-5000:]
finally:
 OUT.parent.mkdir(parents=True,exist_ok=True)
 OUT.write_text(json.dumps(r,indent=2,default=str)+"\n",encoding="utf8")
 unreal.log("GATE12_ORIGINAL_KOIKATSU_COLD_NATIVE_24="+r["status"])
