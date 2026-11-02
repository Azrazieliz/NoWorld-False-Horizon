"""Actual nonzero morph POSITION/normal stream count in lossless source glTF.

A Koikatsu exporter may preserve many named zero-movement shape-key slots.
Native Unreal import collapses/discards targets whose vertices never move.
Do not equate original name count with nonzero runtime facial animations.
"""
import sys,struct,json,pathlib,argparse,collections
p=argparse.ArgumentParser();p.add_argument("shard_glb",type=pathlib.Path);p.add_argument("--output",type=pathlib.Path);args=p.parse_args()
b=args.shard_glb.read_bytes()
assert b[:4]==b"glTF"
size=int.from_bytes(b[12:16],"little")
doc=json.loads(b[20:20+size])
index=20+size
index+=8 if index<len(b) and b[index:index+4]!=b"BIN" else 0
pos=12;chunks={}
while pos<len(b):
 sz,typ=struct.unpack_from("<II",b,pos);pos+=8
 chunks[typ]=b[pos:pos+sz];pos+=sz
buf=chunks[0x4e4942]
views=doc["bufferViews"]
accessors=doc["accessors"]
def has_delta(k,epsilon=1e-6):
 a=accessors[k]
 assert a["componentType"]==5126 and a["type"]=="VEC3"
 if "bufferView" in a:
  v=views[a["bufferView"]]
  start=v.get("byteOffset",0)+a.get("byteOffset",0)
  stride=v.get("byteStride",12)
  for i in range(a["count"]):
   x,y,z=struct.unpack_from("<fff",buf,start+i*stride)
   if abs(x)>epsilon or abs(y)>epsilon or abs(z)>epsilon:return True
 if "sparse" in a:
  s=a["sparse"];vv=views[s["values"]["bufferView"]]
  start=vv.get("byteOffset",0)+s["values"].get("byteOffset",0)
  for i in range(s["count"]):
   x,y,z=struct.unpack_from("<fff",buf,start+i*12)
   if abs(x)>epsilon or abs(y)>epsilon or abs(z)>epsilon:return True
 return False
def max_position_magnitude(k):
 a=accessors[k]
 assert a["componentType"]==5126 and a["type"]=="VEC3"
 largest_sq=0.0
 if "bufferView" in a:
  v=views[a["bufferView"]];start=v.get("byteOffset",0)+a.get("byteOffset",0);stride=v.get("byteStride",12)
  for j in range(a["count"]):
   x,y,z=struct.unpack_from("<fff",buf,start+j*stride)
   largest_sq=max(largest_sq,x*x+y*y+z*z)
 if "sparse" in a:
  s=a["sparse"];v=views[s["values"]["bufferView"]];start=v.get("byteOffset",0)+s["values"].get("byteOffset",0)
  for j in range(s["count"]):
   x,y,z=struct.unpack_from("<fff",buf,start+j*12)
   largest_sq=max(largest_sq,x*x+y*y+z*z)
 return largest_sq**0.5
rows=[]
for mi,mesh in enumerate(doc["meshes"]):
 for pi,primitive in enumerate(mesh["primitives"]):
  nonzero_pos=[]
  visible_015mm=[]
  nonzero_norm=[]
  for ti,target in enumerate(primitive.get("targets",[])):
   if "POSITION" in target:
    if has_delta(target["POSITION"]):nonzero_pos.append(ti)
    if max_position_magnitude(target["POSITION"])>0.00015:visible_015mm.append(ti)
   if "NORMAL" in target and has_delta(target["NORMAL"]):nonzero_norm.append(ti)
  rows.append({"mesh":mi,"primitive":pi,"total_morph_slots":len(primitive.get("targets",[])),
          "nonzero_position_slot_indices":nonzero_pos,
          "native_015mm_position_slot_indices":visible_015mm,
          "nonzero_normal_slot_indices":nonzero_norm,
          "nonzero_position_target_count":len(nonzero_pos),
          "native_015mm_position_target_count":len(visible_015mm),
          "nonzero_normal_target_count":len(nonzero_norm)})
report={"schema":"G12_REAL_GLB_NONZERO_MORPH_GEOMETRY_CHECK_1",
        "status":"PASS_REAL_SOURCE_BYTEWISE_FLOAT_DELTA_ANALYSIS",
        "file":str(args.shard_glb),
        "total_shape_key_slots":sum(x["total_morph_slots"] for x in rows),
        "nonzero_position_morphs":sum(x["nonzero_position_target_count"] for x in rows),
        "native_015mm_position_morphs":sum(x["native_015mm_position_target_count"] for x in rows),
        "nonzero_normal_morphs":sum(x["nonzero_normal_target_count"] for x in rows),
        "meshes":rows}
if args.output:
 args.output.parent.mkdir(parents=True,exist_ok=True)
 args.output.write_text(json.dumps(report,indent=2)+"\n")
print(json.dumps({k:v for k,v in report.items() if k!="meshes"},indent=2))
