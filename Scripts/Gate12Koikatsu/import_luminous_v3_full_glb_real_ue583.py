"""Luminous approved Party source: first direct full-fidelity GLB Unreal skeletal import.
Runs isolated 5.8.3 UnrealEditor-Cmd; asset results do not imply Android PASS.
No frozen files modified; no proxy mesh or texture downscaling.
"""
import os,pathlib,hashlib,json,traceback,time,struct
import unreal
ROOT=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
SOURCE=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011")
STATE=os.environ.get("G12_LUMINOUS_EXPORT_STATE","")
if STATE not in ("CLOTHED","UNDERWEAR","NUDE"):raise RuntimeError("No exact source state selected")
IN_DIR=SOURCE/"BLENDER_FRESH"/STATE
GLB=IN_DIR/("LUMINOUS_V3_"+STATE+"_skinned.glb")
AUDIT=IN_DIR/"GATE12_GLTF_REAL_GEOMETRY_MORPH_AUDIT.json"
QA=ROOT/"Saved/Gate12Koikatsu/LuminousV3"/STATE
DEST="/Game/Experimental/LuminousValentineV3/"+STATE+"/RealPartyImported"
QA.mkdir(parents=True,exist_ok=True)
RECEIPT=QA/"native_full_glb_ue583_import_receipt.json"
report={"schema":"GATE12_LUMINOUS_V3_DIRECT_REAL_SOURCE_UE583_IMPORT_1",
  "character_identity":"LUMINOUS_VALENTINE_V3","state":STATE,"status":"STARTED",
  "native_UE_material_and_morph_appearance":"NOT_PHYSICALLY_TESTED",
  "gate11_approval":False,"original_game_installation_untouched":True,
  "native_android_package":"NOT_BUILT","warnings":[]}
def save():
 report["utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())
 RECEIPT.write_text(json.dumps(report,indent=2,default=str)+"\n",encoding="utf8")
save()
try:
 if "OfflineGame_Gate12_Koikatsu_20261010" not in str(unreal.Paths.get_project_file_path()):
  raise RuntimeError("Wrong Unreal project, isolated import only")
 src=json.loads(AUDIT.read_text(encoding="utf-8-sig"))
 if not src["status"].startswith("PASS_REAL_SOURCE") or src["armature_skin_count"]!=1:
  raise RuntimeError("Missing source GLB skin/material/morph audit")
 with GLB.open("rb") as f:
  sha=hashlib.file_digest(f,"sha256").hexdigest()
 if sha!=src["glb_sha256"]:raise RuntimeError("GLB input hash mismatch")
 report["input_glb_sha256"]=sha
 report["source_part_count"]=src["primitive_count"]
 report["source_morph_links"]=src["morph_target_references"]
 report["source_images"]=src["embedded_image_count"]
 report["source_bones"]=src["joint_count_per_skin"]
 save()
 if unreal.EditorAssetLibrary.does_directory_exist(DEST):
  raise RuntimeError("Target content already exists; refuse to overwrite actual content")
 task=unreal.AssetImportTask()
 task.filename=str(GLB)
 task.destination_path=DEST
 task.automated=True
 task.async_=False
 task.save=True
 task.replace_existing=False
 # Interchange glTF source pipeline; attaching FbxImportUI is forbidden.
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
 paths=list(unreal.EditorAssetLibrary.list_assets(DEST,recursive=True,include_folder=False))
 assets=[];meshes=[];morph_targets=0;materials=0;textures=0
 for path in paths:
  asset=unreal.EditorAssetLibrary.load_asset(path)
  if asset is None:
   report["warnings"].append("Unloaded asset "+str(path))
   continue
  typ=asset.get_class().get_name()
  rec={"path":str(path),"asset_type":typ}
  assets.append(rec)
  if isinstance(asset,unreal.SkeletalMesh):
   sk=asset.get_editor_property("skeleton")
   bones=len(sk.get_editor_property("bone_tree")) if sk else 0
   morphs=len(asset.get_editor_property("morph_targets"))
   morph_targets+=morphs
   rec.update({"skeleton":sk.get_path_name() if sk else None,
       "skeleton_bone_entries":bones,"morph_count":morphs})
   meshes.append(rec)
  elif typ in ("Material","MaterialInstanceConstant"):materials+=1
  elif typ in ("Texture2D","Texture"):textures+=1
 report["assets"]=assets
 report["asset_total"]=len(assets)
 report["skeletal_mesh_count"]=len(meshes)
 report["total_morph_asset_refs"]=morph_targets
 report["material_asset_count"]=materials
 report["texture_asset_count"]=textures
 save()
 if not meshes:raise RuntimeError("Interchange imported no native SkeletalMesh from real Party GLB")
 if max(x["skeleton_bone_entries"] for x in meshes)<80:
  raise RuntimeError("Unexpected native UE skeleton bone count")
 if morph_targets==0:
  raise RuntimeError("Interchange lost all facial/body morph targets")
 if materials<1 or textures<1:
  raise RuntimeError("Source materials or textures missing native import")
 report["status"]="PASS_REAL_NATIVE_UNREAL_SKELETAL_MORPH_TEXTURE_ASSETS_NOT_GAMEPLAY"
except Exception as exc:
 report["status"]="FAIL_OR_PARTIAL_REAL_NATIVE_UNREAL_IMPORT"
 report["error"]=repr(exc)
 report["traceback"]=traceback.format_exc()[-5000:]
finally:
 save()
 unreal.log("GATE12_LUMINOUS_V3_"+STATE+"="+report["status"])
