"""Retarget original Koikatsu source clips to the new actual Berserker skeleton.
No synthetic poses, no version-specific catalog. Original Unity source curve
identity survives each derived AnimSequence, with source IDs reused globally.
"""
import unreal, os, runpy, pathlib, json, gc, traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
IMPORT=R/"Scripts/production/gate12_import_real_koikatsu_studio_anim_on_alter_native_ue583_20261022.py"
os.environ["G12_KK_SOURCE_FORM"]="Berserker"
os.environ["G12_KK_SOURCE_STATE"]="UNDERWEAR"
CLIPS=["M_M_Idle","M_K_Loop","f_reaction_00_00_M","f_cheer_00_00","f_suiei_00_01_M"]
out=R/"Saved/Gate12Readiness/GATE12_BERSERKER_NATIVE_SHARED_SOURCE_DERIVATIVES_20261023.json"
results=[]
try:
 for clip in CLIPS:
  os.environ["G12_KK_SOURCE_CLIP"]=clip
  name=f"AN_G12_KK_Berserker_UNDERWEAR_{clip}"
  path=f"/Game/NoWorld/Characters/Luminous/Berserker/KoikatsuOriginalStudioMotionQA/{name}"
  try:
   if unreal.EditorAssetLibrary.does_asset_exist(path):
    result={"clip":clip,"result":"NATIVE_ALREADY_INSTALLED_PRESERVED"}
   else:
    runpy.run_path(str(IMPORT),run_name="__main__")
    rep=R/"Saved/Gate12Readiness"/f"GATE12_Berserker_UNDERWEAR_{clip}_ORIGINAL_NATIVE_20261023.json"
    data=json.loads(rep.read_text(encoding="utf8"))
    result={"clip":clip,"result":data["status"],
            "bone_tracks":data.get("bone_tracks"),
            "unmapped":data.get("unmapped_rotation_bones"),
            "path":path}
   if not unreal.EditorAssetLibrary.does_asset_exist(path):
    raise RuntimeError("Native sequence missing after import: "+clip)
   seq=unreal.load_asset(path)
   assert isinstance(seq,unreal.AnimSequence)
   result["length_seconds"]=seq.get_play_length()
  except Exception:
   result={"clip":clip,"result":"FAILED","error":traceback.format_exc()[-1900:]}
  results.append(result)
  out.write_text(json.dumps({"source_derivatives":results},indent=2)+"\n",encoding="utf8")
  unreal.log("GATE12_BERSERKER_REAL_SHARED_MOTION="+clip+" "+result["result"])
  unreal.SystemLibrary.collect_garbage()
finally:
 out.write_text(json.dumps({"source_derivatives":results,"imported_count":
  sum(r["result"]!="FAILED" for r in results),
  "original_unity_clips_replaced":False,
  "source_kits_changed":False},indent=2)+"\n",encoding="utf8")
