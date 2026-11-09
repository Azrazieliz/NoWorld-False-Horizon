#!/usr/bin/env python3
"""Finalize twelve source-provenance Eastern Market mobile GLBs for 5-star UE import.

Exactly original authored 2026-10-19 market: eight decimated source-derived
sections and four already-sub-1000-triangle original sections copied byte-exact.
The old master and all raw source .glb assets stay unchanged.
"""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

def digest(file):
 h=hashlib.sha256()
 with open(file,"rb") as f:
  for b in iter(lambda:f.read(1<<20),b""):h.update(b)
 return h.hexdigest()

def main():
 p=argparse.ArgumentParser()
 p.add_argument("--master-root",type=Path,required=True)
 p.add_argument("--output-root",type=Path,required=True)
 a=p.parse_args()
 master=a.master_root.resolve(strict=True)
 out=a.output_root.resolve()
 if out==master or master in out.parents:raise ValueError("Cannot overwrite source master")
 original=json.loads((master/"source_receipt.json").read_text("utf-8"))
 if original.get("schema")!="noworld.r1.native_source_preparation.v1":
  raise ValueError("Original R1 source receipt required")
 source=original["sources"][0]
 if source["source_sha256"]!="a6373a82b629b667714f4e39b741ff66450d8ece338bf5eaf72431148d489e3c":
  raise ValueError("Frozen original market SHA256 changed")
 if len(source["outputs"])!=12:raise ValueError("Twelve original partitions required")
 entries=[]
 for item in source["outputs"]:
  idx=item["section"]
  original_file=Path(item["file"]).resolve(strict=True)
  if digest(original_file)!=item["sha256"]:
   raise ValueError("Original section was modified: "+str(original_file))
  dst=out/"market"/original_file.name
  dst.parent.mkdir(parents=True,exist_ok=True)
  is_tiny=item["triangle_faces"]<1000
  if is_tiny:
   if dst.exists() and digest(dst)!=item["sha256"]:
    raise ValueError("A different mobile mesh already exists at exact tiny source path")
   shutil.copy2(original_file,dst)
   triangles=item["triangle_faces"]
   row={"original_triangles":triangles,"triangle_faces":triangles,
        "source_sha256":item["sha256"],"original_mesh_derived":True,
        "original_unchanged":True,"mobile_ratio":1.0}
  else:
   receipt=dst.with_suffix(".receipt.json")
   if not receipt.is_file():raise FileNotFoundError("Authentic source LOD pending: "+str(receipt))
   row=json.loads(receipt.read_text("utf-8"))
   if row.get("original_full_detail_sha256")!=item["sha256"] or \
      row.get("original_triangles")!=item["triangle_faces"] or \
      row.get("triangle_faces",0)>=item["triangle_faces"]:
    raise ValueError("Original mesh provenance/LOD reduction invalid: "+str(dst))
   triangles=row["triangle_faces"]
  if not dst.is_file():raise FileNotFoundError(dst)
  if not is_tiny and row["sha256"]!=digest(dst):raise ValueError("LOD hash invalid")
  entries.append({"section":idx,"file":str(dst),"sha256":digest(dst),
        "source_master_sha256":item["sha256"],
        "source_original_triangles":item["triangle_faces"],
        "triangle_faces":triangles,"bytes":dst.stat().st_size,
        "original_mesh_derived":True,
        "mobile_ratio":row["mobile_ratio"],"passthrough_source_geometry":is_tiny})
 output={"schema":"noworld.r1.native_source_preparation.v1",
    "source_reused_without_proxy":True,"mobile_source_derivative":True,
    "native_unreal_verified":False,"accepted_by_owner":False,
    "sources":[{"source":source["source"],
        "source_sha256":source["source_sha256"],
        "source_stats":source["source_stats"],
        "output_stats":{"triangle_faces":sum(x["triangle_faces"] for x in entries)},
        "geometry_preserved":False,"outputs":entries}]}
 if {x["section"] for x in entries}!=set(range(12)):raise ValueError("Duplicate/missing source section")
 (out/"source_receipt.json").write_text(json.dumps(output,indent=2)+"\n","utf-8")
 print("MARKET_MOBILE_SOURCE_VERIFIED sections=12","source_triangles="+
       str(source["source_stats"]["triangle_faces"]),"mobile_triangles="+
       str(output["sources"][0]["output_stats"]["triangle_faces"]),"files=12")
if __name__=="__main__":main()
