import unreal,json,traceback
from pathlib import Path
R=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
report=R/"Saved/Gate12Readiness/FirstWorldR1_MergedSource/lod_evidence.json"
d={"status":"STARTED","meshes":[],"errors":[]}
try:
    ed=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for name in ("SM_R1_ShrubSource","SM_R1_FirSource","SM_R1_OakSource"):
        mesh=unreal.EditorAssetLibrary.load_asset(
            "/Game/NoWorld/FirstWorld/R1/Modules/"+name)
        if not isinstance(mesh,unreal.StaticMesh):raise ValueError(name)
        if mesh.get_num_lods()>=4:continue
        ratios=(1.0,0.4,0.16,0.05)
        screens=(1.0,0.55,0.23,0.08)
        settings=[]
        for ratio,screen in zip(ratios,screens):
            item=unreal.EditorScriptingMeshReductionSettings()
            item.percent_triangles=ratio
            item.screen_size=screen
            settings.append(item)
        opt=unreal.EditorScriptingMeshReductionOptions()
        opt.reduction_settings=settings
        ed.set_lods(mesh,opt)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        d["meshes"].append({"mesh":name,"lods":mesh.get_num_lods()})
    d["status"]="NATIVE_SOURCE_MESH_LODS_WRITTEN_UNVERIFIED_IN_WORLD"
except Exception as exc:
    d["status"]="FAILED_OR_PARTIAL"
    d["errors"]=[str(exc),traceback.format_exc()[-4000:]]
finally:
    report.write_text(json.dumps(d,indent=2))
