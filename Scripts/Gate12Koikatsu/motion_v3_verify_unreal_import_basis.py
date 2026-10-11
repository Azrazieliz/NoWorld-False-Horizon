"""Read-only exact glTF→UE5.8 reference-pose basis calibration (one real rig).

Source GLB Y-up/Z-forward → UE Z-up/Y-forward from observed native pose.
Original source geometry/rig and UE .uassets remain unchanged.
"""
from __future__ import annotations
import json, math, pathlib, sys
from motion_v3_bvh_retarget_core import GLTFRig, q_norm
ROOT=pathlib.Path(__file__).resolve().parents[2]
GLB=ROOT/"Saved/Gate12Koikatsu/full_fidelity_primitive_shards/Mesh_03_Prim_00.glb"
READ=ROOT/"Saved/Gate12Koikatsu/MotionV3/native_ue_onepart_refpose_audit.json"
OUT=ROOT/"Saved/Gate12Koikatsu/MotionV3/verified_native_ue_glb_axis_transform.json"
def source_vec_to_unreal(v):
    return (100*v[0],100*v[2],100*v[1])
def source_quat_to_unreal(q):
    x,y,z,w=q_norm(q)
    return (-x,-z,-y,w)
def angular_error_deg(a,b):
    qa=q_norm(a);qb=q_norm(b)
    dot=abs(sum(x*y for x,y in zip(qa,qb)))
    return math.degrees(2*math.acos(min(1.,dot)))
def verify():
    g=GLTFRig(GLB)
    receipt=json.loads(READ.read_text(encoding="utf-8-sig"))
    assert receipt["status"]=="PASS_READONLY_UE_ONEPART_REFERENCE_POSE"
    rows=[]
    for item in receipt["bone_poses"]:
        name=item["name"]
        assert name in g.by_name
        node=g.nodes[g.by_name[name]]
        t=node.get("translation",(0,0,0))
        q=node.get("rotation",(0,0,0,1))
        expected=source_vec_to_unreal(t)
        eu=item["translation"]
        error=math.sqrt(sum((a-b)**2 for a,b in zip(expected,eu)))
        angle=angular_error_deg(source_quat_to_unreal(q),item["rotation"])
        rows.append({"bone":name,"glb_local_translation_m":t,
                     "unreal_local_translation_cm":eu,
                     "predicted_unreal_local_translation_cm":expected,
                     "translation_error_cm":error,
                     "imported_unreal_local_quat":item["rotation"],
                     "predicted_unreal_local_quat":source_quat_to_unreal(q),
                     "quaternion_angle_error_degrees":angle})
    assert len(rows)>=20
    max_pos=max(r["translation_error_cm"] for r in rows)
    max_ang=max(r["quaternion_angle_error_degrees"] for r in rows)
    result={
        "status":"PASS_EXACT_UE_NATIVE_ONEPART_GLB_TRANSFORM" if
             max_pos<.03 and max_ang<.15 else "FAIL_NATIVE_GLB_REFPOSE_TRANSFORM_MISMATCH",
        "tested_bones":len(rows),"translation_map_m_to_cm":"(X,Z,Y)*100",
        "quaternion_map_xyzw_to_ue":"(-x,-z,-y,w)",
        "max_translation_error_cm":max_pos,
        "max_quaternion_angular_error_degrees":max_ang,
        "native_onepart":"KK_Full_M03P00",
        "gltf_character_geometric_face_hint":"+Z",
        "unreal_imported_visual_face_hint":"+Y",
        "baseline_fixture_bp_part_relative_yaw_deg":0,
        "required_actor_X_forward_mesh_yaw_correction_deg":-90,
        "correction_applied_in_unreal":False,
        "other_18_parts_import_transform_check":"NOT_TESTED",
        "animation_ready":"NO_REQUIRES_ACTUAL_UE_POSE_AND_MOBILE_QA",
        "data":rows}
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(json.dumps({k:v for k,v in result.items() if k!="data"},indent=2))
    return 0 if result["status"].startswith("PASS") else 2
if __name__=="__main__":sys.exit(verify())
