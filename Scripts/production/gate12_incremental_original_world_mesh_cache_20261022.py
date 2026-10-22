"""Warm ONE approved first-World mesh per UE session, never its entire world.
Asset data and source are read-only. Nothing is saved or reauthored.
Set G12_MESH_INDEX in the process environment. No duplicated low-poly geometry.
"""
import unreal,os,json,pathlib,time,traceback
ROOT=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
MESHES=[
 "/Game/NoWorld/FirstWorld/R1/Anchors/SM_R1_WatchwoodHamlet",
 "/Game/NoWorld/FirstWorld/R1/Anchors/SM_R1_RoyalCitadel_SmoothedTriangulated",
 "/Game/NoWorld/FirstWorld/R1/Anchors/SM_R1_SouthernFarmland",
 "/Game/NoWorld/FirstWorld/R1/Modules/SM_R1_FirSource",
 "/Game/NoWorld/FirstWorld/R1/Modules/SM_R1_OakSource",
 "/Game/NoWorld/FirstWorld/R1/Modules/SM_R1_ShrubSource",
 "/Game/Experimental/FirstWorldR1/MossRock/moss_rock/StaticMeshes/rock_moss_set_02_rock07_001",
 "/Game/Experimental/FirstWorldR1/Roads/ROAD___Eastern_caravan_road",
 "/Game/Experimental/FirstWorldR1/Roads/ROAD___Market_north_street",
 "/Game/Experimental/FirstWorldR1/Roads/ROAD___Royal_pilgrimage___east_spine",
 "/Game/Experimental/FirstWorldR1/Roads/ROAD___South_rural_return",
 "/Game/Experimental/FirstWorldR1/Roads/ROAD___Southern_mill_and_agriculture_road",
 "/Game/Experimental/FirstWorldR1/Roads/ROAD___Village_woodward_pathway"]
idx=int(os.environ.get("G12_MESH_INDEX","0"))
assert 0<=idx<len(MESHES)
path=MESHES[idx]
receipt=ROOT/"Saved/Gate12Readiness"/("Gate12_MeshCache_{:02d}_20261022.json".format(idx))
result={"schema":"noworld.gate12.firstworld.readonly.meshddc.v1",
 "status":"STARTED","index":idx,"source":path,"source_asset_modified":False,
 "approved_quality_retained":True,"new_mesh_generated":False,
 "no_map_saved":True}
try:
 start=time.monotonic()
 assert unreal.EditorAssetLibrary.does_asset_exist(path),path
 world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
 unreal.SystemLibrary.execute_console_command(world,
     "Editor.AsyncStaticMeshCompilationMaxConcurrency 1")
 mesh=unreal.EditorAssetLibrary.load_asset(path)
 assert isinstance(mesh,unreal.StaticMesh),"Real first World source is not a StaticMesh"
 # Unreal async mesh data initially reads as zero. Block on the actual
 # static-mesh compiler rather than mistaking pending compilation for failure.
 unreal.SystemLibrary.execute_console_command(world,
     "Editor.AsyncStaticMeshCompilationFinishAll")
 result["asset"]=mesh.get_path_name()
 result["lod0_vertices"]=mesh.get_num_vertices(0)
 assert result["lod0_vertices"]>0
 result["elapsed_seconds"]=round(time.monotonic()-start,2)
 result["status"]="ORIGINAL_MESH_LOADED_LOD_QUERIED_NO_ASSET_OVERWRITE"
except Exception:
 result["status"]="SOURCE_MESH_CACHE_STEP_FAILED"
 result["error"]=traceback.format_exc()[-2000:]
finally:
 receipt.parent.mkdir(parents=True,exist_ok=True)
 receipt.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
 unreal.log("G12_FIRSTWORLD_ONE_MESH="+result["status"])
