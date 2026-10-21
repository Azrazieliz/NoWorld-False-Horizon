"""Native intake for existing Gate02 authored R1 anchor GLB->editable FBX.
Do not replace any protected imports or declare the source release-ready."""
import unreal,json,hashlib,traceback
from pathlib import Path
P=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
INPUT=P/"Saved/Gate12Readiness/FirstWorldR1_MergedSource"
OUT=INPUT/"ue_native_anchor_import.json"
targets=[("southern_farmland_windmill","SM_R1_SouthernFarmland"),
         ("watchwood_hamlet","SM_R1_WatchwoodHamlet")]
R={"status":"STARTED","assets":[],"errors":[]}
def save():OUT.write_text(json.dumps(R,indent=2),encoding="utf8")
save()
try:
    assert "OfflineGame_Gate12_Koikatsu_20261010" in unreal.Paths.get_project_file_path()
    tools=unreal.AssetToolsHelpers.get_asset_tools()
    for basename,assetname in targets:
        source=INPUT/(basename+"_NativeUE_Source.fbx")
        assert source.is_file(),source
        folder="/Game/NoWorld/FirstWorld/R1/Anchors"
        objpath=folder+"/"+assetname
        if unreal.EditorAssetLibrary.does_asset_exist(objpath):
            R["assets"].append({"asset":objpath,"status":"PROTECTED_EXISTING"})
            continue
        opts=unreal.FbxImportUI()
        opts.set_editor_property("import_mesh",True)
        opts.set_editor_property("import_as_skeletal",False)
        opts.set_editor_property("import_materials",True)
        opts.set_editor_property("import_textures",True)
        opts.static_mesh_import_data.set_editor_property("combine_meshes",True)
        opts.static_mesh_import_data.set_editor_property("auto_generate_collision",True)
        task=unreal.AssetImportTask()
        task.set_editor_property("filename",str(source))
        task.set_editor_property("destination_path",folder)
        task.set_editor_property("destination_name",assetname)
        task.set_editor_property("automated",True)
        task.set_editor_property("save",True)
        task.set_editor_property("replace_existing",False)
        task.set_editor_property("async_",False)
        task.set_editor_property("options",opts)
        tools.import_asset_tasks([task])
        mesh=unreal.EditorAssetLibrary.load_asset(objpath)
        if not mesh or not isinstance(mesh,unreal.StaticMesh):
            raise RuntimeError("Source FBX native StaticMesh not created: "+objpath)
        R["assets"].append({
            "asset":mesh.get_path_name(),
            "source_name":basename,
            "source_sha256":json.loads((INPUT/(basename+"_conversion.json")).read_text())["source_sha256"],
            "fbx_sha256":hashlib.sha256(source.read_bytes()).hexdigest(),
            "status":"REAL_SOURCE_STATICMESH_IMPORTED_NOT_ART_APPROVED"
        })
        save()
    R["status"]="GATE02_ANCHORS_NATIVE_IMPORTED_NOT_MOBILE_QUALIFIED"
except Exception as e:
    R["status"]="PARTIAL_OR_FAILED"
    R["errors"]=[str(e),traceback.format_exc()[-6500:]]
finally:
    save()
    unreal.log("GATE12_R1_ANCHORS="+R["status"])
