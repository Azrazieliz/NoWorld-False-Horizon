"""Gate09 -> Gate12: full Koikatsu installed Unity AnimationClip source intake.

Preserves exact original Unity streamed/dense/constant animation channels and
bindings for ALL categories (locomotion, sword, martial, spells, dance,
social, adult-context, death, paired interactions). No fake retargeting.

- Reads the authoritative Gate09 inventory, not guesses from file names.
- Reads protected .unity3d files and zipmod :: inner bundles in-place.
- Writes restricted, local, source-faithful .unitycurves.json.gz data.
- Resumable and never overwrites valid previously captured clip objects.
- Does NOT claim native Unreal AnimSequence/playback until rig mapping and
  visual acceptance are genuinely completed.
"""
import argparse,collections,gc,gzip,hashlib,json,math,os,pathlib,sys,tempfile,time,zipfile,traceback
ROOT=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
GATE09=pathlib.Path(r"D:\UnrealProjects\Gate09_Protected_Production_20261016")
INVENTORY=GATE09/"G09_InstalledAnimationInventory_ReadOnly_20261020.json"
VENDOR=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\pipeline\vendor_unitypy")
sys.path.insert(0,str(VENDOR))
import UnityPy

def sha_bytes(b): return hashlib.sha256(b).hexdigest()
def sha_file(path):
    h=hashlib.sha256()
    with open(path,"rb") as f:
        for block in iter(lambda:f.read(1024*1024),b""):h.update(block)
    return h.hexdigest()
def scalar(v,depth=0,seen=None):
    # UnityPy sometimes supplies self-referential PPtr objects. Preserve
    # deterministic known primitive curves without recursively chasing files.
    if v is None or isinstance(v,(bool,int,str)):return v
    if isinstance(v,float):return v if math.isfinite(v) else str(v)
    if isinstance(v,bytes):return {"encoded_hex":v.hex()}
    if depth>13:return {"unity_serialization_depth_limit":type(v).__name__}
    if seen is None:seen=set()
    oid=id(v)
    if oid in seen:return {"unity_reference_cycle":type(v).__name__}
    seen.add(oid)
    try:
        if isinstance(v,(list,tuple)):
            return [scalar(x,depth+1,seen) for x in v]
        if callable(getattr(v,"tolist",None)):
            return scalar(v.tolist(),depth+1,seen)
        if isinstance(v,dict):
            return {str(k):scalar(x,depth+1,seen) for k,x in v.items()}
        if hasattr(v,"m_FileID") and hasattr(v,"m_PathID"):
            return {"m_FileID":scalar(v.m_FileID),"m_PathID":scalar(v.m_PathID)}
        if hasattr(v,"__dict__"):
            return {k:scalar(x,depth+1,seen) for k,x in vars(v).items()
                if not k.startswith("_") and not callable(x)}
        fields=getattr(type(v),"__annotations__",None)
        if fields:
            return {k:scalar(getattr(v,k),depth+1,seen) for k in fields
                if hasattr(v,k) and not callable(getattr(v,k,None))}
        return str(v)
    finally:seen.remove(oid)
def binding_dict(b):
    return {field:scalar(getattr(b,field,None))
        for field in ("path","attribute","typeID","customType","isPPtrCurve",
                      "isIntCurve","isSerializeReferenceCurve","script")
        if getattr(b,field,None) is not None}

def cut_bundle(source):
    if "::" not in source:
        file=pathlib.Path(source)
        if not file.is_file():raise FileNotFoundError(str(file))
        return file,None
    zip_name,member=source.split("::",1)
    file=pathlib.Path(zip_name)
    if not file.is_file():raise FileNotFoundError(str(file))
    return file,member.replace("\\","/")

def load_env(source):
    outer,member=cut_bundle(source)
    if member is None:
        return UnityPy.load(str(outer)),sha_file(outer)
    with zipfile.ZipFile(outer) as z:
        content=z.read(member)
    sha=sha_bytes(content)
    env=UnityPy.load(content)
    return env,sha

