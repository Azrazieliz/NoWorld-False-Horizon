"""Gate12: reject misleading V2 animation-quality PASS from valid-but-wrong axes.

Analyzes the immutable real Koikatsu glTF reference rig and generator axes.
Does not inspect/modify Unreal files. UE-imported bone axes still need
independent in-engine inspection before *corrected* motion can be approved.
"""
from __future__ import annotations
import json, pathlib, struct, sys, math
from kk_native_motion_v2_profiles import AXES
ROOT=pathlib.Path(__file__).resolve().parents[2]
SHARD=ROOT/"Saved/Gate12Koikatsu/full_fidelity_primitive_shards/Mesh_03_Prim_00.glb"
OUTPUT=ROOT/"Saved/Gate12Koikatsu/NativeMotionV2/v2_anatomical_axis_rejection_report.json"
MOTION_PARENT_CHILD={
    "thigh_l":"calf_l",
    "thigh_r":"calf_r",
    "calf_l":"Left ankle",
    "calf_r":"Right ankle",
    "upperarm_l":"lowerarm_l",
    "upperarm_r":"lowerarm_r",
    "lowerarm_l":"hand_l",
    "lowerarm_r":"hand_r",
}
def main():
    raw=SHARD.read_bytes()
    assert raw[:4]==b"glTF"
    size,kind=struct.unpack_from("<II",raw,12)
    assert kind==0x4E4F534A
    gltf=json.loads(raw[20:20+size])
    nodes=gltf["nodes"]
    byname={n.get("name"):n for n in nodes}
    indices={n.get("name"):i for i,n in enumerate(nodes)}
    results=[]
    for parent,child in MOTION_PARENT_CHILD.items():
        assert parent in byname and child in byname and parent in AXES
        node=byname[parent]
        assert indices[child] in node.get("children",[])
        axis=AXES[parent]
        bone=byname[child]["translation"]
        norm_axis=math.sqrt(sum(t*t for t in axis))
        norm_bone=math.sqrt(sum(t*t for t in bone))
        dot=abs(sum(a*b for a,b in zip(axis,bone))/(norm_axis*norm_bone))
        results.append({"joint":parent,"child":child,
            "v2_rotation_axis":axis,
            "reference_bone_direction_local":bone,
            "absolute_axis_bone_dot":round(dot,6),
            "classification": "REJECT_ROTATES_NEAR_BONE_LONG_AXIS" if dot>=0.85 else
                    "DIRECTION_PERPENDICULAR_IN_SOURCE_GLTF_ONLY"})
    rejected=[item for item in results if item["absolute_axis_bone_dot"]>=0.85]
    result={"status":"FAIL_ANATOMICAL_HINGE_AXIS_CONTRACT" if rejected
                  else "PASS_SOURCE_AXES_GEOMETRIC_PERPENDICULARITY_ONLY",
            "source":str(SHARD),"tested":len(results),
            "longitudinal_twist_rejections":len(rejected),
            "unreal_engine_native_axes_verified":False,
            "user_visual_motion_quality":"FAIL_OBSERVED",
            "current_76_native_animations_skeleton_compatible":True,
            "current_76_native_animations_anatomically_approved":False,
            "findings":results}
    OUTPUT.write_text(json.dumps(result,indent=2)+"\n",encoding="utf8")
    print(json.dumps({"status":result["status"],
        "tested":len(results),"longitudinal_twist_rejections":len(rejected),
        "bad_joints":rejected},indent=2))
    return 2 if rejected else 0
if __name__=="__main__":sys.exit(main())
