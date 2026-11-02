"""Decode REAL installed Koikatsu Studio animation streams onto a known
Koikatsu character's native transform-bone identity.
Prepares numeric source-local quaternion deltas, not fabricated poses.
No asset, character or source file is modified or silently approved.
"""
import collections,gzip,hashlib,json,math,pathlib,re,struct,sys,bisect,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
G=pathlib.Path(r"D:\UnrealProjects\Gate09_Protected_Production_20261016")
sys.path.insert(0,str(R/"Scripts/Gate12Koikatsu"))
from gate12_import_all_gate09_koikatsu_unity_source_curves_20261021 import load_env
INDEX=json.loads((R/"SourceAssets/Gate12/Animations20261021/ImportedGate09OriginalUnityCurves/GATE09_ALL_SOURCE_INTAKE_PROGRESS.json").read_text())
MAP=json.loads((G/"G09_Unity_Native_Transform_Hash_Map_20261019.json").read_text())["resolved"]
BASE=r"D:\Tools\Steam\steamapps\common\Koikatsu Party\abdata\chara\oo_base.unity3d"
OUT=R/"Saved/Gate12Readiness/KoikatsuGenuineNativeStudioMotion"
OUT.mkdir(parents=True,exist_ok=True)
EXTRA={
"cf_j_root":"Center","cf_j_hips":"Hips",
"cf_j_spine01":"Spine","cf_j_spine02":"Chest","cf_j_spine03":"Upper-Chest",
"cf_j_neck":"Neck","cf_j_head":"Head",
"cf_j_thigh00_L":"Left-leg","cf_j_leg01_L":"Left-knee","cf_j_leg03_L":"Left-ankle",
"cf_j_thigh00_R":"Right-leg","cf_j_leg01_R":"Right-knee","cf_j_leg03_R":"Right-ankle",
"cf_j_arm00_L":"Left-arm","cf_j_forearm01_L":"Left-elbow","cf_j_hand_L":"Left-wrist",
"cf_j_arm00_R":"Right-arm","cf_j_forearm01_R":"Right-elbow","cf_j_hand_R":"Right-wrist",
"cf_j_shoulder_L":"Left-shoulder","cf_j_shoulder_R":"Right-shoulder"
}
# Measured from the original Alter Unreal Skeleton (not inferred generic Mixamo
# aliases). Preserves original source identities and restores finger articulation.
for _side in ("L","R"):
 for _family,_target in (("index","IndexFinger"),("little","LittleFinger"),
                          ("middle","MiddleFinger"),("ring","RingFinger")):
  for _joint in (1,2,3):
   EXTRA[f"cf_j_{_family}{_joint:02d}_{_side}"]=f"{_target}{_joint}_{_side}"
 for _joint in (1,2,3):
  EXTRA[f"cf_j_thumb{_joint:02d}_{_side}"]=f"Thumb{_joint-1}_{_side}"
 EXTRA[f"cf_j_toes_{_side}"]="Left-toe" if _side=="L" else "Right-toe"

def uint_float(n):return struct.unpack("<f",struct.pack("<I",n))[0]
def norm(q):
 z=math.sqrt(sum(v*v for v in q))
 return tuple(v/z for v in q) if z>1e-10 else (0,0,0,1)
def qm(a,b):
 x,y,z,w=a;X,Y,Z,W=b
 return (w*X+x*W+y*Z-z*Y,w*Y-x*Z+y*W+z*X,
         w*Z+x*Y-y*X+z*W,w*W-x*X-y*Y-z*Z)
def qi(a): x,y,z,w=norm(a);return (-x,-y,-z,w)
def rotate_delta(q):
 x,y,z,w=norm(q);return norm((-x,-z,-y,w))
