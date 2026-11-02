"""Import authentic Koikatsu-derived split rig and material assets into Unreal.
One source-derived skeletal segment per Editor commandlet to fit 8GB device.
The User's source files and existing Base/Alter assets are untouched.
"""
import os,pathlib,json,hashlib,traceback,unreal
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
FORM=os.environ.get("G12_NATIVE_FORM","Berserker")
STATE=os.environ.get("G12_NATIVE_STATE","UNDERWEAR")
PART=int(os.environ["G12_NATIVE_PART"])
assert FORM in ("Awakened","Berserker","Grand")
assert STATE in ("CLOTHED","UNDERWEAR","NUDE") and 0<=PART<6
SOURCE=R/"SourceAssets/Gate12/LuminousForms20261023"/FORM/"CANDIDATE_NATIVE_FROM_EXISTING_KKBP_PMX"
PROOF=json.loads((SOURCE/"UNREAL_REAL_NATIVE_PARTS/GATE12_FORM_REAL_KKBP_SEGMENTED_FBX_RECEIPT.json").read_text(encoding="utf8"))
assert PROOF["status"]=="SIX_SOURCE_BACKED_NATIVE_FBX_PARTS_READY_FOR_UNREAL"
assert PROOF["character_form"]==FORM and len(PROOF["parts"])==6
ROW=PROOF["parts"][PART]
FBX=pathlib.Path(ROW["fbx"])
DEST=f"/Game/NoWorld/Characters/Luminous/{FORM}/NativeOriginal{STATE.title()}"
NAME=f"SK_Luminous_{FORM}_{STATE}_OriginalPart_{PART:02d}"
TARGET=DEST+"/"+NAME
RECEIPT=R/"Saved/Gate12Readiness"/f"GATE12_{FORM}_{STATE}_NATIVE_UE_PART_{PART:02d}_20261023.json"
doc={"schema":"noworld.gate12.luminous.real_kkbp_native_parts.v1",
 "form":FORM,"wardrobe":STATE,"part":PART,
 "source_fbx":str(FBX),"source_fbx_sha256":ROW["fbx_sha256"],
 "source_vertices":ROW["source_vertices"],
 "source_morph_targets":ROW["morphs"],"source_material_slots":ROW["material_slots"],
 "source_armature_bones":ROW["bones"],
 "source_card_to_pmx_hash_pairing":"UNVERIFIED",
 "source_files_modified":False,"status":"STARTED"}
try:
 assert R.name in unreal.Paths.get_project_file_path()
 assert FBX.is_file() and FBX.stat().st_size>100000
 h=hashlib.sha256()
 with FBX.open("rb") as fp:
  for chunk in iter(lambda:fp.read(4*1024*1024),b""):h.update(chunk)
 assert h.hexdigest()==ROW["fbx_sha256"]
 if unreal.EditorAssetLibrary.does_asset_exist(TARGET):
  obj=unreal.load_asset(TARGET)
  assert isinstance(obj,unreal.SkeletalMesh)
  doc["preexisting_preserved"]=True
 else:
  unreal.EditorAssetLibrary.make_directory(DEST)
  opts=unreal.FbxImportUI()
  for key,value in (("automated_import_should_detect_type",False),
     ("mesh_type_to_import",unreal.FBXImportType.FBXIT_SKELETAL_MESH),
     ("import_as_skeletal",True),("import_mesh",True),
     ("import_animations",False),("import_materials",True),
     ("import_textures",True),("create_physics_asset",False)):
   opts.set_editor_property(key,value)
  skeleton=None
  if PART>0:
   skeleton=unreal.load_asset(DEST+f"/SK_Luminous_{FORM}_{STATE}_OriginalPart_00_Skeleton")
   assert isinstance(skeleton,unreal.Skeleton),"Part00 source skeleton missing"
  opts.set_editor_property("skeleton",skeleton)
  opts.get_editor_property("skeletal_mesh_import_data").set_editor_property("import_morph_targets",True)
  task=unreal.AssetImportTask()
  for key,value in (("filename",str(FBX)),("destination_path",DEST),
      ("destination_name",NAME),("automated",True),("save",True),
      ("replace_existing",False),("options",opts)):
   task.set_editor_property(key,value)
  unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
  doc["returned_objects"]=[str(x)for x in task.get_editor_property("imported_object_paths")]
  obj=unreal.load_asset(TARGET)
  assert isinstance(obj,unreal.SkeletalMesh)
 skel=obj.get_editor_property("skeleton")
 assert isinstance(skel,unreal.Skeleton)
 doc["unreal_bones"]=len(skel.get_reference_pose().get_bone_names())
 doc["unreal_material_slots"]=len(obj.get_editor_property("materials"))
 doc["skeletal_mesh"]=obj.get_path_name()
 doc["skeleton"]=skel.get_path_name()
 assert doc["unreal_bones"]>=ROW["bones"] and doc["unreal_material_slots"]>0
 try:doc["unreal_morphs"]=len(obj.get_editor_property("morph_targets"))
 except:doc["unreal_morph_read_unsupported"]=True
 assert unreal.EditorAssetLibrary.save_loaded_asset(obj)
 assert unreal.EditorAssetLibrary.save_loaded_asset(skel)
 doc["status"]="REAL_SOURCE_NATIVE_UNREAL_SKELETAL_PART_IMPORTED"
except Exception:
 doc["status"]="IMPORT_FAILED_SOURCE_UNCHANGED"
 doc["trace"]=traceback.format_exc()[-4000:]
finally:
 RECEIPT.parent.mkdir(parents=True,exist_ok=True)
 RECEIPT.write_text(json.dumps(doc,indent=2)+"\n",encoding="utf8")
 unreal.log("G12_REAL_NATIVE_FORM_PART="+FORM+" "+STATE+" "+str(PART)+" "+doc["status"])
