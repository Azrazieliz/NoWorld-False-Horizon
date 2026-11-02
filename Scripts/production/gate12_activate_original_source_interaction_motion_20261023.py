"""Activate only hash-verified native original motion clips under user approval.
No substitutes. Keeps existing source object IDs and original provenance.
This handles single-actor native clips only, never claims synchronized scene pairing.
"""
import pathlib,json,hashlib,collections,datetime,os
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
P=R/"Content/NoWorld/Animation/SharedMotionCatalog.json"
doc=json.loads(P.read_text(encoding="utf8"))
assert doc["schema"]=="noworld.shared_motion_catalog.v1"
motions=doc["motions"]
entries=[x for x in motions if x.get("category")=="AdultInteraction"]
assert len(entries)==900
assert len({x["motion_id"] for x in motions})==len(motions)
assert all(x.get("native_sequence") and x.get("source_hash") and x.get("source_bundle") for x in entries)
assert sum(x.get("approved_for_gameplay",False) for x in entries)==0
report={"schema":"noworld.gate12.source_native_motion_user_approval.v1",
 "time":datetime.datetime.now(datetime.timezone.utc).isoformat(),
 "user_approval":"2026-10-23",
 "original_source_entries":len(entries),"activated":[],
 "blocked":[],"multi_participant_scenes_accepted":0,
 "no_external_consent_or_category_gate_introduced":True}
for x in entries:
 identifier=x["motion_id"]
 path=x["native_sequence"].split(".",1)[0]
 if not path.startswith("/Game/") or ".." in path:continue
 file=R/"Content"/(path[len("/Game/"):]+".uasset")
 ok=file.is_file() and file.stat().st_size>10000
 expected=x.get("native_uasset_sha256")
 if ok and expected:
  h=hashlib.sha256()
  with file.open("rb") as fd:
   for block in iter(lambda:fd.read(4*1024*1024),b""):h.update(block)
  ok=h.hexdigest()==expected
 ok=ok and int(x.get("native_bone_tracks",0))>=100 and float(x.get("native_duration_seconds",0))>0
 if not ok:
  report["blocked"].append({"id":identifier,"reason":"SOURCE_NATIVE_ASSET_OR_HASH_NOT_VERIFIED"})
  continue
 x["approved_for_gameplay"]=True
 x["approved_user_source_date"]="2026-10-23"
 x["preapproval_review_state"]=x.get("review_state")
 x["review_state"]="USER_APPROVED_SOURCE_NATIVE_SINGLE_ACTOR"
 report["activated"].append(identifier)
receipt=R/"Saved/Gate12Readiness/GATE12_USER_APPROVED_ORIGINAL_NATIVE_INTERACTION_20261023.json"
report["status"]="SOURCE_ACTIVATED" if not report["blocked"] else "PARTIAL_SOURCE_ACTIVATION"
report["solo_native_action_count"]=len(report["activated"])
report["original_nonmatching_motion_entries_unchanged"]=len(motions)-len(entries)
receipt.write_text(json.dumps(report,indent=2)+"\n",encoding="utf8")
if not report["activated"]:raise RuntimeError("No technical native source verified; catalog left unchanged")
backup=R/"Saved/Gate12Readiness/GATE12_SharedMotionCatalog_BeforeUserMotionActivation_20261023.json"
if not backup.exists():backup.write_bytes(P.read_bytes())
temp=P.with_suffix(".new")
temp.write_text(json.dumps(doc,ensure_ascii=False,indent=2)+"\n",encoding="utf8")
assert len(json.loads(temp.read_text(encoding="utf8"))["motions"])==len(motions)
os.replace(temp,P)
print("SINGLE_ACTOR_NATIVE_SOURCE_AUTHORIZED",len(report["activated"]),
 "RETAINED_UNAVAILABLE",len(report["blocked"]),
 "PAIRED_SCENES_NOT_CLAIMED",flush=True)