def serialize_clip(obj,clip,category,source,source_hash,expected):
    muscles=getattr(clip,"m_MuscleClip",None)
    data=getattr(getattr(muscles,"m_Clip",None),"data",None)
    streamed=getattr(data,"m_StreamedClip",None)
    dense=getattr(data,"m_DenseClip",None)
    constant=getattr(data,"m_ConstantClip",None)
    bindings=getattr(getattr(clip,"m_ClipBindingConstant",None),"genericBindings",None)
    dense_count=int(getattr(dense,"m_CurveCount",0)) if dense is not None else 0
    frame_count=int(getattr(dense,"m_FrameCount",0)) if dense is not None else 0
    floats=getattr(dense,"m_SampleArray",[]) if dense is not None else []
    if frame_count>0 and dense_count>0 and len(floats)!=frame_count*dense_count:
        raise ValueError("Unity dense channel length mismatch")
    duration=float(getattr(muscles,"m_StopTime",0)-
                   getattr(muscles,"m_StartTime",0)) if muscles is not None else float(expected.get("seconds") or 0)
    return {
        "schema":"noworld.gate09.unity_native_clip_curves.v2",
        "kind":"UNRETARGETED_SOURCE_ANIMATION_CLIP",
        "source_bundle":source,
        "source_bundle_data_sha256":source_hash,
        "original_unity_path_id":int(obj.path_id),
        "name":clip.m_Name,
        "inventory_source_family":category,
        "source_sample_rate":scalar(getattr(clip,"m_SampleRate",None)),
        "duration_seconds":duration,
        "unity_clip_attributes":{
            "m_WrapMode":scalar(getattr(clip,"m_WrapMode",None)),
            "m_Legacy":scalar(getattr(clip,"m_Legacy",None)),
            "m_Compressed":scalar(getattr(clip,"m_Compressed",None)),
            "m_AnimationClipSettings":scalar(getattr(clip,"m_AnimationClipSettings",None))
        },
        "generic_bindings":[binding_dict(b) for b in bindings] if bindings is not None else [],
        "streamed":{
            "curve_count":int(getattr(streamed,"curveCount",0)) if streamed is not None else 0,
            "packed_u32":scalar(getattr(streamed,"data",[])) if streamed is not None else []
        },
        "dense":{
            "begin_time":scalar(getattr(dense,"m_BeginTime",None)) if dense is not None else None,
            "curve_count":dense_count,"frame_count":frame_count,
            "sample_rate":scalar(getattr(dense,"m_SampleRate",None)) if dense is not None else None,
            "float_samples":scalar(floats)
        },
        "constant_float_values":scalar(getattr(constant,"data",[])) if constant is not None else [],
        "legacy_curve_fields":{
            k:scalar(getattr(clip,k,None))
            for k in ("m_RotationCurves","m_CompressedRotationCurves","m_EulerCurves",
                      "m_PositionCurves","m_ScaleCurves","m_FloatCurves",
                      "m_PPtrCurves")
        },
        "renderable_rig_resolved":False,
        "retargeted_for_luminous_base":False,
        "retargeted_for_luminous_alter":False,
        "native_unreal_animsequence_generated":False,
        "source_geometry_modified":False,
        "adult_or_paired_context_authorized":False
    }

def atomic_gzip(path,payload):
    if path.exists():
        with gzip.open(path,"rb") as f:
            if f.read()==payload:return "PRESERVED_IDENTICAL"
        raise RuntimeError("Existing original captured clip differs; refusing overwrite: "+str(path))
    path.parent.mkdir(parents=True,exist_ok=True)
    temp=path.with_name(path.name+".partial")
    if temp.exists():raise RuntimeError("Incomplete source output found; manual inspect: "+str(temp))
    with open(temp,"wb") as f:
        with gzip.GzipFile(filename="",mode="wb",fileobj=f,compresslevel=3,mtime=0) as z:z.write(payload)
    with gzip.open(temp,"rb") as f:
        if f.read()!=payload:raise RuntimeError("Source clip verification failed")
    temp.rename(path)
    return "IMPORTED_SOURCE_CURVES"

