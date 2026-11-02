"""Resolve Alter original source morph names versus UE morph targets
using Unreal FName's case-insensitive semantics, plus cross-part search.
This is read-only; never rename/deform the canonical character.
"""
import unreal,json,pathlib,unicodedata,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
ROOT=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Gate01\LuminousAlterCandidates_20261016\BLENDER_FRESH")
OUT=R/"SourceAssets/Gate12/LuminousAlter20261022/GATE12_ALTER_CASEFOLD_CROSSPART_MORPH_MATCHING_20261022.json"
report={"status":"STARTED","source_original_modified":False,
"native_ue_character_modified":False,"rule":"NFKC Unicode casefold and alphanumeric normalized matching",
"states":[]}
def fold(x):
 return unicodedata.normalize("NFKC",str(x)).casefold()
def norm(x):return "".join(c for c in fold(x) if c.isalnum())
try:
 for state in ("UNDERWEAR","NUDE"):
  audit=json.loads((ROOT/state/f"GATE12_ALTER_{state}_NATIVE_SHAPEDELTA_DETAILED_FIDELITY_AUDIT_20261022.json").read_text(encoding="utf8"))
  assert audit["status"]=="REAL_ORIGINAL_SHAPEKEY_DELTA_AUDIT_COMPLETED_NO_SOURCE_EDITS"
  all_norm={}
  per=[]
  for i in range(6):
   path=f"/Game/NoWorld/Characters/Luminous/Alter/NativeOriginal{state.title()}/SK_Luminous_Alter_{state}_OriginalPart_{i:02d}"
   mesh=unreal.load_asset(path)
   assert isinstance(mesh,unreal.SkeletalMesh)
   names=[str(m.get_name())for m in mesh.get_editor_property("morph_targets")]
   keys={norm(x)for x in names}
   all_norm[i]=keys
   src=audit["parts"][i]
   per.append({"part":i,"source":src["mesh"],"source_deforming":len(src["nonzero_deformation_shape_keys"]),
   "native_morphs":len(names),"casefold_exact_count":sum(fold(n)in {fold(q)for q in names}for n in src["nonzero_deformation_shape_keys"]),
   "same_part_alnum_match":sum(norm(n)in keys for n in src["nonzero_deformation_shape_keys"]),
   "source_shapes":src["nonzero_deformation_shape_keys"]})
  all_set=set().union(*all_norm.values())
  totals={"state":state,"parts":[]}
  for x in per:
   missing=[n for n in x["source_shapes"]if norm(n)not in all_norm[x["part"]]]
   other=[n for n in missing if norm(n)in all_set]
   truly=[n for n in missing if norm(n)not in all_set]
   totals["parts"].append({"part":x["part"],
     "source_deforming":x["source_deforming"],"native_morphs":x["native_morphs"],
     "casefold_exact_matches":x["casefold_exact_count"],"same_part_casefold_alnum_matches":x["same_part_alnum_match"],
     "matched_another_native_part":other,
     "unmatched_any_original_native_part":truly})
  totals["total_original_source_deforming"]=sum(x["source_deforming"]for x in per)
  totals["casefold_alnum_same_part_unmatched"]=sum(len(x["matched_another_native_part"])+len(x["unmatched_any_original_native_part"]) for x in totals["parts"])
  totals["cross_part_available"]=sum(len(x["matched_another_native_part"])for x in totals["parts"])
  totals["unmatched_in_all_native_parts"]=sum(len(x["unmatched_any_original_native_part"])for x in totals["parts"])
  assert totals["total_original_source_deforming"]==242
  report["states"].append(totals)
 report["status"]="EXACT_CASE_INSENSITIVE_CROSS_PART_MORPH_NAME_AUDIT_PERFORMED"
except Exception:
 report["status"]="CASE_INSENSITIVE_CROSS_PART_AUDIT_BLOCKED"
 report["error"]=traceback.format_exc()[-4100:]
finally:
 OUT.parent.mkdir(parents=True,exist_ok=True)
 assert not OUT.exists()
 OUT.write_text(json.dumps(report,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
 unreal.log("GATE12_ALTER_CASEFOLD_MORPH="+report["status"])
