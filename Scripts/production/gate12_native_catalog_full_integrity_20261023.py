"""Verify shared library refers to actual native Unreal assets and preserve
source provenance. Does not visually approve animation or enable game actions.
"""
import collections,hashlib,json,pathlib
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
catalog=R/"Content/NoWorld/Animation/SharedMotionCatalog.json"
data=json.loads(catalog.read_text("utf8"))
assert data["schema"]=="noworld.shared_motion_catalog.v1"
entries=data["motions"]
seen=set();errors=[];verified=0;with_hash=0;legacy=0
grouped=collections.Counter()
qa_violations=[]
for e in entries:
    motion=e["motion_id"]
    if motion in seen:
        errors.append({"id":motion,"error":"duplicate_id"});continue
    seen.add(motion)
    asset=e["native_sequence"].split(".")[0]
    if not asset.startswith("/Game/"):
        errors.append({"id":motion,"error":"bad_native_package_namespace"});continue
    native=R/"Content"/(asset[6:]+".uasset")
    if not native.is_file():
        errors.append({"id":motion,"error":"native_uasset_missing"});continue
    expected=e.get("native_uasset_sha256")
    if expected:
        with_hash+=1
        if hashlib.sha256(native.read_bytes()).hexdigest()!=expected:
            errors.append({"id":motion,"error":"native_binary_digest_mismatch"});continue
    else:legacy+=1
    family="h_exact" if ".h_exact." in motion else "mixamo" if ".mixamo." in motion else "dance" if ".dance." in motion else "earlier"
    grouped[family]+=1
    if family in ("h_exact","mixamo","dance") and e.get("approved_for_gameplay"):
        qa_violations.append(motion)
    if family=="h_exact" and not e.get("adult_restricted"):
        qa_violations.append(motion)
    verified+=1
out={
 "schema":"noworld.gate12.native_shared_catalog_integrity.v1",
 "catalog_entries":len(entries),"native_file_verified":verified,
 "file_sha256_verified":with_hash,"legacy_without_asset_hash":legacy,
 "by_family":dict(grouped),"games_active":sum(x.get("approved_for_gameplay",False) for x in entries),
 "qa_gated":sum(not x.get("approved_for_gameplay",False) for x in entries),
 "errors":errors,"qa_policy_violations":qa_violations,
 "visual_quality":"NOT_VERIFIED","gameplay_animation_wiring":"NOT_ACCEPTED_FOR_QA_NEW_MOTIONS",
 "paired_scene_visual_approval":"NOT_VERIFIED",
 "result":"NATIVE_INTEGRITY_PASS" if not errors and not qa_violations else "NATIVE_INTEGRITY_PARTIAL"
}
file=R/"Saved/Gate12Readiness/GATE12_ALL_NATIVE_CATALOG_INTEGRITY_20261023.json"
file.parent.mkdir(parents=True,exist_ok=True)
file.write_text(json.dumps(out,indent=2)+"\n",encoding="utf8")
print("GATE12_ALL_NATIVE_CHECK",out["result"],"verified",verified,
 "catalog",len(entries),"sha256",with_hash,"errors",len(errors),"policy",len(qa_violations),"families",dict(grouped))
