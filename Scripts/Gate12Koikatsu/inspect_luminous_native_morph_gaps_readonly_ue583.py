"""Gate12: read-only UE native morph inspection of preserved Luminous parts 7 and 16."""
import pathlib, json, re, collections, time, unreal
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu\LuminousV3\CLOTHED")
source=json.loads((R/"source_morph_names_displacement_indices_007_016.json").read_text(encoding="utf8"))
normal=lambda s:re.sub(r"[^a-z0-9]", "",str(s).casefold())
out={"status":"START","inspector":"COLD_LOAD_EXISTING_ASSETS_NO_REIMPORT","parts":[]}
for case in source["cases"]:
  index=int(case["index"])
  mesh=int(case["mesh_index"]);prim=int(case["primitive_index"])
  path=("/Game/Experimental/LuminousValentineV3/CLOTHED/Parts/"
    "Mesh%03dPrim%03d/Mesh_%02d_Prim_%02d/SkeletalMeshes/"
    "Mesh_%02d_Prim_%02d.Mesh_%02d_Prim_%02d" %
    (mesh,prim,mesh,prim,mesh,prim,mesh,prim))
  obj=unreal.EditorAssetLibrary.load_asset(path)
  if not isinstance(obj,unreal.SkeletalMesh):
    raise RuntimeError("Preserved native skeletal mesh missing at "+path)
  native=[o.get_name() for o in obj.get_editor_property("morph_targets")]
  counts=collections.Counter(normal(x) for x in native)
  matching=[];missing=[]
  for target in case["source_nonzero_targets"]:
    record=dict(target)
    record["normalized_name"]=normal(target["name"])
    record["native_normalized_name_match"]=counts.get(record["normalized_name"],0)>0
    (matching if record["native_normalized_name_match"] else missing).append(record)
  out["parts"].append({"index":index,"asset_path":path,"native_count":len(native),
    "native_names":native,"source_nonzero_count":len(case["source_nonzero_targets"]),
    "name_matches":len(matching),"name_absences":len(missing),
    "missing_named_source_targets":missing,"matched_source_targets":matching})
  unreal.log("G12_LUMINOUS_MORPH_PROBE %s Native %s NameMatches %s SourceAbsences %s" %
    (index,len(native),len(matching),len(missing)))
out["status"]="PASS_READONLY_NATIVE_MORPH_NAME_DIAGNOSTIC_PENDING_REPAIR"
out["created_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())
(R/"native_morph_names_displacement_indices_007_016_ue583.json").write_text(
 json.dumps(out,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
