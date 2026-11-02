"""Gate12 post-freeze Motion V3: parse genuine BVH mocap and calibrate glTF retarget.

NO Unreal assets are modified. NO guess that glTF rotations equal UE rotations.
The first BVH frame is Bruce Hahne's rest/T-pose, NOT a motion sample.
Original GLB/texture/morph/skeleton data stay byte-for-byte unchanged.
"""
from __future__ import annotations
import dataclasses
import json
import math
import pathlib
import re
import struct
from typing import Dict, List, Tuple

V3_MAP={
    "Hips":"pelvis",
    "LeftUpLeg":"thigh_l","LeftLeg":"calf_l","LeftFoot":"Left ankle","LeftToeBase":"ball_l",
    "RightUpLeg":"thigh_r","RightLeg":"calf_r","RightFoot":"Right ankle","RightToeBase":"ball_r",
    "LowerBack":"spine_01","Spine":"spine_02","Spine1":"spine_03",
    "Neck":"neck","Head":"head",
    "LeftShoulder":"clavicle_l","LeftArm":"upperarm_l",
    "LeftForeArm":"lowerarm_l","LeftHand":"hand_l",
    "RightShoulder":"clavicle_r","RightArm":"upperarm_r",
    "RightForeArm":"lowerarm_r","RightHand":"hand_r",
}
IDENT=(0.,0.,0.,1.)
def q_norm(a):
    mag=math.sqrt(sum(x*x for x in a))
    if mag<1e-12:raise ValueError("Zero quaternion")
    return tuple(x/mag for x in a)
def q_mul(a,b):
    x,y,z,w=a;X,Y,Z,W=b
    return (w*X+x*W+y*Z-z*Y,w*Y-x*Z+y*W+z*X,
            w*Z+x*Y-y*X+z*W,w*W-x*X-y*Y-z*Z)
def q_conj(q):
    return (-q[0],-q[1],-q[2],q[3])
def q_axis(name,theta_deg):
    theta=math.radians(theta_deg)*.5
    s=math.sin(theta);c=math.cos(theta)
    return {"Xrotation":(s,0.,0.,c),
            "Yrotation":(0.,s,0.,c),
            "Zrotation":(0.,0.,s,c)}[name]
def q_rotate(q,v):
    vx,vy,vz=v
    out=q_mul(q_mul(q,(vx,vy,vz,0.)),q_conj(q))
    return out[:3]
def v_add(a,b):
    return tuple(x+y for x,y in zip(a,b))
def v_sub(a,b):
    return tuple(x-y for x,y in zip(a,b))
def v_dot(a,b):
    return sum(x*y for x,y in zip(a,b))
def v_len(a):
    return math.sqrt(v_dot(a,a))
def v_norm(a):
    l=v_len(a)
    return tuple(x/l for x in a) if l>1e-10 else (0.,0.,0.)
def angle_between(a,b):
    return math.degrees(math.acos(max(-1.,min(1.,v_dot(v_norm(a),v_norm(b))))))
def q_angle_degrees(q):
    return math.degrees(2*math.acos(min(1.,abs(q_norm(q)[3]))))

@dataclasses.dataclass(frozen=True)
class BVHJoint:
    name:str
    parent:str|None
    offset:tuple[float,float,float]
    channels:tuple[str,...]
    start:int

class BVH:
    def __init__(self,path:pathlib.Path):
        self.path=path
        content=path.read_text(encoding="utf-8-sig")
        assert content.startswith("HIERARCHY\n") or content.startswith("HIERARCHY\r\n")
        hierarchy, motion=content.split("\nMOTION\n",1)
        self._parse_hierarchy(hierarchy)
        lines=motion.splitlines()
        n=re.fullmatch(r"\s*Frames:\s*(\d+)\s*",lines[0])
        time=re.fullmatch(r"\s*Frame Time:\s*([\d.]+)\s*",lines[1])
        if not n or not time:raise ValueError("Invalid BVH MOTION header")
        self.fps=1/float(time.group(1))
        self.frames=[tuple(float(v) for v in row.split()) for row in lines[2:] if row.strip()]
        assert len(self.frames)==int(n.group(1)),(len(self.frames),n.group(1))
        assert all(len(f)==self.channel_count for f in self.frames)
        assert self.frames and 0.001<1/self.fps<.1
        self.ref_pose=self.pose(0)
    def _parse_hierarchy(self,header):
        tokens=re.findall(r"[{}]|[^\s{}]+",header)
        it=iter(tokens)
        assert next(it)=="HIERARCHY"
        self.joints={}
        channel_count=0
        def need(expected):
            word=next(it)
            if word!=expected:raise ValueError(f"Expected {expected}; found {word}")
        def joint(kind,parent):
            nonlocal channel_count
            assert kind in ("ROOT","JOINT")
            name=next(it);need("{");need("OFFSET")
            offset=tuple(float(next(it)) for _ in range(3))
            need("CHANNELS")
            n=int(next(it))
            channels=tuple(next(it) for _ in range(n))
            if name in self.joints:raise ValueError("Duplicate BVH joint "+name)
            self.joints[name]=BVHJoint(name,parent,offset,channels,channel_count)
            channel_count+=n
            while True:
                command=next(it)
                if command=="}":break
                if command in ("ROOT","JOINT"):joint(command,name)
                elif command=="End":
                    need("Site");need("{");need("OFFSET")
                    for _ in range(3):float(next(it))
                    need("}")
                else:raise ValueError("Bad BVH token "+command)
        joint(next(it),None)
        self.channel_count=channel_count
    def pose(self,frame:int):
        numbers=self.frames[frame]
        glob_q={}
        glob_p={}
        local_rot={}
        for name,joint in self.joints.items():
            q=IDENT;tr=list(joint.offset)
            for i,channel in enumerate(joint.channels):
                value=numbers[joint.start+i]
                if channel.endswith("rotation"):
                    q=q_norm(q_mul(q,q_axis(channel,value)))
                elif channel.endswith("position"):
                    tr[{"Xposition":0,"Yposition":1,"Zposition":2}[channel]]=value
            local_rot[name]=q
            if joint.parent is None:
                glob_q[name]=q
                glob_p[name]=tuple(tr)
            else:
                parent=joint.parent
                glob_q[name]=q_norm(q_mul(glob_q[parent],q))
                glob_p[name]=v_add(glob_p[parent],q_rotate(glob_q[parent],tr))
        return {"local_q":local_rot,"global_q":glob_q,"global_p":glob_p}

