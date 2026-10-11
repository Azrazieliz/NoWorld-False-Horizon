"""Read-only probe of genuine installed Koikatsu Action/Studio Unity AnimationClip assets."""
import sys,pathlib,json,collections,time,traceback,hashlib,os
BASE=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty")
sys.path.insert(0,str(BASE/"pipeline/vendor_unitypy"))
import UnityPy
GAME=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party")
OUT=BASE/"characters/LUMINOUS_VALENTINE_V3/gate12_fresh_export_20261011"
paths=[
 "abdata/studio/anime/07.unity3d",
 "abdata/studio/anime/12.unity3d",
 "abdata/studio/anime/02.unity3d",
 "abdata/action/animator/14.unity3d",
]
results={"schema":"G12_KOIKATSU_ANIMATION_SOURCE_PROBE_1",
         "method":"read_only_UnityPy_1.25.4_analyze_installed_Unreal_not_authoring",
         "time":time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime()),"bundles":[]}
for rel in paths:
 f=GAME/rel
 if not f.is_file():continue
 row={"path":str(f),"bytes":f.stat().st_size,"sha256":hashlib.sha256(f.read_bytes()).hexdigest(),
      "status":"START","sample_AnimationClip_objects":[]}
 started=time.monotonic()
 try:
  env=UnityPy.load(str(f))
  counts=collections.Counter(o.type.name for o in env.objects)
  row["unity_object_types"]=dict(counts)
  samples=[]
  for obj in env.objects:
   if obj.type.name!="AnimationClip":continue
   item={"path_id":int(obj.path_id)}
   try:
    cls=obj.read()
    item["name"]=getattr(cls,"m_Name",None)
    item["frame_rate"]=getattr(cls,"m_SampleRate",None)
    for key in ("m_Legacy","m_Compressed","m_WrapMode"):
     v=getattr(cls,key,None)
     if isinstance(v,(bool,int,float,str)):item[key]=v
    item["parsed"]=True
   except Exception as e:item["parsed"]=False;item["error"]=repr(e)[:250]
   samples.append(item)
   if len(samples)>=10:break
  row["sample_AnimationClip_objects"]=samples
  row["AnimationClip_count"]=counts.get("AnimationClip",0)
  row["status"]="PASS_READONLY_REAL_UNITY_ASSET_INSPECTION"
 except Exception as e:
  row["status"]="FAIL_UNITY_BUNDLE_READ"
  row["error"]=repr(e);row["traceback"]=traceback.format_exc()[-1200:]
 row["elapsed_seconds"]=round(time.monotonic()-started,2)
 results["bundles"].append(row)
 print("UNITY_ANIMATION_BUNDLE",rel,row["status"],
       "AnimationClip_count",row.get("AnimationClip_count"),
       "firstclips",[s.get("name") for s in row["sample_AnimationClip_objects"][:6]],
       "elapsed",row["elapsed_seconds"],flush=True)
 (OUT/"KOIKATSU_INSTALLED_ANIMATION_CLIP_READONLY_PROBE.json").write_text(json.dumps(results,indent=2,default=str)+"\n",encoding="utf8")
print("RESULT",OUT/"KOIKATSU_INSTALLED_ANIMATION_CLIP_READONLY_PROBE.json")
