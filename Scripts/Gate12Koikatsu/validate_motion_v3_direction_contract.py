"""Gate 12 Motion V3 pre-implementation contract checks; NO .uasset edits.

Source-space glTF basis and Unreal world-to-character vector projection
must be treated as separate math operations. A passing direction contract
is necessary, but NEVER sufficient to approve authored animation curves,
actual imported mesh forward orientation, blending or phone rendering.
"""
from __future__ import annotations
import argparse, json, math, pathlib, struct, sys, time
ROOT=pathlib.Path(__file__).resolve().parents[2]
SOURCE=ROOT/"Saved/Gate12Koikatsu/full_fidelity_primitive_shards/Mesh_03_Prim_00.glb"
OUT=ROOT/"Saved/Gate12Koikatsu/NativeMotionV2/motion_v3_direction_contract_preimplementation.json"

def local_velocity(world_xy,actor_yaw_degrees):
    radians=math.radians(actor_yaw_degrees)
    f=(math.cos(radians),math.sin(radians))
    r=(-math.sin(radians),math.cos(radians))
    return (world_xy[0]*f[0]+world_xy[1]*f[1],
            world_xy[0]*r[0]+world_xy[1]*r[1])

def magnitude(v):
    return math.sqrt(sum(x*x for x in v))
def normalize(v):
    n=magnitude(v)
    return tuple(x/n for x in v) if n>1e-9 else tuple(0.0 for x in v)

def select_direction(forward,right):
    if math.hypot(forward,right)<1e-3:return "Idle"
    if abs(forward)>=abs(right):return "Forward" if forward>0 else "Backward"
    return "Right" if right>0 else "Left"

def parse_gltf_bone_source(path):
    raw=path.read_bytes();assert raw[:4]==b"glTF"
    n,kind=struct.unpack_from("<II",raw,12);assert kind==0x4E4F534A
    gltf=json.loads(raw[20:20+n]);nodes=gltf["nodes"]
    lookup={q["name"]:q for q in nodes if "name" in q}
    def get_translation(name):
        return tuple(lookup[name].get("translation",(0,0,0)))
    directions={}
    for bone,child in (("thigh_l","calf_l"),("thigh_r","calf_r"),
                       ("calf_l","Left ankle"),("calf_r","Right ankle"),
                       ("upperarm_l","lowerarm_l"),("upperarm_r","lowerarm_r"),
                       ("lowerarm_l","hand_l"),("lowerarm_r","hand_r")):
        a=get_translation(child)
        directions[bone]=normalize(a)
    return directions

def run(source=SOURCE):
    world_vectors={"X+":(1,0),"X-":(-1,0),"Y+":(0,1),"Y-":(0,-1)}
    # At yaw zero, UE actor forward is +X and right is +Y.
    # At yaw +90 deg, UE actor forward is +Y and right is -X.
    expected={0:{"X+":"Forward","X-":"Backward","Y+":"Right","Y-":"Left"},
             90:{"X+":"Left","X-":"Right","Y+":"Forward","Y-":"Backward"},
             180:{"X+":"Backward","X-":"Forward","Y+":"Left","Y-":"Right"},
             270:{"X+":"Right","X-":"Left","Y+":"Backward","Y-":"Forward"}}
    rows=[]
    for yaw,tests in expected.items():
        for axis,direction in tests.items():
            f,r=local_velocity(world_vectors[axis],yaw)
            actual=select_direction(f,r)
            assert actual==direction,(yaw,axis,actual,direction)
            assert abs(math.hypot(f,r)-1)<1e-9
            rows.append({"actor_yaw_deg":yaw,"world_velocity":axis,
                         "character_forward":round(f,6),"character_right":round(r,6),
                         "actual":actual,"expected":direction,"pass":True})
    for a,b in ((0,0),(.00001,0)):
        assert select_direction(a,b)=="Idle"
    bone_directions=parse_gltf_bone_source(source)
    # Known rejected V2 generator uses Y for all limb axes.
    v2_axis=(0,1,0)
    rejection={}
    for bone,d in bone_directions.items():
        dot=abs(sum(a*b for a,b in zip(d,v2_axis)))
        rejection[bone]=round(dot,6)
        assert dot>0.85,(bone,dot)
    # A conservative proposed source-space sagittal hinge axis is X,
    # but DOES NOT authorize UE track generation until imported basis test.
    proposed_source=(1,0,0)
    proposal={}
    for bone,d in bone_directions.items():
        dot=abs(sum(a*b for a,b in zip(d,proposed_source)))
        proposal[bone]=round(dot,6)
        assert dot<0.25,(bone,dot)
    # A direction classifier does not replace imported real animations.
    result={"status":"PASS_PREIMPLEMENTATION_DIRECTION_TESTS_V2_SOURCE_JOINT_REJECTION",
            "world_to_actor_cases":rows,"case_count":len(rows),
            "fixture_source_rig":str(source),"v2_Y_longitudinal_dot":rejection,
            "source_X_geometrically_perpendicular_dot":proposal,
            "unreal_imported_local_joint_axes":"NOT_VERIFIED",
            "mesh_component_visible_forward":"NOT_VERIFIED",
            "jump_airborne_full_pose":"NOT_AUTHORED",
            "natural_directional_locomotion_clips":"NOT_AUTHORED",
            "final_motion_quality":"REJECTED",
            "mobile_runtime_new_build":"NOT_CREATED",
            "utc":time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())}
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    return result
if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--source-glb",type=pathlib.Path,default=SOURCE)
    args=parser.parse_args()
    print(json.dumps(run(args.source_glb),indent=2))