def qdiff(q):return math.degrees(2*math.acos(min(1,abs(norm(q)[3]))))
def sample_streamed(words,curve_count,times):
 # Exact binary Unity StreamedClip frames. Each coefficient is the original
 # cubic segment (a,b,c,d) in source-native local-curve units.
 frames=collections.defaultdict(list)
 i=0
 while i<len(words):
  t=uint_float(words[i]);c=words[i+1];i+=2
  assert 0<=c<=curve_count and i+c*5<=len(words)
  for _ in range(c):
   index=words[i];coef=tuple(uint_float(w) for w in words[i+1:i+5]);i+=5
   assert 0<=index<curve_count
   frames[index].append((t,coef))
 assert i==len(words)
 out=[[]for _ in range(curve_count)]
 for index in range(curve_count):
  values=frames[index]
  assert values,"Original streamed channel has no samples"
  stamps=[v[0] for v in values]
  assert all(stamps[i]<=stamps[i+1] for i in range(len(stamps)-1))
  for t in times:
   k=max(0,bisect.bisect_right(stamps,t)-1)
   s,coef=values[k]
   dt=t-s if s>-1e12 else 0.0
   a,b,c,d=coef
   v=((a*dt+b)*dt+c)*dt+d
   assert math.isfinite(v)
   out[index].append(v)
 return out
def source_bone_catalog():
 env,_=load_env(BASE)
 needed={entry["node"]for vals in MAP.values()for entry in vals}
 byid={}
 for obj in env.objects:
  if obj.type.name=="Transform" and obj.path_id in needed:
   t=obj.read()
   q=t.m_LocalRotation
   byid[obj.path_id]=(float(q.x),float(q.y),float(q.z),float(q.w))
 assert len(byid)>130
 del env
 return byid
def widths(b):
 tid=int(b.get("typeID",0));attr=int(b.get("attribute",0))
 if tid==4:return 3 if attr in (1,3) else 4 if attr==2 else 1
 return 1
