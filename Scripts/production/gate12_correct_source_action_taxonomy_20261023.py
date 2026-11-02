"""Correct demonstrably false source-name semantic matches without renaming,
reimporting, enabling or changing original clips. Keep immutable batch indices.
"""
import json,pathlib,sys
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
sys.path.insert(0,str(R/"Scripts/production"))
from gate12_expand_missing_koikatsu_native_source import semantic
planpath=R/"SourceAssets/Gate12/Animations20261023/KoikatsuExtendedSourcePlan.json"
catalogpath=R/"Content/NoWorld/Animation/SharedMotionCatalog.json"
plan=json.loads(planpath.read_text("utf8"))
catalog=json.loads(catalogpath.read_text("utf8"))
assert len({e["source_clip"] for e in plan["motions"]})==len(plan["motions"])
byid={e["motion_id"]:e for e in catalog["motions"]}
corrected=[]
count=0
for item in plan["motions"]:
    candidate=semantic({"source_clip_name":item["original_clip_name"],
                        "source_family":item["source_family"]})
    if candidate is None:
        # Source identity remains cataloged as unclassified QA material;
        # never invent a combat role to satisfy the taxonomy.
        continue
    kind,context,cap,_=candidate
    alias=item["source_clip"]
    action=kind+"."+alias
    changed=(item["action"],item["context"],item["capability"])!=(action,context,cap)
    if not changed:continue
    corrected.append((alias,item["original_clip_name"],item["action"],action))
    item.update({"action":action,"context":context,"capability":cap,
                 "semantic_category":kind})
    for outfit in ("CLOTHED","UNDERWEAR","NUDE"):
        key=f"kk.mixamo.qa.{outfit}.{alias}"
        entry=byid.get(key)
        if not entry:continue
        assert entry["approved_for_gameplay"] is False
        assert entry["source_original_name"]==item["original_clip_name"]
        entry.update({"action":action,"context":context,"category":context,
                      "capabilities":[cap]})
        count+=1
for p,obj in ((planpath,plan),(catalogpath,catalog)):
    tmp=p.with_suffix(p.suffix+".tmp")
    tmp.write_text(json.dumps(obj,ensure_ascii=False,indent=2)+"\n",encoding="utf8")
    json.loads(tmp.read_text("utf8"))
    tmp.replace(p)
print("SOURCE_SEMANTIC_CORRECTIONS",len(corrected),
      "NATIVE_CATALOG_ENTRIES_CORRECTED",count,flush=True)
for item in corrected[:30]:print("SEMANTIC_REPAIR",item,flush=True)
