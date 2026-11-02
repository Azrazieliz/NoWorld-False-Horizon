"""Register the nine actual user-created Koikatsu card files for new Luminous forms.
Source bytes and frozen Base/Alter production are never rewritten.
"""
import pathlib,hashlib,json
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
D=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party\UserData\chara\female")
names={
 "Awakened":{"CLOTHED":"20261020163646604","UNDERWEAR":"20261020163254893","NUDE":"20261020164224738"},
 "Berserker":{"CLOTHED":"20261020181953705","UNDERWEAR":"20261020190327674","NUDE":"20261020190405616"},
 "Grand":{"CLOTHED":"20261020202050734","UNDERWEAR":"20261020200911508","NUDE":"20261020200119808"}
}
out=R/"SourceAssets/Gate12/LuminousForms20261023/GATE12_USER_APPROVED_REAL_KOIKATSU_FIVE_FORM_SOURCE_MAP_20261023.json"
source=sorted(D.glob("*.png"))
doc={"schema":"noworld.gate12.luminous.koikatsu.approved_native_source_map.v1",
     "date":"2026-10-23","approval":"USER_EXPLICIT_20261023",
     "character":"Luminous Valentine",
     "policy":"PRESERVE_ALL_UNMODIFIED_SOURCE_CARDS",
     "forms":{},"imported_native_assets":[]}
for form,wardrobes in names.items():
 data={}
 for wardrobe,stamp in wardrobes.items():
  matches=[f for f in source if stamp in f.name]
  if len(matches)!=1:raise RuntimeError(f"Source match {form} {wardrobe}: {matches}")
  card=matches[0]
  assert card.stat().st_size>100000
  h=hashlib.sha256(card.read_bytes()).hexdigest()
  data[wardrobe]={"source_card":str(card),"sha256":h,
                  "bytes":card.stat().st_size,
                  "native_geometry":"REQUIRES_REAL_KKBP_EXPORT",
                  "wardrobe_is_not_a_version":True}
 doc["forms"][form]={"approved_character_version":True,
                    "origin":"ORIGINAL_KOIKATSU_CHARACTER_MAKER_SAVED_CARD",
                    "states":data}
 assert len(set(x["sha256"]for x in data.values()))==3
out.parent.mkdir(parents=True,exist_ok=True)
if out.exists():
 if json.loads(out.read_text(encoding="utf8"))!=doc:
  raise RuntimeError("Original card registry changed; refusing overwrite")
else:out.write_text(json.dumps(doc,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
print("REGISTERED",len(doc["forms"]),"UNIQUE_NATIVE_SOURCE_CARDS",sum(len(x["states"])for x in doc["forms"].values()))
print("REGISTRY",out)
