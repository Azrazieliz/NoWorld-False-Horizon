"""Refresh file hash manifest after a verified milestone."""
import json,hashlib,pathlib
root=pathlib.Path(__file__).resolve().parents[1]
items=[]
for p in sorted(root.rglob("*")):
 if not p.is_file() or p.name=="MANIFEST.json" or p.suffix in (".pyc",".blend1",".blend2") or "__pycache__" in p.parts:continue
 items.append({"relative_path":p.relative_to(root).as_posix(),"bytes":p.stat().st_size,
               "sha256":hashlib.sha256(p.read_bytes()).hexdigest()})
manifest={"project":"NoWorld False Horizon","blender":"4.5.10 LTS","unreal":"5.8.3",
          "branch":"software/mastery-blender","date":"2026-10-15",
          "acceptance":"TECHNICAL_VALIDATION_ONLY_PROFESSIONAL_ART_NOT_APPROVED",
          "professional_demonstrations_accepted":0,"file_count":len(items),"files":items}
(root/"MANIFEST.json").write_text(json.dumps(manifest,indent=2),encoding="utf-8")
print("MANIFEST_REFRESHED",len(items))
