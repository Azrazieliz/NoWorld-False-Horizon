"""Exact Unity AnimationClip curve extraction proof for Koikatsu Party.

Source: installed game abdata/action/animator/14.unity3d, genuine clips.
Preserves bindings+streamed/dense/constant channel data without fabricating
rotations, durations or bone identifiers. Transform-hash->bone hierarchy
resolution/Blender baking/Unreal retarget remain independent later gates.
"""
import pathlib,sys,json,hashlib,gzip,struct,time,collections
BASE=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty")
sys.path.insert(0,str(BASE/"pipeline/vendor_unitypy"))
import UnityPy
GAME=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party")
SOURCE=GAME/"abdata/action/animator/14.unity3d"
OUT=BASE/"characters/LUMINOUS_VALENTINE_V3/gate12_fresh_export_20261011/ANIMATION_REUSE"
OUT.mkdir(exist_ok=True,parents=True)
TARGETS={"f_tachi_00_18_L":"STAND_IDLE","f_suiei_00_02_03_S":"SWIM"}
def digest(p):
 h=hashlib.sha256()
 with p.open("rb") as f:
  for block in iter(lambda:f.read(4*1024*1024),b""):h.update(block)
 return h.hexdigest()
env=UnityPy.load(str(SOURCE))
sourcehash=digest(SOURCE)
report={"schema":"G12_KOIKATSU_ACTUAL_UNITY_ANIM_CURVE_SOURCE_PROOF_1",
 "status":"START","source_bundle":str(SOURCE),"source_bundle_sha256":sourcehash,
 "engine_origin":"Koikatsu Party Unity game bundles","output_provenance_restricted_local_only":True,
 "samples":{},"native_Blender_animation_baked":False,"native_UE_anim_sequence":False,
 "source_animation_runtime_allowed":"PENDING_RIG_MAPPING_AND_VISUAL_QA"}
for obj in env.objects:
 if obj.type.name!="AnimationClip":continue
 clip=obj.read();name=clip.m_Name
 if name not in TARGETS:continue
 data=clip.m_MuscleClip.m_Clip.data
 streamed=data.m_StreamedClip
 dense=data.m_DenseClip
 constant=data.m_ConstantClip
 bindings=clip.m_ClipBindingConstant.genericBindings
 samples=list(dense.m_SampleArray)
 assert isinstance(samples,list) and len(samples)>100
 assert len(samples)==dense.m_FrameCount*dense.m_CurveCount,(name,len(samples),dense.m_FrameCount,dense.m_CurveCount)
 def plain(o):
  return {field:getattr(o,field,None) for field in ("path","attribute","typeID","customType") if hasattr(o,field)}
 channels=[plain(x) for x in bindings]
 bundle={
  "clip_name":name,"purpose":TARGETS[name],
  "unity_bundle_sha256":sourcehash,
  "animation_path_id":int(obj.path_id),
  "sample_rate":float(clip.m_SampleRate),
  "source_curve_layout":"UNITY_5_X_NATIVE_STREAM_DENSE_CONST",
  "native_bindings":channels,
  "streamed":{"curve_count":int(streamed.curveCount),"packed_u32":list(streamed.data)},
  "dense":{"begin_time":float(dense.m_BeginTime),
           "curve_count":int(dense.m_CurveCount),
           "frame_count":int(dense.m_FrameCount),
           "sample_rate":float(dense.m_SampleRate),
           "float_samples":samples},
  "constant":{"float_values":list(constant.data)},
  "m_MuscleClip_start_time":float(clip.m_MuscleClip.m_StartTime),
  "m_MuscleClip_stop_time":float(clip.m_MuscleClip.m_StopTime),
  "blender_skeletal_animation_baked":False,
  "hash_to_named_rig_bindings_resolved":False,
  "no_source_joint_curves_fabricated":True}
 blob=json.dumps(bundle,separators=(",",":"),ensure_ascii=True).encode("utf8")
 dst=OUT/(TARGETS[name]+"_"+name+".unitycurves.json.gz")
 if dst.exists():raise RuntimeError("Refuse overwriting original captured Unity clip data "+str(dst))
 with gzip.open(dst,"wb",compresslevel=6) as f:f.write(blob)
 with gzip.open(dst,"rb") as f:assert f.read()==blob
 info={"source_clip_name":name,"role":TARGETS[name],
       "sample_rate":bundle["sample_rate"],
       "native_binding_entries":len(channels),
       "dense_frames":dense.m_FrameCount,
       "dense_float_curves":dense.m_CurveCount,
       "streamed_float_curves":streamed.curveCount,
       "constant_values":len(constant.data),
       "output":str(dst),"output_sha256":digest(dst),
       "blender_or_unreal_baked":False}
 report["samples"][TARGETS[name]]=info
 print("UNITY_CURVE_CAPTURE",TARGETS[name],info,flush=True)
(OUT/"G12_NATIVE_KK_ANIMATION_REUSE_CURVE_INTAKE.json").write_text(
 json.dumps({**report,"status":"PASS_REAL_NATIVE_KOIKATSU_STANDING_AND_SWIM_FLOAT_CHANNEL_EXTRACTION"
            if len(report["samples"])==len(TARGETS) else "FAIL_INCOMPLETE_CURVE_PROOF"},
           indent=2)+"\n",encoding="utf8")
assert len(report["samples"])==len(TARGETS)
print("KOIKATSU_REAL_ANIM_NATIVE_FLOAT_CURVE_EXTRACTION_PASS_2",flush=True)
