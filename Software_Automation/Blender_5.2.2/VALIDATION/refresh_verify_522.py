"""Refresh/validate independent 5.2.2 LTS software mastery bundle.
No private character/YouTube media or third-party add-on sources included.
"""
import pathlib,json,hashlib,py_compile
root=pathlib.Path(__file__).resolve().parents[1]
files=[]
for p in sorted(root.rglob("*")):
 if not p.is_file() or p.name=="MANIFEST.json" or p.suffix in (".pyc",".blend1",".blend2") or "__pycache__" in p.parts:continue
 files.append({"relative_path":p.relative_to(root).as_posix(),"bytes":p.stat().st_size,
     "sha256":hashlib.sha256(p.read_bytes()).hexdigest()})
m={"project":"NoWorld False Horizon","software":"Blender",
 "version":"5.2.2 LTS","cloudrig_version":"2.2.31",
 "ue_version":"5.8.3","gaea_version":"2.3.1.0",
 "branch":"software/mastery-blender","date":"2026-10-15",
 "professional_demonstrations_accepted":0,
 "quality_target":"Studio-exceeding aspirational goal, not achieved",
 "file_count":len(files),"files":files}
(root/"MANIFEST.json").write_text(json.dumps(m,indent=2),encoding="utf-8")
bad=[]
for f in files:
 p=root/f["relative_path"]
 if hashlib.sha256(p.read_bytes()).hexdigest()!=f["sha256"]:bad.append(f["relative_path"])
 if p.suffix==".json":json.loads(p.read_text(encoding="utf-8-sig"))
 if p.suffix==".py":py_compile.compile(str(p),doraise=True)
check={"manifest_count":len(files),"bad_hashes":bad,
 "compatibility_passed":json.loads((root/"VALIDATION"/"blender522_compatibility_results.json").read_text())["passed"]==4,
 "cloudrig_passed":json.loads((root/"VALIDATION"/"cloudrig_522_inventory.json").read_text())["pass"],
 "native_unreal_events_passed":json.loads((root/"VALIDATION"/"ue_notify_authoring_results.json").read_text())["status"]=="PASS_TECHNICAL_TEST_ONLY"}
check["pass"]=len(bad)==0 and all([check["compatibility_passed"],check["cloudrig_passed"],check["native_unreal_events_passed"]])
print("NW_522_MANIFEST="+json.dumps(check))
if not check["pass"]:raise SystemExit(2)
