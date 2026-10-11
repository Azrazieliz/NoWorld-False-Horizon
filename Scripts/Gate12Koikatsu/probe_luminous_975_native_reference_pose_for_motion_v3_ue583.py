"""Read-only calibration of actual Luminous UE 975-joint rest versus immutable source GLB.
No animation or skeletal mesh reimport. Gate12 MotionV3 strict native basis.
"""
import unreal,pathlib,json,hashlib,math,struct,traceback,time
W=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
Q=W/"Saved/Gate12Koikatsu/LuminousV3"
r=json.loads((Q/"CLOTHED/native_shards_001_count04_ue583.json").read_text("utf-8-sig"))
part=next(x for x in r["native_parts"] if int(x["index"])==3)
mesh_path=next(x["path"] for x in part["assets"] if x["class"]=="SkeletalMesh")
out=Q/"luminous_975_ue_native_refpose_calibration_v3.json"
targets=["Hips","Left leg","Left knee","Left ankle","Left toe","Right leg","Right knee","Right ankle","Right toe",
 "Spine","Chest","Upper Chest","Neck","Head","Left shoulder","Left arm","Left elbow","Left wrist",
 "Right shoulder","Right arm","Right elbow","Right wrist"]
def norm(q):
 m=math.sqrt(sum(x*x for x in q));return tuple(x/m for x in q)
def angle(q,p):
 d=min(1,abs(sum(x*y for x,y in zip(norm(q),norm(p)))))
 return math.degrees(2*math.acos(d))
result={"status":"START","source_immutable":True,"target_bones":targets,"rows":[]}
try:
 source=pathlib.Path(part["source"])
 if hashlib.sha256(source.read_bytes()).hexdigest()!=part["sha256"]:raise RuntimeError("Source SHA drift")
 raw=source.read_bytes();size,jkind=struct.unpack_from("<II",raw,12)
 meta=json.loads(raw[20:20+size]);src={x.get("name"):x.get("rotation",[0,0,0,1]) for x in meta["nodes"]}
 asset=unreal.load_asset(mesh_path)
 if not isinstance(asset,unreal.SkeletalMesh):raise RuntimeError("Native skeletal mesh missing")
 sk=asset.get_editor_property("skeleton")
 ref=sk.get_reference_pose()
 names=set(str(x) for x in ref.get_bone_names())
 result["native_anatomical_name_examples"]=[x for x in sorted(names) if any(z in x.lower() for z in ("left","right","hips","pelvis","chest","neck"))][:110]
 result["native_skeleton"]=sk.get_path_name();result["native_bone_count"]=len(names)
 if len(names)!=975:raise RuntimeError("Native 975-bone skeleton requirement violated")
 for name in targets:
  native_name=name.replace(" ","-")
  if name not in src or native_name not in names:raise RuntimeError("Mapped anatomical target missing from source or native: "+name+"="+native_name)
  s=src[name];expected=(-s[0],-s[2],-s[1],s[3])
  t=ref.get_bone_pose(unreal.Name(native_name),unreal.AnimPoseSpaces.LOCAL)
  q=t.rotation
  actual=(q.x,q.y,q.z,q.w)
  error=angle(expected,actual)
  result["rows"].append({"bone":name,"source_quat":s,"expected_native_quat":expected,
    "actual_ue_quat":actual,"angular_error_deg":error})
 result["max_error_deg"]=max(x["angular_error_deg"] for x in result["rows"])
 result["status"]="PASS_MOTION_V3_NATIVE_975_QUAT_BASIS" if result["max_error_deg"]<0.15 else "FAIL_NATIVE_975_QUAT_BASIS_UNCALIBRATED"
except Exception as e:
 result["status"]="FAIL_NATIVE_975_REFERENCE_POSE_PROBE"
 result["exception"]=str(e);result["trace"]=traceback.format_exc()[-3600:]
finally:
 result["updated_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())
 out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf8")
 unreal.log("G12_LUMINOUS_975_REFPOSE="+result["status"])
