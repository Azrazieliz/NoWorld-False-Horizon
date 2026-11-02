#!/usr/bin/env python3
"""5-star source-faithful Gate02 R1 original GLB → Unreal-ready GLB preparation.

Run with installed Blender (NEVER mutate the original source):
  blender -b --python Scripts/production/noworld_r1_prepare_glb.py -- \
    --source-root D:/NoWorld_Art_Production/NoWorld_FirstWorld_RegionKit_R1_20261019 \
    --output-root D:/UnrealProjects/NoWorld_R1_NativeImports_20261102/Prepared

Outputs one genuine static mesh for each original foliage prototype and twelve
spatial source-only sections of the original Eastern Market. All geometry is
preserved, including face materials, UV layers and source-local transforms.
This does not manufacture world objects, gameplay approvals or interaction contact.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

def sha256(path):
    h=hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda:f.read(1<<20),b""): h.update(chunk)
    return h.hexdigest()

def read_args():
    raw=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
    p=argparse.ArgumentParser()
    p.add_argument("--source-root",required=True)
    p.add_argument("--output-root",required=True)
    p.add_argument("--role",choices=["all","foliage","market"],default="all")
    return p.parse_args(raw)

def mesh_stats(items):
    return {
        "source_objects":len(items),
        "vertices":sum(len(o.data.vertices) for o in items),
        "polygons":sum(len(o.data.polygons) for o in items),
        "triangle_faces":sum(max(0,len(p.vertices)-2)
                             for o in items for p in o.data.polygons)
    }

def prepare(src,dst,name,parts):
    import bpy
    from mathutils import Matrix
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.gltf(filepath=str(src))
    meshes=[o for o in bpy.context.scene.objects if o.type=="MESH"]
    if not meshes:raise RuntimeError("Original has no real mesh: "+str(src))
    expected=mesh_stats(meshes)
    if len(meshes)<parts:
        raise RuntimeError("Insufficient original meshes for requested spatial sections")
    # Use the actual imported world-space bounds; section order is stable and
    # local-origin meshes are not re-placed at arbitrary derived coordinates.
    for o in meshes:
        mat=o.matrix_world.copy()
        o.data=o.data.copy()
        o.data.transform(mat)
        o.parent=None
        o.matrix_world=Matrix.Identity(4)
    ordered=sorted(meshes,key=lambda o:(
        round(o.location.x,4),round(o.location.y,4),o.name))
    # The location was baked to vertices, so use real world-space bounding
    # box centers for a spatial stable ordering instead of the reset origin.
    ordered.sort(key=lambda o:(
        round(sum(v.co.x for v in o.data.vertices)/max(1,len(o.data.vertices)),3),
        round(sum(v.co.y for v in o.data.vertices)/max(1,len(o.data.vertices)),3),
        o.name))
    groups=[ordered[i*len(ordered)//parts:(i+1)*len(ordered)//parts]
            for i in range(parts)]
    outputs=[]
    for section,group in enumerate(groups):
        if not group:raise RuntimeError("Empty authentic section")
        bpy.ops.object.select_all(action="DESELECT")
        for o in group:o.select_set(True)
        bpy.context.view_layer.objects.active=group[0]
        if len(group)>1:bpy.ops.object.join()
        result=bpy.context.view_layer.objects.active
        stats=mesh_stats([result])
        original=mesh_stats(group) if len(group)==1 else None
        target=dst/(("SM_R1_EasternMarket_SourceSection_%02d"%section)
                    if parts>1 else name)
        target=target.with_suffix(".glb")
        bpy.ops.object.select_all(action="DESELECT")
        result.select_set(True)
        bpy.context.view_layer.objects.active=result
        bpy.ops.export_scene.gltf(filepath=str(target),
                                  export_format="GLB",use_selection=True)
        outputs.append({"section":section,"file":str(target),"sha256":sha256(target),
                        "bytes":target.stat().st_size,**stats})
        result.select_set(False)
    total={k:sum(x[k] for x in outputs)
           for k in ("vertices","polygons","triangle_faces")}
    for k,v in total.items():
        if v!=expected[k]:raise RuntimeError(
            "Source geometry changed: "+k+" expected="+str(expected[k])+
            " output="+str(v))
    return {"source":str(src),"source_sha256":sha256(src),"source_stats":expected,
            "output_stats":total,"outputs":outputs,"geometry_preserved":True}

def main():
    args=read_args()
    source=Path(args.source_root).resolve(strict=True)
    output=Path(args.output_root).resolve()
    if source==output or source in output.parents:
        raise ValueError("Output cannot overwrite protected source root")
    output.mkdir(parents=True,exist_ok=True)
    targets=[]
    if args.role in ("all","foliage"):
        targets.extend([(f"modules/{name}.glb",f"SM_R1_{native}Source",1)
          for name,native in (("fir_sapling","Fir"),("oak_tree","Oak"),("shrub","Shrub"))])
    if args.role in ("all","market"):
        targets.append(("anchors/eastern_market_town.glb","EasternMarket",12))
    receipts=[]
    for relative,name,count in targets:
        src=source/relative
        if not src.is_file():raise FileNotFoundError(src)
        dst=output/("market" if count>1 else "modules")
        dst.mkdir(parents=True,exist_ok=True)
        receipts.append(prepare(src,dst,name,count))
    receipt={"schema":"noworld.r1.native_source_preparation.v1",
             "accepted_by_owner":False,"native_unreal_verified":False,
             "source_reused_without_proxy":True,"sources":receipts}
    (output/"source_receipt.json").write_text(
        json.dumps(receipt,indent=2)+"\n",encoding="utf-8")
    print("ORIGINAL_R1_GLBS_PREPARED",len(receipts),
          sum(len(s["outputs"]) for s in receipts),
          str(output/"source_receipt.json"))
if __name__=="__main__": main()
