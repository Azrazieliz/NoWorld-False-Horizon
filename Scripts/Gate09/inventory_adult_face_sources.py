"""Gate09 installed adult/facial source dependency inventory. Never copies protected mod payloads."""
from pathlib import Path
import hashlib,json,collections,re,zipfile
game=Path(r'D:\Tools\Steam\steamapps\common\Koikatsu Party')
out=Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for block in iter(lambda:f.read(4*1024*1024),b''):h.update(block)
 return h.hexdigest()
mod_root=game/'mods'
packages=sorted(p for p in mod_root.rglob('*.zipmod') if 'BACKUP' not in str(p).upper())
names=('SEX','H_','H-', 'ANIM','KPLUG','POSE','FACE','ADULT','BETTERPENETRATION')
candidates=[p for p in packages if 'Animations' in str(p) or any(t in p.name.upper() for t in names)]
plugins=[game/'BepInEx/plugins/KK_SexFaces.dll',
         game/'BepInEx/plugins/PoseLib.KK.dll',
         game/'BepInEx/plugins/Timeline.dll',
         game/'BepInEx/plugins/AnimationLoader.Koikatu.dll']
moddirs=['Sideloader Modpack - Animations','BetterPenetration','MyMods']
rows=[]
for p in candidates:
 if not p.is_file():continue
 rel=p.relative_to(mod_root)
 try:
  with zipfile.ZipFile(p) as z:parts=collections.Counter(Path(x.filename).suffix.lower() for x in z.infolist())
  valid=True
 except Exception:parts={};valid=False
 rows.append({'relative_path':str(rel),'bytes':p.stat().st_size,'sha256':sha(p),
   'zip_readable':valid,'archive_entries_by_extension':dict(parts),
   'rights_status':'UNVERIFIED_NO_REDISTRIBUTION'})
 print('INDEXED',rel,flush=True)
hroot=game/'abdata/h'
native=[p for p in hroot.rglob('*') if p.is_file()] if hroot.exists() else []
pmeta=[{'path':str(p),'exists':p.exists(),'bytes':p.stat().st_size if p.exists() else None,
   'sha256':sha(p) if p.exists() else None} for p in plugins]
manifest={'schema':'GATE09_INSTALLED_ADULT_MOTION_ASSET_PROVENANCE_V1',
 'game_directory':str(game),'total_installed_zipmod_packages':len(packages),
 'candidate_adult_animation_or_face_packages':rows,'native_h_directory':str(hroot),
 'native_h_file_count':len(native),
 'native_h_extensions':dict(collections.Counter(x.suffix.lower() for x in native)),
 'native_h_total_bytes':sum(x.stat().st_size for x in native),
 'plugins':pmeta,
 'local_source_transfer':'SOURCE_INDEX_ONLY_PAYLOADS_LEFT_INSTALLED_IN_PLACE',
 'license_review':'REQUIRED_PER_3RD_PARTY_PACKAGE_BEFORE_EMBEDDING_OR_REDISTRIBUTION',
 'character_eligibility':'AGE_VERIFICATION_REQUIRED_BEFORE_ADULT_CHARACTER_ANIMATIONS',
 'unreal_bake':'NOT_TESTED',
 'facial_capability':'KK_SexFaces installed; separate native facial / morph mapping required'}
file=out/'G09_Koikatsu_Adult_Face_Mod_Source_Inventory_20261019.json'
if file.exists():raise FileExistsError(file)
file.write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
print('MANIFEST',str(file),'candidate',len(rows),'all_zipmods',len(packages),'native_H_files',len(native),'sha',sha(file))
