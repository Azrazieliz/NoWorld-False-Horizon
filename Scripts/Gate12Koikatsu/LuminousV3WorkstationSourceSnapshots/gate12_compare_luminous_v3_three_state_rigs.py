"""Read-only exact skin hierarchy/reference rest comparison for 3 Luminous states."""
from pathlib import Path
import hashlib,struct,json,datetime
base=Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011")
sigs={}
for state in ("CLOTHED","UNDERWEAR","NUDE"):
 p=base/"BLENDER_FRESH"/state/("LUMINOUS_V3_"+state+"_skinned.glb")
 raw=p.read_bytes()
 assert raw[:4]==b"glTF"
 n=int.from_bytes(raw[12:16],"little")
 gltf=json.loads(raw[20:20+n])
 assert len(gltf["skins"])==1
 skin=gltf["skins"][0]
 ids=skin["joints"]
 nodes=gltf["nodes"]
 parents={}
 for pi,node in enumerate(nodes):
  for ci in node.get("children",[]):parents[ci]=pi
 skeleton=[]
 for idx in ids:
  node=nodes[idx]
  parent_index=parents.get(idx)
  skeleton.append({"name":node.get("name",""),"parent":nodes[parent_index].get("name","") if parent_index is not None else None,
                   "translation":node.get("translation"),"rotation":node.get("rotation"),"scale":node.get("scale")})
 reference=hashlib.sha256(json.dumps(skeleton,sort_keys=True,separators=(",",":")).encode("utf8")).hexdigest()
 bind=hashlib.sha256(str(skin.get("inverseBindMatrices")).encode()).hexdigest()
 sigs[state]={"sha256":hashlib.sha256(raw).hexdigest(),"skin_joint_count":len(ids),
               "skeleton_rest_name_parent_transform_sha256":reference,
               "inverse_bind_matrices_accessor":skin.get("inverseBindMatrices"),
               "primitive_count":sum(len(x["primitives"]) for x in gltf["meshes"]),
               "mesh_count":len(gltf["meshes"]),
               "embedded_image_count":len(gltf["images"]),"material_count":len(gltf["materials"])}
print(json.dumps(sigs,indent=2))
equal=len(set(v["skeleton_rest_name_parent_transform_sha256"] for v in sigs.values()))==1
result={"schema":"G12_LUMINOUS_V3_THREE_WARDROBE_ONE_RIG_REST_AUDIT_1",
        "status":"PASS_IDENTICAL_SKIN_REFERENCE_HIERARCHY" if equal else "FAIL_OR_DISTINCT_SKELETON_REQUIRES_RETARGET",
        "states":sigs,"one_character_three_wardrobe_states":True,
        "actual_UE_component_compatibility":"NOT_TESTED",
        "full_outfit_switch":"NOT_TESTED",
        "date_utc":datetime.datetime.now(datetime.timezone.utc).isoformat()}
out=base/"LUMINOUS_V3_THREE_STATE_RIG_COMPARISON.json"
out.write_text(json.dumps(result,indent=2)+"\n")
print("RIG_REST_COMPARISON",result["status"])
if not equal:raise SystemExit(2)
