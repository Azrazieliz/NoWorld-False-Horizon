"""One-pass source-authentic local curve intake from installed Koikatsu
H animation bundles; retains all joint hashes, frames and channel data,
does not create explicit rendered scenes, characters or fabricated animation.
Does not distribute original game bundles. Resume-safe checkpoints.
"""
import pathlib,sys,json,hashlib,collections,gc,gzip,argparse,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
ROOT=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party\abdata\h\anim")
DST=R/"SourceAssets/Gate12/Animations20261021/KoikatsuHOriginalSourceCurves"
sys.path.insert(0,str(R/"Scripts/Gate12Koikatsu"))
from gate12_import_all_gate09_koikatsu_unity_source_curves_20261021 import (
    load_env,sha_bytes,sha_file,serialize_clip,atomic_gzip)
def run(limit=0):
    sources=sorted(ROOT.rglob("*.unity3d"),key=lambda p:str(p).lower())
    if not sources:raise RuntimeError("Installed Koikatsu H animation sources are absent")
    DST.mkdir(parents=True,exist_ok=True)
    output=DST/"GATE12_INSTALLED_H_ANIMATION_SOURCE_CURVE_INDEX.json"
    rows=[]
    stats=collections.Counter()
    for i,source in enumerate(sources):
        if limit and i>=limit:break
        source_path=str(source)
        print("SOURCE",i+1,"/",len(sources),source.name,flush=True)
        try:
            env,digest=load_env(source_path)
            clip_num=0
            for obj in env.objects:
                if obj.type.name!="AnimationClip":continue
                clip=obj.read()
                name=str(clip.m_Name)
                identity=json.dumps([source_path,name,int(obj.path_id)],
                                    ensure_ascii=False,separators=(",",":"))
                key=sha_bytes(identity.encode("utf8"))[:24]
                path=DST/f"bundle_{i:03d}"/(key+".unitycurves.json.gz")
                native=serialize_clip(obj,clip,"h_native_source",source_path,digest,{})
                native["context"]="UNCLASSIFIED_ORIGINAL_SOURCE_MOTION"
                native["state"]="CURVES_STORED_NOT_YET_RETARGETED_TO_A_PERSON"
                blob=json.dumps(native,ensure_ascii=False,separators=(",",":"),
                                allow_nan=False).encode("utf8")
                saved=atomic_gzip(path,blob)
                clip_num+=1
                stats[saved]+=1
                rows.append({"bundle_index":i,"clip_name":name,
                  "native_path_id":int(obj.path_id),
                  "source_bundle":source_path,
                  "source_digest_sha256":digest,
                  "curve_file":str(path),
                  "result":saved})
            print("CAPTURED",i+1,clip_num,flush=True)
        except Exception as error:
            stats["BUNDLE_FAILED"]+=1
            rows.append({"bundle_index":i,"source_bundle":source_path,
                "error":str(error)[:600]})
            print("FAILED",i+1,traceback.format_exc()[-800:],flush=True)
        finally:
            if "env" in locals():del env
            gc.collect()
        index={"schema":"noworld.gate12.koikatsu_native_h_original_source_curves.v1",
            "source_directory":str(ROOT),"discovered_bundles":len(sources),
            "bundles_processed":i+1,"native_clip_objects":sum(
              1 for row in rows if "curve_file" in row),
            "counts":dict(stats),
            "only_original_clips":True,"rendered_scenes_created":False,
            "unreal_native_sequences_created":False,
            "source_bundles_mutated":False,
            "status":"SOURCE_INTAKE_COMPLETE" if i+1==len(sources) and
                stats["BUNDLE_FAILED"]==0 else "SOURCE_INTAKE_PARTIAL",
            "clips":rows}
        temp=output.with_suffix(".tmp")
        temp.write_text(json.dumps(index,ensure_ascii=False,indent=1)+"\n",encoding="utf8")
        temp.replace(output)
        print("PROGRESS",i+1,len(sources),index["native_clip_objects"],
              dict(stats),flush=True)
if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--max-bundles",type=int,default=0)
    run(parser.parse_args().max_bundles)
