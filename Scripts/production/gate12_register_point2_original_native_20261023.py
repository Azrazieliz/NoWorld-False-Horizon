"""Register actual newly imported original Koikatsu native UE clips into shared catalog.
No automatic QA approval, no overwrite of existing catalog definitions or source assets.
"""
import json,gzip,hashlib,pathlib,os
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
catalog=R/"Content/NoWorld/Animation/SharedMotionCatalog.json"
path=R/"Content/NoWorld/Characters/Luminous/Alter/KoikatsuOriginalStudioMotionQA"
SET=int(os.environ.get("G12_POINT2_SET","0"))
assert SET in (0,1,2)
cases=[
("f_hasiru_00_00_01","Traversal.Run","Traversal","grounded",True),
("f_suiei_00_01_M","Traversal.Swim","Traversal","swimming",True),
("f_reaction_00_00_M","Combat.HitReaction","Combat","combat",False),
("f_suwari_00_01_M","Interaction.Sit","Interaction","emote",False),
("f_eat_00_07_S","Interaction.Eat","Interaction","emote",False),
("f_cheer_00_00","Emote.Cheer","Interaction","emote",False)]
if SET==1:
 cases=[
  ("f_manken_00_00_01_L","Interaction.UnclassifiedManken","Interaction","emote",False),
  ("f_tachi_00_00","Interaction.StandUp","Interaction","emote",False),
  ("f_talk_00_07_00","Interaction.TalkAlternate","Interaction","emote",False),
  ("f_sadou_00_00_00","Interaction.Tea","Interaction","emote",False),
  ("f_reaction_00_02_M","Combat.HitReactionAlternate","Combat","combat",False),
  ("f_action_00_04_01","Interaction.ActionAlternate","Interaction","emote",False)]
if SET==2:
 cases=[
  ("M_M_Idle","Adult.QA.OriginalSourceIdle","AdultInteraction","adult_interaction",True),
  ("M_K_Loop","Adult.QA.OriginalSourceLoop","AdultInteraction","adult_interaction",True)]
doc=json.loads(catalog.read_text(encoding="utf-8"))
assert doc["schema"]=="noworld.shared_motion_catalog.v1"
ids={m["motion_id"]for m in doc["motions"]}
assert len(ids)==len(doc["motions"]),"Duplicate canonical MotionId before new registration"
inserted=[]
for clip,action,context,capability,loop in cases:
 source=R/"Saved/Gate12Readiness/KoikatsuGenuineNativeStudioMotion"/(clip+".genuine_unity_kk_stream_native_deltas.json.gz")
 with gzip.open(source,"rt",encoding="utf8") as f: src=json.load(f)
 assert src["clip"]==clip and src["status"]=="SOURCE_AUTHENTIC_QUATERNION_TRACKS_CALIBRATED_FOR_NATIVE_UE_REST_BASIS"
 for outfit in ("CLOTHED","UNDERWEAR","NUDE"):
  rid=R/"Saved/Gate12Readiness"/f"GATE12_NATIVE_KOIKATSU_STUDIO_{outfit}_{clip}_20261022.json"
  receipt=json.loads(rid.read_text("utf-8"))
  assert receipt["status"]=="TRUE_ORIGINAL_KK_STUDIO_CLIP_IMPORTED_AS_NATIVE_ANIMSEQUENCE_PRE_VISUAL",rid
  n=f"AN_G12_KK_Original_{outfit}_{clip}"
  binary=path/(n+".uasset")
  assert binary.exists() and binary.stat().st_size>10000,binary
  motion_id=f"kk.alter.studio.{outfit}.{clip}"
  assert motion_id not in ids,motion_id
  entry={
   "motion_id":motion_id,
   "source_clip_id":f"kk.original.unity.{src['source_path_id']}",
   "source_hash":src["source_clip_sha256"],
   "source_bundle":src["source_bundle"],
   "source_unity_path_id":src["source_path_id"],
   "source_bundle_sha256":src["source_sha256"],
   "action":action,"category":context,"context":context,
   "rig":"koikatsu_original","anatomy":"humanoid","outfit":outfit,
   "capabilities":[capability],
   "native_sequence":f"/Game/NoWorld/Characters/Luminous/Alter/KoikatsuOriginalStudioMotionQA/{n}.{n}",
   "native_uasset_sha256":hashlib.sha256(binary.read_bytes()).hexdigest(),
   "native_bone_tracks":receipt["bone_tracks"],
   "native_duration_seconds":receipt["duration_s"],
   "loop":loop,"participants":1,"play_rate":1,
   "approved_for_gameplay":False,"adult_restricted":SET==2,
   "review_state":"REQUIRES_ADULT_ELIGIBILITY_CONSENT_PAIRED_SCENE_RIG_AND_VISUAL_ACCEPTANCE" if SET==2 else "AUTHENTIC_NATIVE_IMPORT_NEEDS_IN_GAME_VISUAL_ACCEPTANCE",
   "priority_if_approved":5}
  doc["motions"].append(entry)
  ids.add(motion_id)
  inserted.append(motion_id)
assert len(inserted)==(6 if SET==2 else 18)
# No replacing existing file with a transient or partial result.
tmp=catalog.with_suffix(".json.new")
tmp.write_text(json.dumps(doc,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
json.loads(tmp.read_text(encoding="utf-8"))
tmp.replace(catalog)
print("POINT2_REGISTERED_GENUINE_NATIVE",len(inserted),"CATALOG_TOTAL",len(doc["motions"]))
