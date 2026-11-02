#!/usr/bin/env python3
"""5-star source-authentic original Koikatsu combat AnimationEvent extractor.

It inspects the *actual SHA256-verified original Unity bundle* and exact clip
path IDs referenced by the EXISTING 3222-motion global catalog. Only an
original Unity AnimationEvent that explicitly identifies an attack contact
may write a contact_seconds value; neither duration fractions, velocity
maxima nor animation endings are automatically fabricated as impact times.

Usage:
  python Scripts/production/noworld_original_combat_contact_author.py \
    --catalog Content/NoWorld/Animation/SharedMotionCatalog.json \
    --output Saved/NoWorldContentFactory/combat_contacts.json [--apply]
Requires UnityPy for genuine .unity3d source examination. Read-only by default.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import tempfile

EVENT= re.compile(r"^(?:On)?(?:AttackHit|HitContact|WeaponImpact|StrikeContact|DamageContact|CombatImpact)$",re.I)
NON_IMPACT=("Combat.HitReaction","Combat.Reaction.","Combat.Defense.")
def sha(path):
    h=hashlib.sha256()
    with path.open("rb") as f:
        for b in iter(lambda:f.read(1<<20),b""):h.update(b)
    return h.hexdigest()
def get_event(e,key):
    if isinstance(e,dict):
        return next((e.get(k) for k in key if k in e),None)
    return next((getattr(e,k) for k in key if hasattr(e,k)),None)
def inspect_bundle(file,wanted):
    import UnityPy
    env=UnityPy.load(str(file))
    out={}
    for obj in env.objects:
        if obj.path_id not in wanted:continue
        if str(obj.type.name)!="AnimationClip":continue
        clip=obj.read()
        events=getattr(clip,"m_Events",None)
        if events is None:
            try:events=obj.read_typetree().get("m_Events",[])
            except Exception:events=[]
        valid=[]
        discarded=[]
        for e in events or []:
            name=get_event(e,("functionName","function_name","m_FunctionName"))
            timestamp=get_event(e,("time","m_Time"))
            if not isinstance(name,str) or not isinstance(timestamp,(int,float)):
                discarded.append({"name":str(name),"time":str(timestamp)})
                continue
            if EVENT.fullmatch(name):
                valid.append({"function":name,"time":float(timestamp)})
            else:discarded.append({"name":name,"time":float(timestamp)})
        out[obj.path_id]={"valid_events":valid,"other_events":discarded,
                           "source_path_id":obj.path_id}
    return out

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--catalog",type=Path,required=True)
    p.add_argument("--output",type=Path,required=True)
    p.add_argument("--apply",action="store_true")
    args=p.parse_args()
    catalog=json.loads(args.catalog.read_text("utf-8-sig"))
    if catalog.get("schema")!="noworld.shared_motion_catalog.v1":
        raise ValueError("Current authentic global motion catalog schema required")
    motions=catalog.get("motions")
    if not isinstance(motions,list) or len(motions)!=3222:
        raise ValueError("Original 3222-entry global catalog must be preserved")
    combat=[m for m in motions if m.get("context")=="Combat"]
    if len(combat)!=937:
        raise ValueError("Original 937 combat entries changed; reconcile before authoring")
    groups={}
    for m in combat:
        key=(m.get("source_bundle"),m.get("source_bundle_sha256"))
        if not key[0] or not key[1]:raise ValueError("Unproven combat original bundle")
        groups.setdefault(key,[]).append(m)
    report={"schema":"noworld.original_combat_contact_evidence.v1",
            "source_clips":937,"native_library_total":3222,"by_source_bundle":[],
            "source_authored_contacts":[],"non_impact_motion_count":0,
            "unresolved_motion_count":0,"ambiguous_events":[],
            "not_a_gameplay_acceptance":True,"updated":False}
    for (filename,digest),rows in sorted(groups.items()):
        original=Path(filename)
        if not original.is_file():
            raise FileNotFoundError("Cannot inspect actual protected Unity source: "+str(original))
        observed=sha(original)
        if observed.lower()!=digest.lower():
            raise ValueError("Source bundle SHA256 mismatch: "+str(original))
        wanted={int(m["source_unity_path_id"]) for m in rows}
        discovered=inspect_bundle(original,wanted)
        report["by_source_bundle"].append({
            "original":str(original),"sha256":observed,
            "indexed_motion_count":len(rows),
            "distinct_path_ids":len(wanted),
            "read_source_clips":len(discovered)})
        for m in rows:
            action=m["action"]
            if action.startswith(NON_IMPACT):
                report["non_impact_motion_count"]+=1
                continue
            info=discovered.get(int(m["source_unity_path_id"]))
            valid=info["valid_events"] if info else []
            duration=m.get("native_duration_seconds")
            eligible=[e for e in valid if isinstance(duration,(float,int)) and
                      0<=e["time"]<=duration]
            if len(eligible)==1 and not m.get("loop"):
                when=eligible[0]["time"]
                if m.get("contact_seconds") is not None and m["contact_seconds"]!=when:
                    raise ValueError("Existing authored contact conflict for "+m["motion_id"])
                report["source_authored_contacts"].append({
                    "motion_id":m["motion_id"],"source_unity_path_id":m["source_unity_path_id"],
                    "source_bundle_sha256":digest,"contact_seconds":when,
                    "animation_event":eligible[0]["function"],
                    "native_sequence":m["native_sequence"]})
                if args.apply:m["contact_seconds"]=when
            else:
                report["unresolved_motion_count"]+=1
                if len(eligible)>1:
                    report["ambiguous_events"].append({
                        "motion_id":m["motion_id"],"events":eligible})
    if args.apply and report["source_authored_contacts"]:
        fd,tmp=tempfile.mkstemp(prefix=".noworld_combat_events_",suffix=".json",
                                 dir=args.catalog.parent)
        try:
            with os.fdopen(fd,"w",encoding="utf-8") as f:
                json.dump(catalog,f,indent=2,ensure_ascii=False)
                f.write("\n")
                f.flush();os.fsync(f.fileno())
            os.replace(tmp,args.catalog)
        finally:
            if os.path.exists(tmp):os.unlink(tmp)
        report["updated"]=True
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+"\n","utf-8")
    print(json.dumps({k:v for k,v in report.items()
                      if k in ("source_clips","native_library_total",
                               "non_impact_motion_count","unresolved_motion_count",
                               "updated")} |
           {"real_authored_contact_motions":len(report["source_authored_contacts"])},
           indent=2))
if __name__=="__main__":main()