def run(limit=0):
    inventory=json.loads(INVENTORY.read_text(encoding="utf8"))
    bundles=inventory["bundles"]
    assert inventory["observed_clip_objects"]==2741 and len(bundles)==47
    out=ROOT/"SourceAssets"/"Gate12"/"Animations20261021"/"ImportedGate09OriginalUnityCurves"
    out.mkdir(parents=True,exist_ok=True)
    index_path=out/"GATE09_ALL_SOURCE_INTAKE_PROGRESS.json"
    rows=[]
    status=collections.Counter()
    total_objects=0
    for bidx,b in enumerate(bundles):
        if limit and bidx>=limit:break
        source=b["path"]
        source_index={k:collections.deque() for k in set(x["name"] for x in b["clips"])}
        for c in b["clips"]:source_index[c["name"]].append(c)
        print("SOURCE_BUNDLE",bidx+1,len(bundles),source,flush=True)
        try:
            env,hash_source=load_env(source)
            found=collections.Counter()
            for obj in env.objects:
                if obj.type.name!="AnimationClip":continue
                clip=obj.read()
                name=clip.m_Name
                if name not in source_index or not source_index[name]:continue
                c=source_index[name].popleft()
                identity=json.dumps([source,name,int(obj.path_id)],separators=(",",":"),ensure_ascii=False)
                key=sha_bytes(identity.encode("utf8"))[:24]
                path=out/f"bundle_{bidx:03d}"/(key+".unitycurves.json.gz")
                native=serialize_clip(obj,clip,c.get("category","other"),source,hash_source,c)
                blob=json.dumps(native,ensure_ascii=False,separators=(",",":"),allow_nan=False).encode("utf8")
                imported=atomic_gzip(path,blob)
                rows.append({"source_bundle_index":bidx,"original_clip_path_id":int(obj.path_id),
                     "source_clip_name":name,"source_family":c.get("category","other"),
                     "source_key":key,"raw_curve_path":str(path),"raw_sha256":sha_file(path),
                     "state":imported,"source_hash":hash_source})
                found[name]+=1
                status[imported]+=1
                total_objects+=1
            missing=sum(len(q) for q in source_index.values())
            if missing:
                status["INVENTORY_NAME_NOT_FOUND_IN_SOURCE"]+=missing
                print("MISSING_OR_RENAMED_SOURCE",bidx,missing,flush=True)
        except Exception as error:
            status["BUNDLE_READ_ERROR"]+=1
            print("SOURCE_ERROR",bidx,type(error).__name__,str(error)[:300],traceback.format_exc()[-2400:],flush=True)
        finally:
            if "env" in locals():del env
            gc.collect()
        receipt={"schema":"noworld.gate12.gate09_source_curve_intake.v2",
            "source_inventory":str(INVENTORY),"source_clip_observations":2741,
            "bundles_expected":47,"bundles_attempted":bidx+1,
            "actual_curve_objects_extracted":total_objects,
            "source_counts":dict(status),"source_files":rows,
            "status":"ALL_SOURCE_BUNDLES_PROCESSED" if bidx+1==47 else "PARTIAL_SOURCE_CAPTURE",
            "runtime_acceptance":"NOT_RETARGETED_NOT_GAMEPLAY_APPROVED",
            "no_original_source_mutation":True}
        receipt_tmp=index_path.with_suffix(".tmp")
        receipt_tmp.write_text(json.dumps(receipt,ensure_ascii=False,indent=1)+"\n",encoding="utf8")
        os.replace(receipt_tmp,index_path)
        print("BUNDLE_PROGRESS",bidx+1,"CURVES",total_objects,"STATUS",dict(status),flush=True)
    return total_objects
if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--limit-bundles",type=int,default=0)
    a=parser.parse_args()
    print("GATE09_RAW_CLIP_SOURCE_COUNT",run(a.limit_bundles),flush=True)
