"""Bounded real Koikatsu source-to-native preparation. NOT a source-only completion claim.
Select exact unique installed Unity AnimationClip objects and preserve source
hashes and per-bone quaternion curves without touching original source bundles.
All decoded data are unapproved until a real Unreal AnimSequence is authored
and a visual/scene acceptance path is satisfied.
"""
import argparse,collections,json,pathlib,re,sys,traceback
ROOT=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
SOURCE=ROOT/"SourceAssets/Gate12/Animations20261021/ImportedGate09OriginalUnityCurves/GATE09_ALL_SOURCE_INTAKE_PROGRESS.json"
DEST=ROOT/"SourceAssets/Gate12/Animations20261023/KoikatsuCalibratedNativeTracks"
PLAN=ROOT/"SourceAssets/Gate12/Animations20261023/KoikatsuBulkNativeConversionPlan.json"
sys.path.insert(0,str(ROOT/"Scripts/production"))
import gate12_decode_genuine_koikatsu_studio_to_native_joint_curves_20261022 as core

def classify(name):
    n=name.lower()
    if n.startswith(("f_defense_", "f_reaction_")):return ("Combat.Reaction." if "reaction" in n else "Combat.Defense.")+name,"Combat","combat"
    if n.startswith(("f_aruki_", "f_hasiru_")):return ("Traversal.Walk." if "aruki" in n else "Traversal.Run.")+name,"Traversal","grounded"
    if n.startswith("f_suiei_"):return "Traversal.Swim."+name,"Traversal","swimming"
    if n.startswith("f_land_"):return "Traversal.Land."+name,"Traversal","grounded"
    if n.startswith("f_cheer_"):return "Emote.Cheer."+name,"Interaction","emote"
    if n.startswith(("stand_ang_", "stand_sad_", "stand_joy_")):return "Emote.Stance."+name,"Interaction","emote"
    if n.startswith("idle_"):return "Emote.Idle."+name,"Interaction","emote"
    if n.startswith(("stand_", "f_stand_", "f_tachi_")):return "Interaction.Stance."+name,"Interaction","emote"
    if n.startswith("f_talk_"):return "Interaction.Talk."+name,"Interaction","emote"
    if n.startswith("f_eat_"):return "Interaction.Eat."+name,"Interaction","emote"
    if n.startswith("f_suwari_"):return "Interaction.Sit."+name,"Interaction","emote"
    if n.startswith("f_sleep_"):return "Interaction.Sleep."+name,"Interaction","emote"
    if n.startswith("f_sadou_"):return "Interaction.Tea."+name,"Interaction","emote"
    if n.startswith("f_manken_"):return "Interaction.MartialTraining."+name,"Interaction","emote"
    if n.startswith("f_action_"):return "Interaction.Action."+name,"Interaction","emote"
    if n.startswith(("f_syagami_", "f_undou_")):return "Interaction.Exercise."+name,"Interaction","emote"
    return None

def get_plan():
    ledger=json.loads(SOURCE.read_text(encoding="utf8"))
    first=[e for e in ledger["source_files"] if e["source_bundle_index"]<10]
    byname=collections.Counter(e["source_clip_name"] for e in first)
    rows=[]
    for e in first:
        name=e["source_clip_name"]
        semantic=classify(name)
        if byname[name]!=1 or not re.fullmatch("[A-Za-z0-9_]{3,65}",name) or not semantic:
            continue
        if any(x in name.lower() for x in ("dance", "koikatu", "soine", "pole")):
            continue
        action,context,cap=semantic
        rows.append({
            "source_clip":name,"action":action,"context":context,"capability":cap,
            "source_original_path_id":e["original_clip_path_id"],
            "source_unity_bundle_index":e["source_bundle_index"],
            "source_unity_bundle_hash":e["source_hash"],
            "source_curve_path":e["raw_curve_path"],
            "source_curve_sha256":e["raw_sha256"],
            "native_file":name+".genuine_unity_kk_stream_native_deltas.json.gz",
            "qa_only":True})
    order={"Combat":0,"Traversal":1,"Interaction":2}
    rows.sort(key=lambda e:(order[e["context"]],e["action"],e["source_clip"]))
    return rows

def main(args):
    rows=get_plan()
    if PLAN.exists():
        existing=json.loads(PLAN.read_text(encoding="utf8"))
        if [e["source_clip"] for e in existing["motions"]] != [e["source_clip"] for e in rows]:
            raise RuntimeError("Unapproved source plan changed; preserve previous selection")
    else:
        PLAN.parent.mkdir(parents=True,exist_ok=True)
        PLAN.write_text(json.dumps({
            "schema":"noworld.gate12.original_koikatsu_native_bulk.v1",
            "selection":"source-unique original clips, safe semantic prefixes, excludes rejected dance",
            "motions":rows,"gameplay_approval":False,
            "upstream_unity_archive_modified":False},indent=2)+"\n",encoding="utf8")
    end=min(args.end,len(rows))
    assert 0<=args.start<end<=len(rows)
    DEST.mkdir(parents=True,exist_ok=True)
    core.OUT=DEST
    core.INDEX={"source_files":[
       {"source_clip_name":e["source_clip"],"source_bundle_index":e["source_unity_bundle_index"],
        "raw_curve_path":e["source_curve_path"],"source_hash":e["source_unity_bundle_hash"]}
        for e in rows]}
    rest=core.source_bone_catalog()
    receipt=DEST/f"Gate12NativeSourceBatch_{args.start:04d}_{end:04d}.json"
    report={"schema":"noworld.gate12.native_source_conversion_batch.v1",
        "start":args.start,"end":end,"results":[],"status":"RUNNING"}
    def save():
        tmp=receipt.with_suffix(".json.tmp")
        tmp.write_text(json.dumps(report,indent=2)+"\n",encoding="utf8")
        tmp.replace(receipt)
    save()
    for index in range(args.start,end):
        item=rows[index];clip=item["source_clip"]
        output=DEST/item["native_file"]
        try:
            if output.exists():
                with __import__("gzip").open(output,"rt",encoding="utf8") as f:
                    stored=json.load(f)
                assert stored["clip"]==clip and stored["source_path_id"]==item["source_original_path_id"]
                result={"index":index,"clip":clip,"state":"PRESERVED_VERIFIED"}
            else:
                data=core.convert(clip,rest)
                assert data["source_path_id"]==item["source_original_path_id"]
                result={"index":index,"clip":clip,"state":"SOURCE_BONE_TRACKS_CALIBRATED",
                    "moving_bones":data["animated_rotation_bone_count"],"frames":data["frame_count"]}
        except Exception as ex:
            result={"index":index,"clip":clip,"state":"SOURCE_NOT_NATIVE_READY",
                    "error":type(ex).__name__+":"+str(ex)[:200]}
        report["results"].append(result);save()
        print("G12_DECODE",index+1,len(rows),clip,result["state"],flush=True)
    report["status"]="SOURCE_SHARD_DONE";save()
    print("G12_SOURCE_SHARD_SUMMARY",sum(e["state"]!="SOURCE_NOT_NATIVE_READY" for e in report["results"]),
        "OF",len(report["results"]),flush=True)

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--start",type=int,required=True)
    p.add_argument("--end",type=int,required=True)
    main(p.parse_args())
