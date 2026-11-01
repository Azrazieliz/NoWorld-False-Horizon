#!/usr/bin/env python3
"""One-call native content submission API. Original creators supply their actual
card/artwork/FBX and canonical character or semantic IDs; no double-entry of
source hashes and no per-Character animation-library authoring.
Generated job is immutable, hashed, and picked up by the 5-star Windows worker.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re

REPO=Path(__file__).resolve().parents[2]
def sha(path):
    h=hashlib.sha256()
    with Path(path).open("rb") as f:
        for block in iter(lambda:f.read(4*1024*1024),b""):h.update(block)
    return h.hexdigest()
def source(path):
    file=Path(path).resolve(strict=True)
    if not file.is_file():raise ValueError("Actual original source required")
    return {"path":str(file),"sha256":sha(file)}
def create(job,folder):
    raw=json.dumps(job,sort_keys=True,ensure_ascii=False).encode()
    job_id="nw_"+hashlib.sha256(raw).hexdigest()[:22]
    job["job_id"]=job_id
    p=Path(folder).resolve()/ (job_id+".json")
    p.parent.mkdir(parents=True,exist_ok=True)
    content=json.dumps(job,indent=2,ensure_ascii=False)+"\n"
    if p.exists() and p.read_text("utf-8")!=content:
        raise FileExistsError("Immutable content request hash collision")
    if not p.exists():p.write_text(content,encoding="utf-8")
    print(json.dumps({"job":str(p),"job_id":job_id,
       "status":"SOURCE_JOB_READY_FOR_NATIVE_WINDOWS_FACTORY",
       "runtime_verified":False},indent=2))
    return p
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inbox",default="ContentFactory/Jobs")
    parser.add_argument("--publish-to-github",action="store_true",
        help="Explicitly permit generated UE assets (not raw cards) to be versioned in Git LFS")
    sub=parser.add_subparsers(dest="kind",required=True)
    char=sub.add_parser("character")
    char.add_argument("--card",required=True)
    char.add_argument("--identity",required=True)
    char.add_argument("--version",required=True)
    char.add_argument("--rig",required=True)
    char.add_argument("--anatomy",required=True)
    char.add_argument("--wardrobe",required=True)
    char.add_argument("--wardrobe-field",required=True)
    char.add_argument("--shared-library",required=True)
    char.add_argument("--capabilities",nargs="*",default=[])
    animation=sub.add_parser("motion")
    animation.add_argument("--fbx",required=True)
    animation.add_argument("--skeleton",required=True)
    animation.add_argument("--motion-id",required=True)
    animation.add_argument("--source-clip-id",required=True)
    animation.add_argument("--source-original-hash",required=True)
    animation.add_argument("--action",required=True)
    animation.add_argument("--context",required=True)
    animation.add_argument("--rig",required=True)
    animation.add_argument("--anatomy",required=True)
    animation.add_argument("--outfit",required=True)
    animation.add_argument("--approval-evidence",required=True)
    animation.add_argument("--contact-seconds",type=float)
    animation.add_argument("--contact-notify")
    animation.add_argument("--natural-completion-contact",action="store_true")
    art=sub.add_parser("artwork")
    art.add_argument("--image",required=True)
    art.add_argument("--identity")
    art.add_argument("--version")
    art.add_argument("--approval-evidence",required=True)
    args=parser.parse_args()
    if args.kind=="character":
        if not args.card.lower().endswith(".png"):
            raise ValueError("Original Koikatsu cards must be native .png, not a rasterized imitation")
        inputs=source(args.card)
        job={"schema":"noworld.factory.job.v1","kind":"character",
             "identity":args.identity,"version":args.version,
             "sources":[dict(role="koikatsu_card",**inputs)],
             "native":{"rig":args.rig,"anatomy":args.anatomy,
                       "wardrobe":args.wardrobe,
                       "wardrobe_state_field":args.wardrobe_field,
                       "shared_motion_library":args.shared_library,
                       "capabilities":args.capabilities},
             "publish_to_github":args.publish_to_github}
    elif args.kind=="motion":
        src=source(args.fbx)
        if len(args.source_original_hash)!=64 or not re.fullmatch("[0-9a-fA-F]{64}",args.source_original_hash):
            raise ValueError("Exact original motion-source SHA256 required")
        if not any((args.contact_seconds is not None,
                    bool(args.contact_notify),args.natural_completion_contact)):
            raise ValueError("Real authored contact notify/time or natural full sequence completion required")
        m=dict(src,skeleton=args.skeleton,motion_id=args.motion_id,
               source_clip_id=args.source_clip_id,
               source_hash=args.source_original_hash.lower(),
               action=args.action,category=args.context,context=args.context,
               rig=args.rig,anatomy=args.anatomy,outfit=args.outfit,
               approval_evidence=args.approval_evidence,
               approved_for_gameplay=True,loop=False,participants=1)
        if args.contact_seconds is not None:m["contact_seconds"]=args.contact_seconds
        if args.contact_notify:m["contact_notify"]=args.contact_notify
        if args.natural_completion_contact:m["natural_completion_contact"]=True
        job={"schema":"noworld.factory.job.v1","kind":"motion","animations":[m],
             "publish_to_github":args.publish_to_github}
    else:
        job={"schema":"noworld.factory.job.v1","kind":"artwork",
             "artwork":[dict(source(args.image),approval_evidence=args.approval_evidence,
                             purpose="original")],
             "publish_to_github":args.publish_to_github}
        if args.identity and args.version:job.update(identity=args.identity,version=args.version)
    create(job,args.inbox)
if __name__=="__main__":main()
