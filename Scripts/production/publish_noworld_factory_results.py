#!/usr/bin/env python3
"""Copy ONLY generated/approved native factory products from authorized PC
Unreal project into GitHub checkout, for Git LFS commit by CI.
No raw Koikatsu source cards, Blender scenes, credentials or unrelated PC files.
Refuse existing binary conflicts and mismatched original source IDs.
"""
import argparse
import hashlib
import json
import pathlib
import shutil
import re

REPO=pathlib.Path(__file__).resolve().parents[2]
def read(p):return json.loads(pathlib.Path(p).read_text("utf-8-sig"))
def sha(p):
    with pathlib.Path(p).open("rb") as f:
        return hashlib.file_digest(f,"sha256").hexdigest()
def copy_immutable(source,dest):
    source=source.resolve(strict=True)
    if not source.is_file():raise ValueError("Expected true native source package")
    if dest.exists():
        if sha(source)!=sha(dest):
            raise FileExistsError("Never overwrite a GitHub-published approved package: "+str(dest))
        return False
    dest.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(source,dest)
    if sha(source)!=sha(dest):raise IOError("Native package copy SHA256 mismatch")
    return True
def export_job(job,pc):
    data=read(job)
    if data.get("schema")!="noworld.factory.job.v1" or data.get("publish_to_github") is not True:
        return []
    ident=data["job_id"]
    if not re.fullmatch(r"[a-z0-9][a-z0-9_.-]{2,80}",ident):
        raise ValueError("Untrusted published job ID")
    state=read(pathlib.Path(pc)/"Saved/NoWorldContentFactory"/ident/"status.json")
    if state.get("state")!="SOURCE_ASSETS_INTEGRATED_NOT_PLAYABILITY_ACCEPTED":
        raise ValueError("Cannot publish an incomplete/no-source job: "+ident)
    created=[]
    native=pc/"Content/NoWorld/ContentFactory"
    if data["kind"] in ("character","bundle","artwork"):
        receipts=pathlib.Path(pc)/"Saved/NoWorldContentFactory"/ident
        for name in ("unreal_receipt.json","artwork_receipt.json"):
            proof=receipts/name
            if not proof.is_file():continue
            row=read(proof)
            if row.get("status") not in (
                "NATIVE_ORIGINAL_CHARACTER_SOURCE_ASSET_REGISTERED",
                "ORIGINAL_ARTWORK_UASSET_IMPORTED"):
                raise ValueError("Native original asset not actually saved: "+name)
            # Copy all importer-created skeletal materials, textures, native
            # skeletons and geometry; not just the top-level mesh.
            paths=list(row.get("assets",[]))
            if row.get("appearance"):paths.append(row["appearance"])
            for obj in paths:
                pkg=obj.rsplit(".",1)[0]
                if not pkg.startswith("/Game/NoWorld/ContentFactory/"):
                    raise ValueError("Non-factory asset path rejected: "+obj)
                physical=pc/("Content/"+pkg.removeprefix("/Game/")+".uasset")
                # Importer may produce ancillary assets in the same dedicated
                # job folder. Include these, otherwise cooked materials vanish.
                parent=physical.parent
                if not parent.is_dir():raise FileNotFoundError(str(physical))
                for f in parent.rglob("*.uasset"):
                    rel=f.relative_to(pc)
                    if "NoWorld/ContentFactory" not in rel.as_posix():
                        raise ValueError("Unexpected source asset root: "+str(f))
                    if copy_immutable(f,REPO/rel):created.append(rel.as_posix())
        index=pc/"Content/NoWorld/ContentFactory/Characters.json"
        if index.exists():
            src=read(index)
            target=REPO/"Content/NoWorld/ContentFactory/Characters.json"
            dst=read(target) if target.exists() else {
                "schema":"noworld.content_factory.index.v1","characters":[]}
            if src.get("schema")!=dst.get("schema"):
                raise ValueError("Incompatible factory character registry")
            for row in src.get("characters",[]):
                if row.get("identity")!=data.get("identity") or row.get("version")!=data.get("version"):
                    continue
                found=[r for r in dst["characters"] if r["identity"]==row["identity"] and r["version"]==row["version"]]
                if found and found[0]!=row:raise ValueError("Existing approved character registration differs")
                if not found:dst["characters"].append(row)
            target.parent.mkdir(parents=True,exist_ok=True)
            target.write_text(json.dumps(dst,indent=2,ensure_ascii=False)+"\n","utf-8")
            created.append(target.relative_to(REPO).as_posix())
    if data.get("motions_recipe"):
        from importlib.util import spec_from_file_location,module_from_spec
        # Merge only the newly authored, source-approved motion IDs,
        # using exactly the immutable recipes already accepted by the binder.
        source=pc/"Content/NoWorld/Animation/SharedMotionCatalog.json"
        target=REPO/"Content/NoWorld/Animation/SharedMotionCatalog.json"
        if not source.is_file():raise FileNotFoundError(str(source))
        native_motions=read(source)
        current=read(target)
        recipe=read(REPO/data["motions_recipe"])
        ids={r["motion_id"].casefold() for r in recipe["motions"]}
        approved=[m for m in native_motions["motions"] if m["motion_id"].casefold() in ids]
        if len(approved)!=len(ids):raise ValueError("Some authored native actions never entered Unreal catalog")
        prior={m["motion_id"].casefold():m for m in current["motions"]}
        for motion in approved:
            key=motion["motion_id"].casefold()
            if key in prior and prior[key].get("source_clip_id")!=motion["source_clip_id"]:
                raise ValueError("Cannot overwrite a different original source motion")
            prior[key]=motion
        current["motions"]=sorted(prior.values(),key=lambda m:m["motion_id"].casefold())
        target.write_text(json.dumps(current,indent=2,ensure_ascii=False)+"\n","utf-8")
        created.append(target.relative_to(REPO).as_posix())
    report=REPO/"docs/production/content_receipts"/(ident+".json")
    report.parent.mkdir(parents=True,exist_ok=True)
    report.write_text(json.dumps({"schema":"noworld.content_factory.published.v1",
        "job_id":ident,"identity":data.get("identity"),"version":data.get("version"),
        "original_source_sha256":[x["sha256"] for x in data["sources"]],
        "created_repository_assets":sorted(set(created)),
        "playable_game_verified":False,"owner_accepted":False},indent=2)+"\n","utf-8")
    created.append(report.relative_to(REPO).as_posix())
    return sorted(set(created))

def main():
    a=argparse.ArgumentParser(description=__doc__)
    a.add_argument("--inbox",default="ContentFactory/Jobs")
    a.add_argument("--native-project",default=r"D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031")
    args=a.parse_args()
    pc=pathlib.Path(args.native_project).resolve(strict=True)
    if not (pc/"OfflineGame.uproject").is_file():raise ValueError("Not actual Unreal native project")
    results=[]
    for job in sorted(pathlib.Path(args.inbox).glob("*.json")):
        results+=export_job(job,pc)
    print(json.dumps({"status":"SOURCE_ARTIFACTS_STAGED_FOR_GIT_LFS",
                      "files":sorted(set(results))},indent=2))
if __name__=="__main__":main()
