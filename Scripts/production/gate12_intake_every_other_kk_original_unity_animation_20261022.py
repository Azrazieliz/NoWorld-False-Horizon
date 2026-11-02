"""Continue Koikatsu *original Unity AnimationClip* source-intake beyond the
288 installed H animation bundles already preserved in their frozen index.
Does not invent native Unreal AnimSequence or silently accept rig retarget.
Read-only Unity bundles, exact original clip IDs, lossless native source curves.
Resume-safe and collision-safe. Each batch is independently committed via receipts.
"""
import argparse,collections,gc,gzip,hashlib,json,pathlib,sys,traceback,time,os
ROOT=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
ABDATA=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party\abdata")
DST=ROOT/"SourceAssets/Gate12/Animations20261022/KoikatsuAdditionalOriginalUnityCurves"
sys.path.insert(0,str(ROOT/"Scripts/Gate12Koikatsu"))
from gate12_import_all_gate09_koikatsu_unity_source_curves_20261021 import load_env,serialize_clip,atomic_gzip,sha_bytes
def main(start,end):
    all_bundles=sorted(ABDATA.rglob("*.unity3d"),key=lambda x:str(x).lower())
    # Existing source curve archive is authoritative and will never be redone.
    bundles=[p for p in all_bundles if not
             (len(p.relative_to(ABDATA).parts)>=3 and p.relative_to(ABDATA).parts[:2]==("h","anim"))]
    assert len(all_bundles)>=1600 and len(bundles)>1000
    assert 0<=start<end<=len(bundles)
    DST.mkdir(parents=True,exist_ok=True)
    output=DST/f"GATE12_ALL_INSTALLED_KK_ADDITIONAL_ANIMATIONS_PART_{start:04d}_{end:04d}.json"
    if output.exists():raise RuntimeError("Preserve already sealed batch, never overwrite")
    rows=[];stats=collections.Counter()
    for i in range(start,end):
        src=bundles[i];rel=src.relative_to(ABDATA).as_posix()
        added=0
        try:
            env,digest=load_env(str(src))
            for obj in env.objects:
                if obj.type.name!="AnimationClip":continue
                try:
                    clip=obj.read();name=str(clip.m_Name)
                    ident=json.dumps([str(src),name,int(obj.path_id)],ensure_ascii=False,separators=(",",":"))
                    key=sha_bytes(ident.encode("utf8"))[:24]
                    dest=DST/f"bundle_{i:04d}"/(key+".unitycurves.json.gz")
                    native=serialize_clip(obj,clip,"kk_installed_native_additional",str(src),digest,{})
                    native["context"]="ORIGINAL_KOIKATSU_UNCLASSIFIED_NONGAMEPLAY_SOURCE"
                    native["state"]="ORIGINAL_CURVES_CAPTURED_NOT_RIG_RETARGETED"
                    blob=json.dumps(native,ensure_ascii=False,separators=(",",":"),allow_nan=False).encode("utf8")
                    status=atomic_gzip(dest,blob)
                    rows.append({"bundle_index":i,"source_bundle":rel,"source_bundle_sha256":digest,
                                 "name":name,"path_id":int(obj.path_id),
                                 "native_curve_path":str(dest),"result":status})
                    stats[status]+=1;added+=1
                except Exception as err:
                    rows.append({"bundle_index":i,"source_bundle":rel,
                                 "path_id":int(obj.path_id),"error":type(err).__name__+": "+str(err)[:250]})
                    stats["CLIP_FAILED"]+=1
            stats["BUNDLE_READ_OK"]+=1
        except Exception as err:
            stats["BUNDLE_FAILED"]+=1
            rows.append({"bundle_index":i,"source_bundle":rel,
                         "error":type(err).__name__+": "+str(err)[:500]})
        finally:
            if "env" in locals():del env
            gc.collect()
        print("KK_SOURCE",i+1,"/",len(bundles),rel,"ANIMATIONS",added,"FAILURES",stats["BUNDLE_FAILED"],flush=True)
        if (i-start+1)%5==0 or i==end-1:
            report={"schema":"noworld.gate12.all_installed_kk_original_additional_clips.v1",
                    "source_directory":str(ABDATA),"installed_unity_bundles":len(all_bundles),
                    "already_ingested_h_animation_bundles":len(all_bundles)-len(bundles),
                    "additional_bundles":len(bundles),"batch_start":start,"batch_end":end,
                    "progress_next_index":i+1,"counts":dict(stats),"clip_count":sum("native_curve_path" in q for q in rows),
                    "status":"SOURCE_ARCHIVE_COMPLETE_NOT_NATIVE_UNREAL" if i==end-1 and not(stats["CLIP_FAILED"]+stats["BUNDLE_FAILED"]) else "SOURCE_ARCHIVE_PARTIAL_OR_FAILED",
                    "playable_ue_animsequence_created":0,"retarget_authorized":False,
                    "original_bundles_unchanged":True,"clips":rows}
            temp=output.with_suffix(".json.tmp")
            temp.write_text(json.dumps(report,ensure_ascii=False,indent=1)+"\n",encoding="utf8")
            os.replace(temp,output)
    print("ADDITIONAL_ANIMATION_BATCH",start,end,"CURVES",report["clip_count"],"STATE",report["status"],flush=True)
if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--start",type=int,required=True)
    parser.add_argument("--end",type=int,required=True)
    a=parser.parse_args()
    main(a.start,a.end)
