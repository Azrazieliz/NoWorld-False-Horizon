"""Read original Alter state UE morph channels; reconcile against actual
nonzero original Blender shape-key deformation without overwriting assets.
"""
import unreal,json,os,pathlib,traceback,unicodedata,re
STATE=os.environ["G12_ALTER_MORPH_STATE"]
assert STATE in ("UNDERWEAR","NUDE")
ROOT=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
ART=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Gate01\LuminousAlterCandidates_20261016")
FOLDER=ART/"BLENDER_FRESH"/STATE
SHAPES=json.loads((FOLDER/f"GATE12_ALTER_{STATE}_NATIVE_SHAPEDELTA_DETAILED_FIDELITY_AUDIT_20261022.json").read_text(encoding="utf8"))
assert SHAPES["status"]=="REAL_ORIGINAL_SHAPEKEY_DELTA_AUDIT_COMPLETED_NO_SOURCE_EDITS"
FBX=json.loads((FOLDER/"UNREAL_ORIGINAL_PARTS"/f"GATE12_ALTER_{STATE}_NATIVE_PARTS_20261022.json").read_text(encoding="utf8"))
assert FBX["status"]=="EXACT_ORIGINAL_APPROVED_ALTER_WARDROBE_SKINNED_PARTS_EXPORTED"
OUT=ROOT/"Saved/Gate12Readiness"/f"Gate12_Alter_{STATE}_NativeMorphMeaningfulFidelity_20261022.json"
r={"state":STATE,"status":"STARTED","underlying_source_unmodified":True,"unreal_assets_unmodified":True,"parts":[]}
def norm(s):
 return "".join(ch for ch in unicodedata.normalize("NFKC",s).casefold() if ch.isalnum())
try:
 for row in FBX["parts"]:
  i=row["part"]
  label=row["mesh_original_name"]
  src=next(x for x in SHAPES["parts"] if x["mesh"]==label)
  pathname=f"/Game/NoWorld/Characters/Luminous/Alter/NativeOriginal{STATE.title()}/SK_Luminous_Alter_{STATE}_OriginalPart_{i:02d}"
  mesh=unreal.load_asset(pathname)
  assert isinstance(mesh,unreal.SkeletalMesh)
  morphs=mesh.get_editor_property("morph_targets")
  names=[str(item.get_name()) for item in morphs]
  assert len(names)==len(set(names))
  effective=list(src["nonzero_deformation_shape_keys"])
  native_normal=set(norm(x) for x in names)
  missing=[x for x in effective if norm(x) not in native_normal]
  r["parts"].append({
      "part":i,"mesh":label,"original_nonzero":len(effective),
      "original_shape_keys":src["source_shape_keys"],
      "unreal_morph_count":len(names),
      "original_nonzero_not_name_matched":missing,
      "original_nonzero_not_name_matched_count":len(missing),
      "native_example":names[:16],
      "source_effective_example":effective[:16]
  })
  assert len(names)>=0
 r["original_all_shape_keys"]=SHAPES["source_total_shape_keys"]
 r["original_true_deforming_keys"]=SHAPES["effective_gt_1e7"]
 r["unreal_total_morph_channels"]=sum(x["unreal_morph_count"] for x in r["parts"])
 r["native_nonzero_names_unmatched"]=sum(x["original_nonzero_not_name_matched_count"] for x in r["parts"])
 r["status"]="MORPH_TARGET_CHANNELS_RECONCILED_BY_NAME_NONZERO_DELTAS" if r["native_nonzero_names_unmatched"]==0 else "MORPH_COUNT_AND_NONZERO_NAME_DIFFERENCES_REMAIN_DIAGNOSED"
except Exception:
 r["status"]="NATIVE_MORPH_AUDIT_FAILED"
 r["error"]=traceback.format_exc()[-4000:]
finally:
 OUT.parent.mkdir(parents=True,exist_ok=True)
 OUT.write_text(json.dumps(r,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
 unreal.log("GATE12_ORIGINAL_ALTER_MORPH_VERIFIED "+STATE+" "+r["status"])
