"""Read verified 100%-semantic-equivalent GLB copies with glTF sparse accessors expanded to dense.
New staging UAssets only, production morph gaps not overwritten/accepted.
"""
import unreal,pathlib,json,hashlib,time,traceback,re
W=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
Q=W/"Saved/Gate12Koikatsu/LuminousV3/CLOTHED"
S=W/"Saved/Gate12Koikatsu/LuminousV3/LosslessSources/MorphSparseExpandedProbe/CLOTHED"
m=json.loads((S/"lossless_sparse_expansion_manifest.json").read_text("utf-8-sig"))
assert m["status"]=="PASS_LOSSLESS_SPARSE_ACCESSOR_EXPANSION_DIAGNOSTIC_SOURCES"
ROOT="/Game/Experimental/LuminousValentineV3/MorphDenseSourceProbe_20261011"
r={"status":"START","input_manifest":str(S/"lossless_sparse_expansion_manifest.json"),
    "semantic_byte_equal":True,"original_parts_modified":False,"parts":[]}
out=Q/"morph_dense_source_native_probe_007_016_ue583.json"
def save():out.write_text(json.dumps(r,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
save()
try:
 for part in m["parts"]:
  i=int(part["index"])
  source=pathlib.Path(part["output_path"])
  if hashlib.sha256(source.read_bytes()).hexdigest()!=part["staging_glb_sha256"]:raise RuntimeError("Staging source hash mismatch")
  dest=ROOT+"/Index%03d"%i
  if unreal.EditorAssetLibrary.does_directory_exist(dest):raise RuntimeError("Never overwrite existing sparse-expanded native staging "+dest)
  task=unreal.AssetImportTask()
  task.filename=str(source);task.destination_path=dest
  task.automated=True;task.async_=False;task.replace_existing=False;task.save=True
  unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
  sk=[]
  for n in unreal.EditorAssetLibrary.list_assets(dest,recursive=True,include_folder=False):
   obj=unreal.EditorAssetLibrary.load_asset(n)
   if isinstance(obj,unreal.SkeletalMesh):
    sk.append({"path":str(n),"morph_names":[q.get_name() for q in obj.get_editor_property("morph_targets")],
       "morph_count":len(obj.get_editor_property("morph_targets")),
       "bones":len(obj.get_editor_property("skeleton").get_editor_property("bone_tree"))})
  target=91 if i==7 else 15
  quality="PASS_ALL_SEMANTIC_MORPH_TARGETS" if len(sk)==1 and sk[0]["morph_count"]==target and sk[0]["bones"]==975 else "FAIL_MORPH_IMPORT_INCOMPLETE"
  r["parts"].append({"index":i,"target_effective_source_morphs":target,"stage_native_skeletal_meshes":sk,
    "stage_asset_count":len(unreal.EditorAssetLibrary.list_assets(dest,recursive=True,include_folder=False)),
    "stage_path":dest,"status":quality,"source_sha256":part["source_sha256"],
    "reencoded_sha256":part["staging_glb_sha256"],"geometry_byte_equivalent":True})
  save()
 r["status"]="PASS_SEMANTIC_MORPH_IMPORT_BOTH" if all(x["status"].startswith("PASS") for x in r["parts"]) else "FAIL_SOME_MORPH_STILL_MISSING"
except Exception as e:
 r["status"]="FAIL_STAGING_NATIVE_PROBE"
 r["exception"]=str(e)
 r["trace"]=traceback.format_exc()[-4500:]
finally:
 r["updated_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime());save()
 unreal.log("G12_MORPH_SPARSE_TO_DENSE_PROBE="+r["status"])
