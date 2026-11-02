#!/usr/bin/env python3
"""NoWorld 5★: never lose original indexed motion assets when merging GitHub.

Read-only by default:
  python Scripts/production/reconcile_native_motion_catalog.py \
    --native-project D:/UnrealProjects/Gate12_WholeCharacter_Build_20261031 \
    --ref origin/work/gate12-whole-character-shared-motion-20261031

For a genuine lost catalog (not needed after ce190e87):
  --restore --push
The program creates a child commit on the fetched remote ref through a PRIVATE
temporary Git index; it does not checkout/reset, alter, replace, or delete the
protected Windows Unreal working tree. It NEVER auto-approves combat contact.
"""
from __future__ import annotations
import argparse
from collections import Counter
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

REL="Content/NoWorld/Animation/SharedMotionCatalog.json"
SCHEMA="noworld.shared_motion_catalog.v1"

def command(repo:Path,*args:str,stdin:bytes|None=None)->bytes:
    p=subprocess.run(["git","-C",str(repo),*args],input=stdin,
                     capture_output=True)
    if p.returncode!=0:
        raise RuntimeError("Git read/write failed: "+" ".join(args[:3])+" "+p.stderr.decode("utf8","replace")[:400])
    return p.stdout.strip()

def read_catalog(raw:bytes,where:str)->dict:
    data=json.loads(raw.decode("utf8-sig"))
    if data.get("schema")!=SCHEMA or not isinstance(data.get("motions"),list):
        raise ValueError(where+": not the original approved schema")
    ids=[m["motion_id"].casefold() for m in data["motions"]]
    if len(ids)!=len(set(ids)):
        raise ValueError(where+": duplicate motion IDs; refuse silent loss")
    return data

def paths(data:dict)->set[str]:
    out=set()
    for m in data["motions"]:
        native=m.get("native_sequence","")
        if not native.startswith("/Game/") or native.count(".")!=1:
            raise ValueError("Original source has invalid native Unreal package name")
        package,asset=native[6:].split(".",1)
        if package.rsplit("/",1)[-1]!=asset or not package:
            raise ValueError("Unreal package name mismatches source asset")
        out.add("Content/"+package+".uasset")
    return out

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--native-project",type=Path,required=True)
    p.add_argument("--ref",default="origin/work/gate12-whole-character-shared-motion-20261031")
    p.add_argument("--restore",action="store_true",help="Create a new non-destructive child commit with missing catalog entries")
    p.add_argument("--push",action="store_true",help="Fast-forward push the generated commit to the active remote branch")
    a=p.parse_args()
    root=a.native_project.resolve(strict=True)
    if not (root/"OfflineGame.uproject").is_file() or not (root/".git").exists():
        raise ValueError("Require the original authorized and already-connected native Unreal Git project")
    native_file=root/REL
    if not native_file.is_file():raise FileNotFoundError(str(native_file))
    original_bytes=native_file.read_bytes()
    original=read_catalog(original_bytes,"protected native original")
    refsha=command(root,"rev-parse","--verify",a.ref+"^{commit}").decode()
    remote_bytes=command(root,"show",refsha+":"+REL)
    remote=read_catalog(remote_bytes,"fetched GitHub ref")
    tracked=set(x.decode("utf8","replace") for x in command(root,"ls-tree","-r","--name-only","-z",refsha,"--","Content").split(b"\0") if x)
    sourcepaths=paths(original)
    on_disk=[n for n in sourcepaths if not (root/n).is_file()]
    in_git=[n for n in sourcepaths if n not in tracked]
    if on_disk or in_git:
        raise ValueError(f"Real native source missing: PC={len(on_disk)}, remote Git objects={len(in_git)}; not a safe restore")
    original_by={m["motion_id"].casefold():m for m in original["motions"]}
    current_by={m["motion_id"].casefold():m for m in remote["motions"]}
    conflicts=[k for k in set(original_by)&set(current_by) if original_by[k]!=current_by[k]]
    missing=[v for k,v in original_by.items() if k not in current_by]
    status={"native_original_indexed":len(original_by),
            "github_indexed_before":len(current_by),
            "missing_git_catalog_entries":len(missing),
            "conflicts_require_manual_review":len(conflicts),
            "native_asset_paths_in_pc_and_git":len(sourcepaths),
            "by_category":dict(Counter(m.get("context","unknown") for m in original["motions"])),
            "approved_by_context":dict(Counter(m.get("context","unknown") for m in original["motions"] if m.get("approved_for_gameplay") is True)),
            "combat_authored_contacts":sum(m.get("context")=="Combat" and
                (isinstance(m.get("contact_seconds"),(int,float)) and m["contact_seconds"]>=0 or bool(m.get("contact_notify"))) for m in original["motions"]),
            "fetched_parent":refsha,"working_tree_untouched":True}
    print(json.dumps(status,indent=2))
    if conflicts:
        raise ValueError("Catalog diverged with same-ID different original motion source; never overwrite")
    if a.push and not a.restore:raise ValueError("To publish new source inventory use both --restore and --push")
    if not a.restore:
        return
    if not missing:
        print("NO_RESTORATION_NEEDED: the complete original source catalog is already present")
        return
    merged=dict(remote)
    merged["motions"]=remote["motions"]+missing
    if not remote["motions"]:
        # Lossless recovery: preserve the entire original 4.7MB source document,
        # its metadata and original ordering, not just an approximated rewrite.
        payload=original_bytes
    else:
        payload=(json.dumps(merged,indent=2,ensure_ascii=False)+"\n").encode("utf8")
    blob=command(root,"hash-object","-w","--stdin",stdin=payload).decode()
    fd,index=tempfile.mkstemp(prefix="noworld_5star_gitindex_")
    os.close(fd)
    os.unlink(index)
    old=os.environ.get("GIT_INDEX_FILE")
    try:
        os.environ["GIT_INDEX_FILE"]=index
        command(root,"read-tree",refsha)
        command(root,"update-index","--add","--cacheinfo","100644,"+blob+","+REL)
        tree=command(root,"write-tree").decode()
        name="NoWorld 5-star catalog recovery"
        email="noworld-source-recovery@users.noreply.github.com"
        commit=command(root,"-c","user.name="+name,"-c","user.email="+email,
                       "commit-tree",tree,"-p",refsha,
                       "-m","Restore original indexed native motion catalog without losing source entries").decode()
        print("SAFE_RECOVERY_COMMIT",commit)
        if a.push:
            if not a.ref.startswith("origin/"):raise ValueError("Only an explicit origin/branch may be published")
            destination=a.ref.removeprefix("origin/")
            actual=command(root,"ls-remote","origin","refs/heads/"+destination).decode()
            remotehead=actual.split()[0] if actual else None
            if remotehead!=refsha:
                raise ValueError("GitHub branch advanced after inventory; refetch first. Working tree remains unchanged")
            command(root,"push","origin",commit+":refs/heads/"+destination)
            print("RECOVERED_TO_GITHUB",commit)
    finally:
        if old is None:os.environ.pop("GIT_INDEX_FILE",None)
        else:os.environ["GIT_INDEX_FILE"]=old
        if os.path.exists(index):os.unlink(index)

if __name__=="__main__":
    try:main()
    except (ValueError,RuntimeError,OSError,KeyError,json.JSONDecodeError) as e:
        print("5★ CATALOG PRESERVATION BLOCKED:",str(e),file=sys.stderr)
        sys.exit(2)
