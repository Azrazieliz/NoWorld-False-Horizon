"""Add exact-skeleton derivative entries to ONE reusable shared motion catalog.
Never per-Version animation architecture, never substitute a foreign Skeleton.
"""
import pathlib,json,hashlib,os
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
catalog=R/"Content/NoWorld/Animation/SharedMotionCatalog.json"
report_path=R/"Saved/Gate12Readiness/GATE12_BERSERKER_REAL_NATIVE_SHARED_CATALOG_20261023.json"
doc=json.loads(catalog.read_text(encoding="utf8"))
assert doc["schema"]=="noworld.shared_motion_catalog.v1"
ids={x["motion_id"]for x in doc["motions"]}
added=[]
for clip in ("M_M_Idle","M_K_Loop","f_reaction_00_00_M","f_cheer_00_00","f_suiei_00_01_M"):
 original=[x for x in doc["motions"] if x.get("motion_id")==f"kk.alter.studio.UNDERWEAR.{clip}"]
 assert len(original)==1,clip
 template=original[0]
 prefix="AN_G12_KK_Berserker_UNDERWEAR_"
 package="/Game/NoWorld/Characters/Luminous/Berserker/KoikatsuOriginalStudioMotionQA/"
 sequence=prefix+clip
 path=R/"Content/NoWorld/Characters/Luminous/Berserker/KoikatsuOriginalStudioMotionQA"/(sequence+".uasset")
 if not path.exists():continue
 receipt=R/"Saved/Gate12Readiness"/f"GATE12_Berserker_UNDERWEAR_{clip}_ORIGINAL_NATIVE_20261023.json"
 if receipt.exists():
  detail=json.loads(receipt.read_text(encoding="utf8"))
  assert detail["status"]=="TRUE_ORIGINAL_KK_STUDIO_CLIP_IMPORTED_AS_NATIVE_ANIMSEQUENCE_PRE_VISUAL",clip
  assert detail["native_skeleton"].endswith("/SK_Luminous_Berserker_UNDERWEAR_OriginalPart_00_Skeleton.SK_Luminous_Berserker_UNDERWEAR_OriginalPart_00_Skeleton")
  assert detail["source_unity_path_id"]==template["source_unity_path_id"]
 else:continue
 ident=f"kk.berserker.native.UNDERWEAR.{clip}"
 if ident in ids:continue
 new=template.copy()
 new["motion_id"]=ident
 new["native_sequence"]=package+sequence+"."+sequence
 new["native_uasset_sha256"]=hashlib.sha256(path.read_bytes()).hexdigest()
 new["native_bone_tracks"]=detail["bone_tracks"]
 new["native_duration_seconds"]=detail["duration_s"]
 new["retarget"]="exact_original_source_to_independent_886_bone_koikatsu"
 new["outfit"]="UNDERWEAR"
 new["unmapped_original_rotation_bones"]=detail["unmapped_rotation_bones"]
 new["approved_for_gameplay"]=bool(template["category"]=="AdultInteraction" and template.get("approved_for_gameplay"))
 new["review_state"]=("USER_APPROVED_ORIGINAL_MATURE_NATIVE_RIG_DERIVATIVE" if new["approved_for_gameplay"]
                      else "SOURCE_NATIVE_DERIVATIVE_NEEDS_VISUAL_ACCEPTANCE")
 doc["motions"].append(new)
 ids.add(ident)
 added.append({"id":ident,"category":template["category"],
               "native_bones":detail["bone_tracks"],"approved":new["approved_for_gameplay"]})
report={"schema":"noworld.gate12.native.original.source.derivatives.v1",
 "added":added,"original_source_ids_retained":True,
 "no_other_character_clip_modified":True}
report_path.write_text(json.dumps(report,indent=2)+"\n",encoding="utf8")
if added:
 tmp=catalog.with_suffix(".new")
 tmp.write_text(json.dumps(doc,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
 assert len({x["motion_id"]for x in doc["motions"]})==len(doc["motions"])
 os.replace(tmp,catalog)
print("NATIVE_SOURCE_DERIVATIVES_REGISTERED",len(added),"MATURE_AUTHORIZED",sum(x["approved"]for x in added))
