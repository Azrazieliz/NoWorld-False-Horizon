#!/usr/bin/env python3
"""5★ operational contract: read 7★/6★/5★, record a REAL source change
against exact 36 systems without redoing existing code or claiming acceptance.

Usage from THIS active GitHub checkout:
  python Scripts/production/record_noworld_progress.py --resume
  python Scripts/production/record_noworld_progress.py \
    --commit <actual-git-commit> --system M03 --system M09 \
    --result "Exactly what changed in the game runtime" \
    --next "Concrete remaining playable requirements" \
    --reuse-5star Scripts/production/bind_native_motion.py

If no reusable 5★ program supports the need, author a NEW genuine reusable
program first and provide both --new-5star-program and --why-new.
This program MUST NOT fabricate approval, overwrite existing milestone states,
alter creative rules, or mark source-only work as a completed game system.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import os

ROOT=Path(__file__).resolve().parents[2]
AUTHORITY=ROOT/"docs/production/NOWORLD_7STAR_AUTHORITY.md"
LEDGER=ROOT/"docs/production/ADVANCEMENT_PROGRESS.json"
FACTORY=ROOT/"docs/production/CONTENT_BINDING_5STAR.md"
MILESTONES=ROOT/"docs/roadmap/milestones.json"
ALLOWED_NEW_ROOTS=("Scripts/production/","Tools/","Scripts/Gate12Koikatsu/")

def git(*args: str)->str:
    p=subprocess.run(["git","-C",str(ROOT),*args],text=True,
                     capture_output=True,check=False)
    if p.returncode:
        raise ValueError(f"Git operation failed: {args[0]}: {p.stderr.strip()[:360]}")
    return p.stdout.strip()

def checked_path(name: str)->Path:
    if not name or Path(name).is_absolute():raise ValueError("Use a repository-relative path")
    p=(ROOT/name).resolve()
    if not p.is_relative_to(ROOT) or not p.is_file():
        raise ValueError("Required 5★ source file is not present: "+name)
    return p

def read_ledger():
    for file in (AUTHORITY,LEDGER,FACTORY,MILESTONES):
        if not file.is_file():raise ValueError("Required canonical file missing: "+str(file))
    ledger=json.loads(LEDGER.read_text("utf-8-sig"))
    milestones=json.loads(MILESTONES.read_text("utf-8-sig"))["milestones"]
    ids={m["id"] for m in milestones}
    if len(ids)!=36 or len(ledger.get("tasks",[]))!=111:
        raise ValueError("The original 36 systems/111 task ledger changed: do not rewrite blindly")
    return ledger,milestones,ids

def continuation(ledger,milestones):
    print(json.dumps({"authority":"7★ v0.40 full baseline + selective v0.41 mature addendum + later approved decisions",
       "current_ledger":"docs/production/ADVANCEMENT_PROGRESS.json",
       "last_recorded":ledger.get("continuation_records",[])[-3:],
       "historical":ledger.get("summary"),
       "unaccepted":[{"id":m["id"],"title":m["title"]} for m in milestones
                     if next((x for x in ledger["integrated_system_advancement"]["milestones"]
                              if x["id"]==m["id"]),{}).get("acceptance_state")!="OWNER_ACCEPTED"],
       "5star":"Use existing indexed production program before writing anything new",
       "source_only_is_not_player_accepted":True},indent=2,ensure_ascii=False))

def record(a):
    ledger,milestones,ids=read_ledger()
    if a.resume:
        continuation(ledger,milestones)
        return
    if not (a.commit and a.system and a.result and a.next):
        raise ValueError("Require real commit, system IDs, actual result, and remaining player-visible work")
    if any(m not in ids for m in a.system):
        raise ValueError("Unknown 7★-indexed 36-system ID")
    if len(a.result.strip())<20 or len(a.next.strip())<20:
        raise ValueError("State concrete implementation and remaining work, not labels")
    reused=bool(a.reuse_5star)
    created=bool(a.new_5star_program)
    if reused==created:
        raise ValueError("Choose exactly one: existing 5★ program or NEW reusable 5★ program with written reason")
    if reused:
        checked_path(a.reuse_5star)
        if a.why_new:raise ValueError("Do not claim a new program when one was reused")
    else:
        if not a.why_new or len(a.why_new.strip())<30:
            raise ValueError("Explain precisely why the 5★ inventory cannot do this work")
        if not a.new_5star_program.startswith(ALLOWED_NEW_ROOTS):
            raise ValueError("New reusable 5★ source program must live under a known program root")
        checked_path(a.new_5star_program)
    sha=git("rev-parse","--verify",a.commit+"^{commit}")
    if git("merge-base","--is-ancestor",sha,"HEAD") not in ("",):
        raise ValueError("Unexpected ancestor response")
    paths=git("diff-tree","--no-commit-id","--name-only","-r",sha).splitlines()
    if not paths:
        raise ValueError("The claimed commit has no changed source files")
    if not any(p.startswith(("Source/","Content/","Config/","Scripts/","Tools/")) for p in paths):
        raise ValueError("The commit has no gameplay/content/tool source change; update 6★ only for material source")
    original=ledger.get("continuation_records",[])
    if any(v["source_commit"]==sha for v in original):
        raise ValueError("Commit already recorded in 6★: do not repeat progress")
    item={"source_commit":sha,"system_ids":sorted(set(a.system)),
          "result":a.result.strip(),"remaining_player_visible_work":a.next.strip(),
          "source_paths":paths,
          "5star_reused_program":a.reuse_5star if reused else None,
          "5star_new_program":a.new_5star_program if created else None,
          "5star_new_program_reason":a.why_new.strip() if created else None,
          "status":"SOURCE_IMPLEMENTED_NOT_OWNER_ACCEPTED",
          "counted_as_completed_system":False,
          "saved_utc":datetime.now(timezone.utc).isoformat()}
    original.append(item)
    ledger["continuation_records"]=original
    ledger["continuation_last_source_commit"]=sha
    ledger["continuation_required_read_order"]=["docs/production/NOWORLD_7STAR_AUTHORITY.md",
        "docs/production/ADVANCEMENT_PROGRESS.json","docs/production/CONTENT_BINDING_5STAR.md"]
    # Do NOT advance original milestone/task checked or user acceptance flags.
    before=ledger["summary"].copy()
    if ledger["summary"]!=before:raise ValueError("Historical completion cannot change in recorder")
    fd,temp=tempfile.mkstemp(prefix=".noworld_6star_",suffix=".json",dir=LEDGER.parent)
    try:
        with os.fdopen(fd,"w",encoding="utf8") as f:
            json.dump(ledger,f,indent=2,ensure_ascii=False)
            f.write("\n")
            f.flush()
            os.fsync(f.fileno())
        os.replace(temp,LEDGER)
    finally:
        if os.path.isfile(temp):os.unlink(temp)
    print(json.dumps({"status":"6STAR_LEDGER_UPDATED_NOT_GAME_COMPLETION",
                      "commit":sha,"systems":item["system_ids"],"changed_files":len(paths)},
                     indent=2))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--resume",action="store_true",
                   help="Show all 36 real-game systems and latest saved work without modifying files")
    p.add_argument("--commit",help="Existing committed SHA from current checkout")
    p.add_argument("--system",action="append",help="One of M01-M36, repeatable")
    p.add_argument("--result",help="Exact implemented integrated source delta")
    p.add_argument("--next",help="Specific remaining gameplay work")
    p.add_argument("--reuse-5star",dest="reuse_5star",
                   help="Path to the actual pre-existing reused 5★ program")
    p.add_argument("--new-5star-program",dest="new_5star_program",
                   help="Path to a new reusable automation program that was really authored")
    p.add_argument("--why-new",help="Concrete gap in existing 5★ programs")
    args=p.parse_args()
    try:record(args)
    except (OSError,ValueError,KeyError,json.JSONDecodeError) as e:
        p.exit(2,"6★ checkpoint blocked: "+str(e)+"\n")

if __name__=="__main__":main()
