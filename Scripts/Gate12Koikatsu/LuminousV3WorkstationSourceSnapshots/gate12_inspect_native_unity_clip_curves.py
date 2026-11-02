"""Read-only inspect real Action animator Unity AnimationClip binding/curve encoding."""
import pathlib,sys,json,hashlib,time
ROOT=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty")
sys.path.insert(0,str(ROOT/"pipeline/vendor_unitypy"))
import UnityPy
source=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party\abdata\action\animator\14.unity3d")
OUT=ROOT/"characters/LUMINOUS_VALENTINE_V3/gate12_fresh_export_20261011"
env=UnityPy.load(str(source))
selected={}
for obj in env.objects:
 if obj.type.name!="AnimationClip":continue
 clip=obj.read()
 name=clip.m_Name
 if name in ("f_suiei_00_02_03_S","f_tachi_00_18_L") or (name.startswith("f_suiei") and len(selected)<3):
  fields={}
  for key in ("m_Legacy","m_Compressed","m_UseHighQualityCurve","m_SampleRate","m_WrapMode",
              "m_RotationCurves","m_EulerCurves","m_PositionCurves","m_ScaleCurves","m_FloatCurves",
              "m_PPtrCurves","m_MuscleClip","m_ClipBindingConstant"):
   val=getattr(clip,key,None)
   if isinstance(val,(int,float,bool,str)):fields[key]=val
   elif isinstance(val,(list,tuple,dict)):fields[key]={"container_type":type(val).__name__,"count":len(val)}
   elif val is not None:
    attrs={}
    for a in ("m_Clip","m_DeltaPose","m_StartX","m_StopX","m_AverageSpeed","m_CycleOffset",
              "m_StreamedClip","m_DenseClip","m_ConstantClip",
              "m_AnimationClipSettings","m_ClipBindingConstant","genericBindings"):
     nested=getattr(val,a,None)
     if nested is None:continue
     if isinstance(nested,(int,float,bool,str)):attrs[a]=nested
     elif isinstance(nested,(list,tuple,dict)):attrs[a]={"type":type(nested).__name__,"count":len(nested)}
     else:
      sub={}
      for k in ("m_FrameCount","m_CurveCount","m_SampleRate","m_BeginTime","m_Data","m_SampleArray","genericBindings"):
       v=getattr(nested,k,None)
       if v is None:continue
       sub[k]=len(v) if isinstance(v,(list,tuple,dict)) else v if isinstance(v,(int,float,bool,str)) else type(v).__name__
      attrs[a]={"type":type(nested).__name__,**sub}
    fields[key]={"type":type(val).__name__,"contents":attrs}
  selected[name]={"path_id":int(obj.path_id),"fields":fields}
  print("INSPECTED",name,"SAMPLE_RATE",fields.get("m_SampleRate"),"MUSCLE",fields.get("m_MuscleClip"),flush=True)
 if len(selected)>=3:break
out={"status":"PASS_UNITY_NATIVE_SERIALIZED_ANIMATION_CURVE_STRUCTURE_READ","source":str(source),
     "source_sha256":hashlib.sha256(source.read_bytes()).hexdigest(),
     "engine":"Unity 5.x -> retargeting still required","selected_clips":selected,
     "unreal_anim_sequences_generated":False}
(OUT/"KOIKATSU_ANIMATION_NATIVE_CURVE_STRUCTURE.json").write_text(json.dumps(out,indent=2,default=str)+"\n",encoding="utf8")
print("SOURCE_CURVE_STRUCTURE_WRITTEN",len(selected))
