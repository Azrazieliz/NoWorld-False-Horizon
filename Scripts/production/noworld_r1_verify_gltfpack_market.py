#!/usr/bin/env python3
"""Verify an original R1 market-part source-derived gltfpack mobile GLB.

No fabricated triangle counts or inferred provenance; parse real GLB index
accessors, compare original source receipt SHA, and record both file SHA.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

def digest(p):
 h=hashlib.sha256()
 with open(p,"rb") as f:
  for b in iter(lambda:f.read(1<<20),b""):h.update(b)
 return h.hexdigest()
def glb_triangles(p):
 with open(p,"rb") as f:
  header=f.read(20)
  magic,version,total,chunklen,chunktype=struct.unpack("<IIIII",header)
  if magic!=0x46546c67 or version!=2 or chunktype!=0x4e4f534a:
   raise ValueError("True GLB 2.0 JSON source required")
  scene=json.loads(f.read(chunklen))
 mesh_triangles=0
 for mesh in scene.get("meshes",[]):
  for prim in mesh.get("primitives",[]):
   if prim.get("mode",4)!=4:continue
   idx=prim.get("indices")
   if idx is None:
    count=scene["accessors"][prim["attributes"]["POSITION"]]["count"]
   else:
    count=scene["accessors"][idx]["count"]
   if count%3:raise ValueError("Triangle index buffer length invalid")
   mesh_triangles+=count//3
 return scene,mesh_triangles

def main():
 ap=argparse.ArgumentParser()
 ap.add_argument("--original-master",type=Path,required=True)
 ap.add_argument("--derivative",type=Path,required=True)
 ap.add_argument("--section",type=int,required=True)
 ap.add_argument("--optimizer",type=Path,required=True)
 args=ap.parse_args()
 original=args.original_master.resolve(strict=True)
 derived=args.derivative.resolve(strict=True)
 master=json.loads((original.parent.parent/"source_receipt.json").read_text("utf8"))["sources"][0]
 entry=next((x for x in master["outputs"] if x["section"]==args.section),None)
 if entry is None or original!=Path(entry["file"]).resolve(strict=True):
  raise ValueError("Original frozen market partition identity mismatch")
 if digest(original)!=entry["sha256"] or original==derived:
  raise ValueError("Original full-detail source missing or modified")
 scene,triangles=glb_triangles(original)
 modified,smaller=glb_triangles(derived)
 if triangles!=entry["triangle_faces"] or not (0<smaller<triangles):
  raise ValueError("Actual geometry must be original and strictly reduced")
 if not modified.get("meshes") or not modified.get("materials"):
  raise ValueError("Source material/geometry lost in GLB")
 if "EXT_meshopt_compression" in modified.get("extensionsRequired",[]):
  raise ValueError("Native Unreal importing requires uncompressed GLB geometry")
 asset={"section":args.section,"file":str(derived),"sha256":digest(derived),
        "bytes":derived.stat().st_size,"original_full_detail_file":str(original),
        "original_full_detail_sha256":entry["sha256"],
        "original_region_source_sha256":master["source_sha256"],
        "original_triangles":triangles,"triangle_faces":smaller,
        "original_mesh_derived":True,"mobile_ratio":smaller/triangles,
        "optimizer":"zeux_meshoptimizer_gltfpack","optimizer_binary_sha256":digest(args.optimizer),
        "material_count":len(modified["materials"]),
        "accepted_by_owner":False,"native_imported":False}
 dst=derived.with_suffix(".receipt.json")
 dst.write_text(json.dumps(asset,indent=2)+"\n","utf8")
 print("SOURCE_MOBILE_GLTFPACK_VERIFIED",args.section,triangles,smaller,asset["sha256"])
if __name__=="__main__":main()
