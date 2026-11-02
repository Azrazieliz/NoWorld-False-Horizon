#!/usr/bin/env python3
"""NoWorld 5★ neutralize historic animation QA flags in ONE global catalog.

Authoring approval ==/= owner phone-test review. Removes only obsolete
approved_for_gameplay fields. Never invents owner acceptance/rejection, adjusts
combat contacts, changes source IDs, or reimports a real asset. The old metadata
is retained in Git history.

Operate on the current fetched remote Git tree, and create a descendant commit
via a temporary index. The PC native worktree remains untouched.
"""
from __future__ import annotations
import argparse, json, os, pathlib, subprocess, tempfile, hashlib, sys

CATALOG="Content/NoWorld/Animation/SharedMotionCatalog.json"

def call(root,*args,input=None):
    r=subprocess.run(["git","-C",str(root),*args],input=input,stdout=subprocess.PIPE,
                     stderr=subprocess.PIPE)
    if r.returncode:raise ValueError("Git failed: "+" ".join(args[:3])+": "+r.stderr.decode("utf8","replace")[:240])
    return r.stdout.strip()

def load(raw):
    data=json.loads(raw.decode("utf-8-sig"))
    if data.get("schema")!="noworld.shared_motion_catalog.v1" or not isinstance(data.get("motions"),list):
        raise ValueError("Expected the existing single original motion source schema")
    ids=[m["motion_id"].casefold() for m in data["motions"]]
    if len(ids)!=len(set(ids)):raise ValueError("Original motion IDs were not unique")
    return data

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--native-project",type=pathlib.Path,required=True)
    p.add_argument("--ref",default="origin/work/gate12-whole-character-shared-motion-20261031")
    p.add_argument("--publish",action="store_true")
    a=p.parse_args()
    root=a.native_project.resolve(strict=True)
    if not (root/"OfflineGame.uproject").is_file():raise ValueError("Not the real Unreal project checkout")
    parent=call(root,"rev-parse","--verify",a.ref+"^{commit}").decode()
    old=call(root,"show",parent+":"+CATALOG)
    data=load(old)
    original=[(m["motion_id"],m.get("source_clip_id"),m.get("native_sequence"),m.get("contact_seconds"),m.get("contact_notify")) for m in data["motions"]]
    changed=0
    for m in data["motions"]:
        if "approved_for_gameplay" in m:
            del m["approved_for_gameplay"]
            changed+=1
    if not changed:
        print(json.dumps({"status":"ALREADY_NEUTRAL","entries":len(original),"parent":parent}))
        return
    after=[(m["motion_id"],m.get("source_clip_id"),m.get("native_sequence"),m.get("contact_seconds"),m.get("contact_notify")) for m in data["motions"]]
    if original!=after:raise ValueError("Migration unexpectedly changed original animation identity/contact data")
    payload=(json.dumps(data,ensure_ascii=False,indent=2)+"\n").encode("utf8")
    print(json.dumps({"schema":"noworld.5star.motion_owner_review_migration.v1","total_entries":len(original),
          "legacy_flags_removed":changed,"new_owner_acceptance":0,"new_owner_rejection":0,
          "original_source_hash_before":hashlib.sha256(old).hexdigest(),
          "after_hash":hashlib.sha256(payload).hexdigest(),"pc_native_worktree_modified":False,
          "publish_requested":a.publish},indent=2))
    if not a.publish:return
    if not a.ref.startswith("origin/"):raise ValueError("Only explicit origin/branch may be published")
    branch=a.ref.removeprefix("origin/")
    remote=call(root,"ls-remote","origin","refs/heads/"+branch).decode()
    if not remote or remote.split()[0]!=parent:raise ValueError("Remote branch moved; fetch again, refuse overwrite")
    blob=call(root,"hash-object","-w","--stdin",input=payload).decode()
    index=tempfile.mktemp(prefix="noworld_neutral_review_index_")
    oldindex=os.environ.get("GIT_INDEX_FILE")
    try:
        os.environ["GIT_INDEX_FILE"]=index
        call(root,"read-tree",parent)
        call(root,"update-index","--add","--cacheinfo","100644,"+blob+","+CATALOG)
        tree=call(root,"write-tree").decode()
        commit=call(root,"-c","user.name=NoWorld source migration",
                    "-c","user.email=noworld-source-migration@users.noreply.github.com",
                    "commit-tree",tree,"-p",parent,
                    "-m","Neutralize legacy motion preapprovals while retaining all original source and contact data").decode()
        call(root,"push","origin",commit+":refs/heads/"+branch)
        print("GITHUB_MIGRATION_COMMIT",commit)
    finally:
        if oldindex is None:os.environ.pop("GIT_INDEX_FILE",None)
        else:os.environ["GIT_INDEX_FILE"]=oldindex
        if os.path.isfile(index):os.unlink(index)

if __name__=="__main__":
    try:main()
    except (ValueError,OSError,json.JSONDecodeError) as e:
        print("5star motion review migration blocked:",e,file=sys.stderr);sys.exit(2)
