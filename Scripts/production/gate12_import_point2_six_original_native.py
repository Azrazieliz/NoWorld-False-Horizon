"""Incremental original-source Koikatsu native assets for actual shared-gameplay integration.
Reuses the validated 20261022 importer; no source or previously authored assets overwritten.
Run G12_POINT2_PHASE=0/1/2. Six distinct action categories, three native Alter wardrobe skeletons.
"""
import os,json,pathlib,runpy,traceback,unreal
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
PHASE=int(os.environ["G12_POINT2_PHASE"])
SET=int(os.environ.get("G12_POINT2_SET","0"))
assert SET in (0,1,2)
CLIPS=[
 ("f_hasiru_00_00_01","Traversal.RunOriginal"),
 ("f_suiei_00_01_M","Traversal.SwimOriginal"),
 ("f_reaction_00_00_M","Combat.HitReaction"),
 ("f_suwari_00_01_M","Interaction.Sit"),
 ("f_eat_00_07_S","Interaction.Eat"),
 ("f_cheer_00_00","Emote.Cheer")
]
if SET==1:
 CLIPS=[
  ("f_manken_00_00_01_L","Interaction.MartialPractice"),
  ("f_tachi_00_00","Interaction.StandUp"),
  ("f_talk_00_07_00","Interaction.TalkAlternate"),
  ("f_sadou_00_00_00","Interaction.Tea"),
  ("f_reaction_00_02_M","Combat.HitReactionAlternate"),
  ("f_action_00_04_01","Interaction.ActionAlternate")]
if SET==2:
 CLIPS=[("M_M_Idle","Adult.QA.OriginalSourceIdle"),
        ("M_K_Loop","Adult.QA.OriginalSourceLoop")]
assert PHASE in (0,1,2)
assert SET!=2 or PHASE==0
work=CLIPS[PHASE*2:(PHASE+1)*2]
target=R/"Saved/Gate12Readiness"/(f"GATE12_POINT2_REAL_ORIGINAL_NATIVE_PHASE_{PHASE}_20261023.json" if SET==0 else f"GATE12_POINT2_REAL_ORIGINAL_NATIVE_SET{SET}_PHASE_{PHASE}_20261023.json")
tool=R/"Scripts/production/gate12_import_real_koikatsu_studio_anim_on_alter_native_ue583_20261022.py"
receipt_dir=R/"Saved/Gate12Readiness"
report={"schema":"noworld.gate12.original_kk_native_point2.v1",
 "phase":PHASE,"source_set":SET,"status":"STARTED","results":[],"source_fabrication":False,
 "gameplay_approval":False,"prior_assets_preserved":True}
def save():
 target.parent.mkdir(parents=True,exist_ok=True)
 target.write_text(json.dumps(report,indent=2,default=str)+"\n",encoding="utf-8")
save()
for clip,action in work:
 for outfit in ("CLOTHED","UNDERWEAR","NUDE"):
  receipt=receipt_dir/f"GATE12_NATIVE_KOIKATSU_STUDIO_{outfit}_{clip}_20261022.json"
  path=f"/Game/NoWorld/Characters/Luminous/Alter/KoikatsuOriginalStudioMotionQA/AN_G12_KK_Original_{outfit}_{clip}"
  existing=unreal.EditorAssetLibrary.does_asset_exist(path)
  if not existing:
   os.environ["G12_KK_SOURCE_STATE"]=outfit
   os.environ["G12_KK_SOURCE_CLIP"]=clip
   try:runpy.run_path(str(tool),run_name="__main__")
   except Exception:unreal.log_error("GATE12_POINT2_NATIVE_IMPORT_FAIL "+
        clip+" "+outfit+": "+traceback.format_exc()[-1200:])
  status="MISSING_OR_INCOMPLETE"
  native=None
  if receipt.exists():
   item=json.loads(receipt.read_text(encoding="utf-8"))
   native=item.get("native_animsequence")
   if item["status"]=="TRUE_ORIGINAL_KK_STUDIO_CLIP_IMPORTED_AS_NATIVE_ANIMSEQUENCE_PRE_VISUAL" and unreal.EditorAssetLibrary.does_asset_exist(path):
    status="NATIVE_REAL_ANIMSEQUENCE_SOURCE_PRESERVED" if existing else "NATIVE_REAL_ANIMSEQUENCE_CREATED"
  report["results"].append({"clip":clip,"semantic":action,"wardrobe":outfit,
                            "status":status,"native_asset":native})
  report["completed"]=sum("NATIVE_REAL_ANIMSEQUENCE" in x["status"] for x in report["results"])
  save()
  unreal.log(f"POINT2_NATIVE {len(report['results'])}/{len(work)*3} {clip} {outfit} {status}")
report["status"]="ALL_SIX_NATIVE_ANIMSEQUENCES_CREATED_OR_PRESERVED_PENDING_VISUAL" if report["completed"]==len(work)*3 else "PARTIAL_NATIVE_INTEGRATION"
save()
unreal.log("GATE12_POINT2_PHASE_RESULT="+report["status"])
