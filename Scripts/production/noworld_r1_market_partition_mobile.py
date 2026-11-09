#!/usr/bin/env python3
"""Prepare mobile-static native-compatible LOD from ONE frozen original
Eastern Market source partition, never a substitute; isolated Blender worker.

The original 12 full-detail partitions and frozen original source hash are
preserved. The output is an explicitly marked geometrically derived LOD.
Run one section per Blender process on the 8 GiB worker to bound RAM.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import bpy

def sha(path):
 h=hashlib.sha256()
 with open(path,"rb") as stream:
  for chunk in iter(lambda: stream.read(2**20),b""): h.update(chunk)
 return h.hexdigest()

def stats(objects):
 return {"vertices":sum(len(o.data.vertices) for o in objects),
         "triangles":sum(sum(max(0,len(p.vertices)-2) for p in o.data.polygons)
                         for o in objects),
         "meshes":len(objects)}

def main():
 raw=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
 p=argparse.ArgumentParser()
 p.add_argument("--master",type=Path,required=True)
 p.add_argument("--section",type=int,required=True)
 p.add_argument("--output-root",type=Path,required=True)
 p.add_argument("--ratio",type=float,default=0.12)
 a=p.parse_args(raw)
 if a.section<0 or a.section>11 or a.ratio<=0 or a.ratio>=1:
  raise ValueError("Only twelve real original partitions and genuine reduced LOD are valid")
 master=a.master.resolve(strict=True)
 manifest=json.loads((master.parent.parent/"source_receipt.json").read_text("utf-8"))
 if manifest.get("schema")!="noworld.r1.native_source_preparation.v1" or len(manifest["sources"])!=1:
  raise RuntimeError("Source-authored 12-way market receipt required")
 original=manifest["sources"][0]
 if len(original["outputs"])!=12 or original["source_sha256"]!="a6373a82b629b667714f4e39b741ff66450d8ece338bf5eaf72431148d489e3c":
  raise RuntimeError("Frozen original Eastern Market provenance mismatch")
 entry=original["outputs"][a.section]
 if master!=Path(entry["file"]).resolve(strict=True) or sha(master)!=entry["sha256"]:
  raise RuntimeError("Exact original full-detail section checksum mismatch")
 if a.output_root.resolve()==master.parent.resolve():
  raise ValueError("Original source must not be overwritten")
 bpy.ops.object.select_all(action="SELECT")
 bpy.ops.object.delete(use_global=False)
 bpy.ops.import_scene.gltf(filepath=str(master))
 meshes=[o for o in bpy.context.scene.objects if o.type=="MESH"]
 before=stats(meshes)
 if before["triangles"]!=entry["triangle_faces"]:
  raise RuntimeError("Imported source triangles differ from approved source receipt")
 for ob in meshes:
  if len(ob.data.polygons)<1000:continue
  bpy.ops.object.select_all(action="DESELECT")
  ob.select_set(True)
  bpy.context.view_layer.objects.active=ob
  mod=ob.modifiers.new("NoWorld_SourceDerived_MobileLOD","DECIMATE")
  mod.ratio=a.ratio
  bpy.ops.object.modifier_apply(modifier=mod.name)
 after=stats(meshes)
 if not 0<after["triangles"]<before["triangles"]:
  raise RuntimeError("Decimator did not produce true original geometry reduction")
 dst=a.output_root.resolve()/"market"
 dst.mkdir(parents=True,exist_ok=True)
 out=dst/master.name
 bpy.ops.object.select_all(action="DESELECT")
 for ob in meshes:ob.select_set(True)
 bpy.context.view_layer.objects.active=meshes[0]
 bpy.ops.export_scene.gltf(filepath=str(out),export_format="GLB",use_selection=True)
 row={"section":a.section,"file":str(out),"sha256":sha(out),
      "bytes":out.stat().st_size,"original_full_detail_file":str(master),
      "original_full_detail_sha256":entry["sha256"],
      "original_region_source_sha256":original["source_sha256"],
      "original_triangles":before["triangles"],"triangle_faces":after["triangles"],
      "vertices":after["vertices"],"mobile_ratio":a.ratio,
      "original_mesh_derived":True,"native_imported":False,"owner_accepted":False}
 (dst/(master.stem+".receipt.json")).write_text(json.dumps(row,indent=2)+"\n","utf-8")
 print("NW_R1_MOBILE_SECTION_OK",a.section,after["triangles"],str(out))
if __name__=="__main__":main()
