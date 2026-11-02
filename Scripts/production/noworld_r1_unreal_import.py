#!/usr/bin/env python3
"""5-star real UE5.8 R1 native static-mesh import (run inside UnrealEditor).

Only imports source-authenticated geometries prepared from the frozen 2026-10-19
original GLB kit. Never creates low-poly stand-ins or copies user-owned game
packages from another project. Exact native /Game paths used by the existing
AOGStartingRegionGenerator are checked before writing receipts.

Native invocation:
  UnrealEditor-Cmd.exe <isolated_project.uproject> -run=pythonscript \
    -script=Scripts/production/noworld_r1_unreal_import.py -unattended -nop4
Environment: NOWORLD_R1_PREPARED=<.../Prepared>; NOWORLD_R1_RECEIPT=<...json>
"""
import hashlib
import json
import os
from pathlib import Path
import traceback
import unreal

root=Path(os.environ["NOWORLD_R1_PREPARED"]).resolve(strict=True)
receipt=Path(os.environ["NOWORLD_R1_RECEIPT"]).resolve()
plan=json.loads((root/"source_receipt.json").read_text("utf-8"))
if plan.get("schema")!="noworld.r1.native_source_preparation.v1" or not plan.get("source_reused_without_proxy"):
    raise ValueError("Original native R1 source provenance absent")
if not plan.get("sources"):raise ValueError("R1 original meshes not prepared")
assetlib=unreal.EditorAssetLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
summary={"schema":"noworld.r1.unreal_static_mesh_import.v1",
         "accepted_by_owner":False,"android_verified":False,
         "status":"STARTED","packages":[],"source_sha256":[]}
def sha256(path):
    h=hashlib.sha256()
    with open(path,"rb") as f:
        for b in iter(lambda:f.read(1<<20),b""):h.update(b)
    return h.hexdigest()

def import_one(source,expected_name):
    file=Path(source["file"])
    if not file.is_file():file=root/file.name
    file=file.resolve(strict=True)
    if file.suffix.lower()!=".glb" or sha256(file)!=source["sha256"] or not source.get("triangle_faces"):
        raise ValueError("Imported GLB not authentic or geometric: "+str(file))
    dest="/Game/NoWorld/FirstWorld/R1/"
    if expected_name.startswith("SM_R1_EasternMarket_"):
        dest+="Anchors/Market/"
    else:dest+="Modules/"
    object_path=dest+expected_name+"."+expected_name
    if assetlib.does_asset_exist(object_path):
        raise FileExistsError("Refusing native source overwrite: "+object_path)
    staging=dest+"OriginalSource_"+expected_name
    if assetlib.does_directory_exist(staging) and assetlib.list_assets(staging,True,False):
        raise FileExistsError("Existing imported original assets in staging: "+staging)
    task=unreal.AssetImportTask()
    task.set_editor_property("filename",str(file))
    task.set_editor_property("destination_path",staging)
    task.set_editor_property("automated",True)
    task.set_editor_property("replace_existing",False)
    task.set_editor_property("save",True)
    tools.import_asset_tasks([task])
    all_assets=assetlib.list_assets(staging,recursive=True,include_folder=False)
    mesh_paths=[]
    for asset in all_assets:
        item=assetlib.load_asset(asset)
        if item and item.get_class().get_name()=="StaticMesh":
            mesh_paths.append(str(item.get_path_name()))
    if len(mesh_paths)!=1:
        raise RuntimeError("Expected one exact joined source StaticMesh, got "+
                           str(len(mesh_paths))+" from "+str(file)+" paths="+str(mesh_paths))
    if not assetlib.rename_asset(mesh_paths[0],object_path):
        raise RuntimeError("Cannot place real source mesh at canonical runtime path: "+object_path)
    if not assetlib.save_asset(object_path):
        raise RuntimeError("Real native mesh did not save: "+object_path)
    if not assetlib.does_asset_exist(object_path):
        raise RuntimeError("Native asset disappeared: "+object_path)
    return {"native_object":object_path,"source_glb":str(file),
            "source_sha256":source["sha256"],
            "native_class":"StaticMesh","original_triangles":source["triangle_faces"],
            "source_bytes":source["bytes"],
            "asset_paths":[str(p) for p in assetlib.list_assets(staging,True,False)]}

try:
    # Small authenticated batches are required on the real 8 GB Unreal
    # worker. Never initialize every original mesh/material at once.
    allowed={x.strip() for x in os.environ.get("NOWORLD_R1_ONLY","").split(",") if x.strip()}
    for item in plan["sources"]:
        for source in item["outputs"]:
            name=Path(source["file"]).stem
            if allowed and name not in allowed:continue
            if item["source_sha256"] not in summary["source_sha256"]:
                summary["source_sha256"].append(item["source_sha256"])
            summary["packages"].append(import_one(source,name))
            receipt.parent.mkdir(parents=True,exist_ok=True)
            summary["status"]="PARTIAL"
            receipt.write_text(json.dumps(summary,indent=2)+"\n","utf-8")
    if not summary["packages"]:
        raise ValueError("No authenticated original assets matched selected batch")
    summary["status"]="NATIVE_IMPORTED_SOURCE_UNVERIFIED_ON_DEVICE"
except Exception as exc:
    summary["status"]="FAILED"
    summary["error"]=repr(exc)
    summary["traceback"]=traceback.format_exc()[-3500:]
    unreal.log_error(summary["traceback"])
    raise
finally:
    receipt.parent.mkdir(parents=True,exist_ok=True)
    receipt.write_text(json.dumps(summary,indent=2)+"\n","utf-8")
print("NOWORLD_R1_NATIVE_IMPORT",summary["status"],len(summary["packages"]))