def convert(clipname,rest,source_row=None,output_alias=None,rig_family="koikatsu_full"):
 rows=([source_row] if source_row is not None else
       [f for f in INDEX["source_files"] if f["source_clip_name"]==clipname and
        f["source_bundle_index"]<10])
 assert len(rows)==1,("Choose one exact bundled source",clipname,len(rows))
 row=rows[0]
 raw=pathlib.Path(row["raw_curve_path"])
 assert raw.is_file()
 with gzip.open(raw,"rt",encoding="utf8")as f:obj=json.load(f)
 assert obj["kind"]=="UNRETARGETED_SOURCE_ANIMATION_CLIP"
 assert obj["name"]==clipname and obj["source_bundle_data_sha256"]==row["source_hash"]
 assert obj["source_sample_rate"]>0 and obj["duration_seconds"]>0
 binds=obj["generic_bindings"]
 count0=obj["streamed"]["curve_count"]
 count1=obj["dense"]["curve_count"]
 count2=len(obj["constant_float_values"])
 total=sum(widths(b)for b in binds)
 assert total==count0+count1+count2,(total,count0,count1,count2)
 rate=min(30.0,float(obj["source_sample_rate"]))
 n=max(2,round(obj["duration_seconds"]*rate)+1)
 timeline=[min(i/rate,obj["duration_seconds"])for i in range(n)]
 stream=sample_streamed(obj["streamed"]["packed_u32"],count0,timeline)
 dense=obj["dense"]
 assert len(dense["float_samples"])==int(dense["frame_count"])*count1
 def get_channel(idx):
  if idx<count0:return stream[idx]
  if idx<count0+count1:
   x=idx-count0;first=dense["begin_time"];freq=dense["sample_rate"]
   vals=dense["float_samples"];frames=int(dense["frame_count"])
   return [float(vals[(lambda a:a*count1+x)(min(frames-1,max(0,round((t-first)*freq))))])for t in timeline]
  return [obj["constant_float_values"][idx-count0-count1]]*n
 records=[]
 offset=0
 for b in binds:
  sz=widths(b)
  if b["typeID"]==4 and b["attribute"]==2:
   path=str(b["path"])
   resolutions=MAP.get(path,[])
   matching=next((x for x in resolutions if x["node"]in rest),None)
   if matching is not None:
    leaf=matching["path"].split("/")[-1]
    target=EXTRA.get(leaf,leaf)
    original=norm(rest[matching["node"]])
    chan=[get_channel(offset+k)for k in range(4)]
    frames=[]
    for fi in range(n):
     q=norm(tuple(float(chan[k][fi])for k in range(4)))
     delta=qm(qi(original),q)
     outq=rotate_delta(delta)
     assert abs(sum(x*x for x in outq)-1)<.0001
     frames.append([round(v,7)for v in outq])
    if all(math.isfinite(v)for x in frames for v in x):
     records.append({"hash":path,"source_bone":leaf,"target_bone":target,
                     "rest_node":matching["node"],"deltas":frames,
                     "angular_span_deg":round(max(qdiff(qm(qi(frames[0]),q))for q in frames),3)})
  offset+=sz
 assert offset==total
 # Avoid applying ambiguous overlapping transforms to a target bone.
 seen=set();channels=[]
 for r in records:
  if r["target_bone"] in seen:continue
  seen.add(r["target_bone"]);channels.append(r)
 changed=sum(x["angular_span_deg"]>.05 for x in channels)
 if rig_family=="mixamo_koikatsu_compatible":
  # Original Mixamo-for-CharaStudio sources only animate a documented 93-bone
  # reduced rig. Never misrepresent it as full Koikatsu facial/secondary data.
  essentials={"Center","Hips","Spine","Chest","Upper-Chest","Neck","Head",
   "Left-arm","Right-arm","Left-elbow","Right-elbow","Left-wrist","Right-wrist",
   "Left-leg","Right-leg","Left-knee","Right-knee","Left-ankle","Right-ankle"}
  assert len(channels)>=80 and changed>=10 and essentials.issubset(seen),(len(channels),changed,sorted(essentials-seen))
 else:
  assert len(channels)>=140 and changed>=12,(len(channels),changed)
 assert output_alias is None or re.fullmatch(r"[A-Za-z0-9_]{3,100}",output_alias)
 output=OUT/((output_alias or clipname)+".genuine_unity_kk_stream_native_deltas.json.gz")
 assert not output.exists(),"Do not overwrite extracted source motion"
 result={
   "schema":"noworld.gate12.koikatsu.uncooked_original_studio_unity_native_motion.v1",
   "status":"SOURCE_AUTHENTIC_QUATERNION_TRACKS_CALIBRATED_FOR_NATIVE_UE_REST_BASIS",
   "clip":output_alias or clipname,"original_source_clip":clipname,
   "rig_family":rig_family,
   "source_path_id":obj["original_unity_path_id"],
   "source_bundle":obj["source_bundle"],"source_sha256":row["source_hash"],
   "source_clip_sha256":hashlib.sha256(raw.read_bytes()).hexdigest(),
   "source_curve_path":str(raw),"frame_rate":rate,
   "frame_count":n,"original_duration_s":obj["duration_seconds"],
   "source_transform_bindings":len(binds),
   "source_streamed_float_channels":count0,"source_dense_float_channels":count1,
   "source_constant_float_channels":count2,
   "resolved_rotation_bone_count":len(channels),
   "animated_rotation_bone_count":changed,
   "relative_source_pose_only":True,"pairing_approved":False,"visual_accepted":False,
   "original_sources_untouched":True,
   "native_ue_sequence_created":False,"bones":channels
 }
 blob=json.dumps(result,separators=(",",":"),allow_nan=False).encode("utf8")
 with gzip.open(output,"wb",compresslevel=4) as f:f.write(blob)
 print("CALIBRATED_KK_REAL_ORIGINAL",clipname,"SOURCE_TRACKS",len(channels),"CHANGED",changed,"OUTPUT",output,flush=True)
 return result
if __name__=="__main__":
 rest=source_bone_catalog()
 passed=0
 for clip in sys.argv[1:]:
  try:
   f=OUT/(clip+".genuine_unity_kk_stream_native_deltas.json.gz")
   if f.exists():
    print("PRESERVED_EXISTING_REAL_KK_SOURCE",clip,flush=True)
    passed+=1
   else:
    convert(clip,rest)
    passed+=1
  except Exception:
   print("NOT_CONVERTED_SOURCE",clip,traceback.format_exc()[-850:],flush=True)
 print("ORIGINAL_KK_DECODED_OR_PRESERVED",passed,"OF",len(sys.argv[1:]),flush=True)