class GLTFRig:
    def __init__(self,path:pathlib.Path):
        self.path=path
        b=path.read_bytes()
        if b[:4]!=b"glTF":raise ValueError("Not a GLB")
        size,token=struct.unpack_from("<II",b,12)
        if token!=0x4e4f534a:raise ValueError("Unexpected GLB chunk")
        meta=json.loads(b[20:20+size])
        self.nodes=meta["nodes"]
        self.by_name={node["name"]:i for i,node in enumerate(self.nodes) if "name" in node}
        self.parents={}
        for i,node in enumerate(self.nodes):
            for child in node.get("children",[]):
                self.parents[child]=i
        self.global_q={}
        self.global_p={}
        self.local_q={}
        for i in range(len(self.nodes)):
            self.compute_global(i)
    def compute_global(self,i:int):
        if i in self.global_q:return
        node=self.nodes[i]
        if "matrix" in node:raise ValueError("Matrix-only node needs TRS decomposition")
        q=q_norm(node.get("rotation",IDENT))
        t=tuple(node.get("translation",(0.,0.,0.)))
        self.local_q[i]=q
        if i in self.parents:
            par=self.parents[i]
            self.compute_global(par)
            self.global_q[i]=q_norm(q_mul(self.global_q[par],q))
            self.global_p[i]=v_add(self.global_p[par],q_rotate(self.global_q[par],t))
        else:
            self.global_q[i]=q
            self.global_p[i]=t
    def names(self):
        return set(self.by_name)
    def mapped_pose(self,bvh:BVH,frame:int,include_root_translation=False):
        # Animate *relative* world-space bone orientation changes from the
        # authentic BVH rest frame, then attach to exact source GLB rest.
        # Target root translations intentionally use original intact GLB.
        src=bvh.pose(frame)
        mapped={target:source for source,target in V3_MAP.items()}
        local={}
        world_q={}
        world_p={}
        for i in range(len(self.nodes)):
            self._solve_node(i,local,world_q,world_p,src,bvh,mapped)
        return {"local_q":local,"global_q":world_q,"global_p":world_p,
                "bvh_source_frame":frame,"original_source_rig":str(self.path),
                "root_translation_applied":False}
    def _solve_node(self,i,local,world_q,world_p,src,bvh,mapped):
        if i in world_q:return
        node=self.nodes[i];name=node.get("name")
        if i in self.parents:
            par=self.parents[i]
            self._solve_node(par,local,world_q,world_p,src,bvh,mapped)
            parent_q=world_q[par]
            p=world_p[par]
            pos=v_add(p,q_rotate(parent_q,node.get("translation",(0.,0.,0.))))
        else:
            par=None
            parent_q=IDENT
            pos=tuple(node.get("translation",(0.,0.,0.)))
        if name in mapped and mapped[name] in src["global_q"]:
            joint=mapped[name]
            delta=q_mul(src["global_q"][joint],q_conj(bvh.ref_pose["global_q"][joint]))
            desired=q_norm(q_mul(delta,self.global_q[i]))
            q_local=q_norm(q_mul(q_conj(parent_q),desired))
        else:
            q_local=self.local_q[i]
            desired=q_norm(q_mul(parent_q,q_local))
        local[name if name else str(i)]=q_local
        world_q[i]=desired
        world_p[i]=pos

def knee_flex(pose,upper,knee,ankle):
    p=pose["global_p"]
    a=v_sub(p[knee],p[upper]);b=v_sub(p[ankle],p[knee])
    return angle_between(a,b)
def source_knee_flex(bvh,frame,side):
    d=bvh.pose(frame)
    return knee_flex(d,"LeftUpLeg" if side=="l" else "RightUpLeg",
                     "LeftLeg" if side=="l" else "RightLeg",
                     "LeftFoot" if side=="l" else "RightFoot")
def target_knee_flex(rig,pose,side):
    idx=rig.by_name
    p={name:pose["global_p"][i] for name,i in idx.items()}
    return knee_flex({"global_p":p},"thigh_"+side,"calf_"+side,
                     "Left ankle" if side=="l" else "Right ankle")
