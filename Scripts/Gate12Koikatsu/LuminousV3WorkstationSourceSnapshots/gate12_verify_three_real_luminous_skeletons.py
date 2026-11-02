"""Compare actual three independently exported Koikatsu V3 GLB skeletons.

No substitutions, no external assets, no animation/UE acceptance claims.
Detects if same-rig instant visual clothing swap is possible or if modular
pose copying / variant rig retargeting is required.
"""
import pathlib,json,struct,hashlib,collections
ROOT=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011")
STATES=["CLOTHED","UNDERWEAR","NUDE"]
out={}
def open_glb(path):
 b=path.read_bytes()
 assert b[:4]==b"glTF" and struct.unpack_from("<I",b,8)[0]==len(b)
 n,magic=struct.unpack_from("<II",b,12)
 assert magic==0x4e4f534a
 return json.loads(b[20:20+n])
for state in STATES:
 report=json.loads((ROOT/"BLENDER_ROUNDTRIPS"/state/"FRESH_KKBP_BLENDER_FBX_GLB_REPORT.json").read_text(encoding="utf8"))
 assert report["status"]=="PASS_FRESH_KOIKATSU_TO_BLENDER_FBX_GLB_PRE_UNREAL"
 glb=ROOT/"BLENDER_ROUNDTRIPS"/state/("Luminous_V3_"+state+".glb")
 j=open_glb(glb)
 assert len(j.get("skins",[]))==1
 nodes=j["nodes"]
 parent={}
 for ni,n in enumerate(nodes):
  for child in n.get("children",[]):parent[child]=ni
 jointidx=j["skins"][0]["joints"]
 allbones={}
 for bone in jointidx:
  n=nodes[bone];name=n.get("name")
  assert name is not None
  allbones[name]={"name":name,
    "parent":nodes[parent[bone]].get("name") if bone in parent else None,
    "translation":n.get("translation",[0,0,0]),
    "rotation":n.get("rotation",[0,0,0,1]),
    "scale":n.get("scale",[1,1,1])}
 assert len(allbones)==len(jointidx),"Duplicate names prevent identity switch"
 out[state]={"glb_sha256":hashlib.sha256(glb.read_bytes()).hexdigest(),
  "joint_count":len(jointidx),"node_count":len(nodes),
  "mesh_count":len(j.get("meshes",[])),
  "primitive_count":sum(len(mesh["primitives"]) for mesh in j.get("meshes",[])),
  "morph_target_refs":sum(len(p.get("targets",[])) for m in j.get("meshes",[]) for p in m["primitives"]),
  "images":len(j.get("images",[])),"materials":len(j.get("materials",[])),
  "bones":allbones,"bone_order":[nodes[x].get("name") for x in jointidx]}
common=set.intersection(*(set(out[s]["bones"]) for s in STATES))
per=[]
for other in ("UNDERWEAR","NUDE"):
 a=out["CLOTHED"]["bones"];b=out[other]["bones"]
 shared=sorted(set(a)&set(b))
 same_parent=sum(a[n]["parent"]==b[n]["parent"] for n in shared)
 same_pose=sum(
  a[n]["parent"]==b[n]["parent"] and
  max(abs(x-y) for x,y in zip(a[n]["translation"],b[n]["translation"]))<1e-4 and
  max(abs(x-y) for x,y in zip(a[n]["rotation"],b[n]["rotation"]))<1e-4
  for n in shared)
 per.append({"state":other,"shared_bones_with_clothed":len(shared),
  "common_percentage_of_clothed":round(100*len(shared)/len(a),4),
  "parent_name_compatible":same_parent,"reference_pose_compatible":same_pose,
  "only_clothed_bones":sorted(set(a)-set(b))[:35],
  "only_variant_bones":sorted(set(b)-set(a))[:35],
  "identical_order":out["CLOTHED"]["bone_order"]==out[other]["bone_order"]})
report={"schema":"G12_LUMINOUS_NATIVE_THREE_STATE_SKELETON_COMPAT_1",
 "states":{s:{k:v for k,v in out[s].items() if k not in ("bones","bone_order")} for s in STATES},
 "common_joint_name_count_all_states":len(common),
 "comparison_to_clothed":per,
 "source_states_are_one_identity_version":True,
 "cloth_runtime_switching_implemented":False,
 "source_joint_name_compatibility":
    "PASS" if len(common)>500 else "FAIL",
 "leader_pose_ready":all(x["identical_order"] and
    x["reference_pose_compatible"]==x["shared_bones_with_clothed"] for x in per),
 "unreal_native_per_state_import":"NOT_TESTED",
 "gate11_artifact_ready":False}
p=ROOT/"UE_INTAKE"/"THREE_STATE_SKELETON_COMPATIBILITY.json"
p.parent.mkdir(parents=True,exist_ok=True)
p.write_text(json.dumps(report,indent=2)+"\n",encoding="utf8")
print(json.dumps(report,indent=2))
