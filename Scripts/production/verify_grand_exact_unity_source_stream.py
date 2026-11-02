"""Verify the ACTUAL Grand original Koikatsu card's direct Unity source mesh stream.
Read-only, bounded binary parser; rejects truncated/foreign/mock meshes.
This proves source extraction, never Unreal gameplay/Android acceptance.
"""
import pathlib,struct,json,hashlib,re,sys
PROJECT=pathlib.Path(r"D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031")
SOURCE=pathlib.Path(r"D:\UnrealProjects\G12_LuminousNativeImport_20261031\Saved\GrandExactUnitySourceStream")
CARD=PROJECT/"Assets/NoWorld/Luminous/KoikatsuOriginalCards/LUMINOUS_GRAND_CONVERGENCE_CLOTHED_KK.png"
CARD_SHA="26c48e1c03c154f9f39a0b06afaade53423c11dbd079604e430162eb4cae5353"
EXPECTED_MAGIC="NOWORLD_EXACT_KOIKATSU_UNITY_SKIN_V3_SPARSE_MORPHS_CURRENT_WEIGHTS"
def digest(path):
 h=hashlib.sha256()
 with path.open("rb") as f:
  for c in iter(lambda:f.read(4*1024*1024),b""):h.update(c)
 return h.hexdigest()
class Reader:
 def __init__(self,f):
  self.f=f;self.file_size=f.seek(0,2);f.seek(0)
 def n(self,sz):
  b=self.f.read(sz)
  if len(b)!=sz:raise ValueError("Truncated original mesh source")
  return b
 def u(self):return struct.unpack("<I",self.n(4))[0]
 def i(self):return struct.unpack("<i",self.n(4))[0]
 def length(self):
  value=0
  for j in range(5):
   b=self.n(1)[0];value|=(b&127)<<(7*j)
   if b<128:return value
  raise ValueError("Malformed .NET length-prefixed string")
 def text(self):
  size=self.length()
  if size>65536:raise ValueError("Oversize string")
  return self.n(size).decode("utf8")
 def skip(self,sz):
  if sz<0 or self.f.tell()+sz>self.file_size:raise ValueError("Bad mesh buffer bounds")
  self.f.seek(sz,1)
def audit(path):
 with path.open("rb") as f:
  rd=Reader(f)
  if rd.text()!=EXPECTED_MAGIC or rd.text()!=CARD_SHA:raise ValueError("Unapproved Grand card mesh source")
  ix=rd.i();name=rd.text();bone_root=rd.text()
  verts,subs,shapes=rd.i(),rd.i(),rd.i()
  if not(0<verts<2000000 and 0<subs<4096 and 0<=shapes<50000):raise ValueError("Impossible renderer dimensions")
  rd.skip(64)
  normals,uvs,weights=rd.i(),rd.i(),rd.i()
  if normals not in(0,verts) or uvs not in(0,verts) or weights!=verts:
   raise ValueError("Incomplete native geometry/skin data")
  rd.skip(12*verts+12*normals+8*uvs)
  bone_count=rd.i();root=rd.text()
  if bone_count<2 or bone_count>5000:raise ValueError("Invalid original skeletal bone count")
  known_bones=set()
  for _ in range(bone_count):
   nm=rd.text()
   if not nm or nm in known_bones:raise ValueError("Duplicate or missing native bone")
   known_bones.add(nm);rd.skip(64*2)
  rd.skip(32*weights)
  indices=[]
  for sub in range(subs):
   n=rd.i()
   if n<3 or n%3 or n>20000000:raise ValueError("Corrupt native triangle indices")
   rd.skip(4*n)
   indices.append(n)
  mats=rd.i()
  if mats<0 or mats>subs+150:raise ValueError("Invalid source material slots")
  material_names=[];textures=[]
  for _ in range(mats):
   material_names.append(rd.text());shader=rd.text();rd.skip(16)
   tex=rd.text();width,height=rd.i(),rd.i()
   if width<0 or height<0:raise ValueError("Invalid source texture")
   textures.append({"name":tex,"width":width,"height":height})
  frames_total=0
  for _ in range(shapes):
   shapename=rd.text()
   if not shapename:raise ValueError("Invalid source morph")
   source_current_weight=struct.unpack("<f",rd.n(4))[0]
   if not -100000.0<=source_current_weight<=100000.0:
    raise ValueError("Invalid exact current character morph weight")
   frames=rd.i()
   if frames<1 or frames>100:raise ValueError("Bad original shape key frames")
   for _ in range(frames):
    rd.skip(4)
    changed=rd.i()
    if changed<0 or changed>verts:raise ValueError("Invalid source morph's active vertex count")
    last=-1
    for _ in range(changed):
     ix=rd.i()
     if ix<=last or ix>=verts:raise ValueError("Noncanonical or duplicate exact morph vertex index")
     last=ix
     rd.skip(36)
   frames_total+=frames
  if f.tell()!=rd.file_size:raise ValueError("Undeclared trailing bytes in original Grand renderer")
  return {"source_renderer_index":ix,"renderer_name":name,"renderer_path":bone_root,
    "vertex_count":verts,"submesh_count":subs,"blendshape_count":shapes,
    "blendshape_frames":frames_total,"bone_count":bone_count,"material_count":mats,
    "submesh_triangle_counts":[n//3 for n in indices],"original_material_names":material_names,
    "original_texture_refs":textures,"source_sha256":digest(path),"bytes":path.stat().st_size,
    "source_file":str(path)}
def main():
 assert digest(CARD)==CARD_SHA,"Original approved Grand card SHA mismatch"
 outputs=sorted(SOURCE.glob("ExactGrand_Renderer_*.unitymesh"))
 if not outputs:raise RuntimeError("Grand source extraction has not produced any real meshes")
 rows=[audit(f) for f in outputs]
 if len({r["source_renderer_index"] for r in rows})!=len(rows):
  raise RuntimeError("Original renderer index collision")
 marker=SOURCE/"GATE12_GRAND_EXACT_SOURCE_STREAM_COMPLETE.sha256"
 complete=marker.is_file() and marker.read_text("utf8").strip()==CARD_SHA
 doc={"schema":"noworld.gate12.grand_koikatsu_original_unity_stream.v1",
  "original_approved_card_sha256":CARD_SHA,
  "source_renderers":rows,"meshes":len(rows),"vertices":sum(x["vertex_count"] for x in rows),
  "source_bones_max":max(x["bone_count"] for x in rows),
  "source_morphs":sum(x["blendshape_count"] for x in rows),
  "source_material_references":sum(x["material_count"] for x in rows),
  "source_complete":complete,
  "native_unreal_3d_imported":False,
  "original_visual_accepted":False,
  "android_gameplay_verified":False,
  "status":"PASS_EXACT_APPROVED_GRAND_UNITY_SOURCE_COMPLETE_PENDING_NATIVE_UE"
           if complete else "PARTIAL_REAL_GRAND_NATIVE_MESH_STREAM"}
 out=SOURCE/"G12_Grand_Unity_Exact_OriginalCard_Provenance.json"
 out.write_text(json.dumps(doc,indent=2)+"\n","utf8")
 print("VERIFIED_EXACT_GRAND_CARD_STREAM",doc["status"],
       "renderers",len(rows),"vertices",doc["vertices"],
       "native_morphs",doc["source_morphs"],
       "max_bones",doc["source_bones_max"],flush=True)
 return 0 if complete else 2
if __name__=="__main__":sys.exit(main())
