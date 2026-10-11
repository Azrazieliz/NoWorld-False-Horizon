"""Read-only full-resolution metadata audit for fresh KKBP Blender GLB.
Never mistake file existence for Unreal import. Preserve all source assets."""
import argparse,hashlib,json,pathlib,struct,io
from PIL import Image
p=argparse.ArgumentParser();p.add_argument("state",choices=("CLOTHED","UNDERWEAR","NUDE"));args=p.parse_args()
stage=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011")
out=stage/"BLENDER_FRESH"/args.state
receipt=json.loads((out/"GATE12_FRESH_BLENDER_CONVERSION_RECEIPT.json").read_text(encoding="utf-8-sig"))
assert receipt["status"]=="PASS_REAL_PRIMARY_PARTY_SOURCE_BLENDER_FBX_GLB_EXPORTED_PENDING_INDEPENDENT_VALIDATION",receipt["status"]
source=json.loads((stage/("FRESH_EXPORT_"+args.state+"_RECEIPT.json")).read_text(encoding="utf-8-sig"))
glb=out/("LUMINOUS_V3_"+args.state+"_skinned.glb")
assert glb.is_file() and glb.stat().st_size>100000
raw=glb.read_bytes()
assert raw[:4]==b"glTF" and struct.unpack_from("<I",raw,4)[0]==2 and struct.unpack_from("<I",raw,8)[0]==len(raw)
i=12;chunks={}
while i<len(raw):
 size,typ=struct.unpack_from("<II",raw,i);i+=8
 chunks[typ]=raw[i:i+size];i+=size
assert 0x4e4f534a in chunks and 0x4e4942 in chunks
metadata=json.loads(chunks[0x4e4f534a])
blob=chunks[0x4e4942]
assert len(metadata.get("skins",[]))>0
assert len(metadata.get("meshes",[]))>0
assert len(metadata.get("materials",[]))>0
assert len(metadata.get("nodes",[]))>50
primitives=[p for mesh in metadata.get("meshes",[]) for p in mesh["primitives"]]
assert len(primitives)>=6
assert all("POSITION" in x["attributes"] for x in primitives)
assert any("JOINTS_0" in x["attributes"] for x in primitives)
assert any("WEIGHTS_0" in x["attributes"] for x in primitives)
assert any("TEXCOORD_0" in x["attributes"] for x in primitives)
assert any("NORMAL" in x["attributes"] for x in primitives)
images=[]
for img in metadata.get("images",[]):
 if "bufferView" in img:
  view=metadata["bufferViews"][img["bufferView"]]
  rawimage=blob[view.get("byteOffset",0):view.get("byteOffset",0)+view["byteLength"]]
  im=Image.open(io.BytesIO(rawimage))
  assert im.width>2 and im.height>2
  images.append({"name":img.get("name"),"format":im.format,"width":im.width,"height":im.height,"bytes":len(rawimage),
                 "sha256":hashlib.sha256(rawimage).hexdigest()})
 else:
  raise ValueError("GLB image outside embedded buffer; cannot audit resolution")
assert images
morph_count=sum(len(x.get("targets",[])) for x in primitives)
report={"schema":"G12_LUMINOUS_V3_REAL_KKBP_GLB_GEOMETRY_IMAGE_MORPH_AUDIT",
"status":"PASS_REAL_SOURCE_GEOMETRY_SKINS_UVS_IMAGES_WITH_MORPH_LINKS" if morph_count>0 else "FAIL_GLTF_HAS_ZERO_MORPHS",
"state":args.state,"source_party_state_hash":source["source_card_sha256"],"source_pmx_sha256":source["pmx_sha256"],
"source_immutable":True,"glb_sha256":hashlib.sha256(raw).hexdigest(),"glb_bytes":len(raw),
"armature_skin_count":len(metadata["skins"]),"glTF_mesh_count":len(metadata["meshes"]),
"node_count":len(metadata["nodes"]),"primitive_count":len(primitives),
"joint_count_per_skin":[len(x["joints"]) for x in metadata["skins"]],
"morph_target_references":morph_count,"embedded_image_count":len(images),
"materials_count":len(metadata["materials"]),"image_inventory":images,
"native_unreal_import":"NOT_TESTED","native_android_cook":"NOT_TESTED",
"real_party_character":True,"source_material_shader_fidelity":"NEEDS_UNREAL_RUNTIME_TEST",
"native_morph_geometry_nonzero":"NEEDS_UNREAL_RUNTIME_TEST",
"on_device_state_switch":"NOT_TESTED"}
path=out/"GATE12_GLTF_REAL_GEOMETRY_MORPH_AUDIT.json"
path.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
print(json.dumps({k:v for k,v in report.items() if k!="image_inventory"},indent=2))
if not report["status"].startswith("PASS"):raise SystemExit(2)
