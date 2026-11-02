"""Import one original Gate01 Alter body part (1130-bone rig) per UE session.

Preserves the six original skinned meshes and all source morph channels
without the 8GB-host-failing monolithic FBX import. No C++ compile/cook,
Base asset replacement or generated clothing.
"""
import unreal,json,hashlib,traceback,pathlib
PART=0  # production runner sets next segment only AFTER prior receipt is verified
ROOT=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
SOURCE_DIR=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Gate01\LuminousAlterCandidates_20261016\BLENDER_FRESH\CLOTHED\UNREAL_ORIGINAL_PARTS")
DOC=json.loads((SOURCE_DIR/"GATE12_ALTER_6_NATIVE_PARTS_EXPORT_20261021.json").read_text())
assert DOC["status"]=="EXACT_SIX_USER_CONFIRMED_ALTER_SKINNED_PARTS_FBX_EXPORTED"
ROW=DOC["parts"][PART]
SOURCE=pathlib.Path(ROW["fbx"])
DEST="/Game/NoWorld/Characters/Luminous/Alter/NativeOriginalClothed"
NAME=f"SK_Luminous_Alter_CLOTHED_OriginalPart_{PART:02d}"
RECEIPT=ROOT/"Saved/Gate12Readiness"/f"Gate12_Alter_UE_OriginalPart_{PART:02d}_20261021.json"
report={"part":PART,"original_mesh":ROW["mesh_original_name"],
"expected_vertices":ROW["vertices"],"expected_morphs":ROW["morph_targets"],
"source_fbx":str(SOURCE),"source_sha256":ROW["fbx_sha256"],
"status":"STARTED","source_modified":False,"base_mesh_replaced":False,
"game_compiled":False,"native_3d_source_is_approved_alter":True}
try:
    assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==ROW["fbx_sha256"]
    assert "OfflineGame_Gate12_Koikatsu_20261010" in unreal.Paths.get_project_file_path()
    target=DEST+"/"+NAME
    if unreal.EditorAssetLibrary.does_asset_exist(target):
        obj=unreal.load_asset(target)
        assert isinstance(obj,unreal.SkeletalMesh),"Existing asset at target is not native source"
        report["preexisting_preserved"]=True
    else:
        unreal.EditorAssetLibrary.make_directory(DEST)
        opts=unreal.FbxImportUI()
        opts.set_editor_property("automated_import_should_detect_type",False)
        opts.set_editor_property("mesh_type_to_import",unreal.FBXImportType.FBXIT_SKELETAL_MESH)
        opts.set_editor_property("import_as_skeletal",True)
        opts.set_editor_property("import_mesh",True)
        opts.set_editor_property("import_animations",False)
        opts.set_editor_property("import_materials",True)
        opts.set_editor_property("import_textures",True)
        opts.set_editor_property("create_physics_asset",False)
        skeleton=None
        if PART>0:
            skeleton=unreal.load_asset(DEST+"/SK_Luminous_Alter_CLOTHED_OriginalPart_00_Skeleton")
            assert isinstance(skeleton,unreal.Skeleton),"Original part 00 common rig unavailable"
        opts.set_editor_property("skeleton",skeleton)
        data=opts.get_editor_property("skeletal_mesh_import_data")
        data.set_editor_property("import_morph_targets",True)
        task=unreal.AssetImportTask()
        task.set_editor_property("filename",str(SOURCE))
        task.set_editor_property("destination_path",DEST)
        task.set_editor_property("destination_name",NAME)
        task.set_editor_property("automated",True)
        task.set_editor_property("save",True)
        task.set_editor_property("replace_existing",False)
        task.set_editor_property("options",opts)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        report["imported_object_paths"]=[str(x) for x in task.get_editor_property("imported_object_paths")]
        obj=unreal.load_asset(target)
        assert isinstance(obj,unreal.SkeletalMesh),"Part failed to import as Unreal SkeletalMesh"
    native_skeleton=obj.get_editor_property("skeleton")
    assert isinstance(native_skeleton,unreal.Skeleton)
    source_bones=len(native_skeleton.get_reference_pose().get_bone_names())
    report["native_skeleton_bones"]=source_bones
    report["native_mesh"]=obj.get_path_name()
    report["native_skeleton"]=native_skeleton.get_path_name()
    report["native_material_slots"]=len(obj.get_editor_property("materials"))
    assert source_bones>=900 and report["native_material_slots"]>0
    try:
        report["native_morph_targets"]=len(obj.get_editor_property("morph_targets"))
    except Exception:
        report["native_morph_read_unavailable"]=True
    assert unreal.EditorAssetLibrary.save_loaded_asset(obj)
    assert unreal.EditorAssetLibrary.save_loaded_asset(native_skeleton)
    report["status"]="IMPORTED_ORIGINAL_ALTER_SKINNED_PART"
except Exception:
    report["status"]="BLOCKED_PART_IMPORT_NO_QUALITY_SUBSTITUTION"
    report["error"]=traceback.format_exc()[-3200:]
finally:
    RECEIPT.parent.mkdir(parents=True,exist_ok=True)
    RECEIPT.write_text(json.dumps(report,indent=2)+"\n",encoding="utf8")
    unreal.log("GATE12_ALTER_NATIVE_PART_"+str(PART)+"="+report["status"])
