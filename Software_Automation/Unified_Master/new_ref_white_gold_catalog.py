import sqlite3,pathlib,collections
R=pathlib.Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME')
S=pathlib.Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014')
c=sqlite3.connect('file:'+str((R/'CATALOG'/'FULL_INSTALLED_MOD_LIBRARY.sqlite').as_posix())+'?mode=ro',uri=True)
terms=['veil','bride','bridal','wedding','belly','dancer','harem','arabian','goddess','greek','see through','transparent','sheer','lace','turquoise','gold jewelry','jewel','necklace','armlet','white dress','gold dress','bra','bikini','skirt','waist chain','earring','crown','headband','hip','long hair']
for t in terms:
 rows=c.execute("""select p.guid,p.relative_path,a.category_no,a.original_id,a.display_name from assets a join packages p on p.id=a.package_id where lower(a.display_name) like ? and a.category_no between 100 and 130 limit 100""",('%'+t+'%',)).fetchall()
 active=[v for v in rows if (S/'mods'/'SM01_SELECTED'/pathlib.PurePosixPath(v[1])).is_file()]
 print('TERM',t,'TOTAL',len(rows),'ACTIVE',len(active),flush=True)
 for a in active[:11]:print(' AVAILABLE',a[0],a[2],a[3],a[4],flush=True)
 for a in rows[:6]:
  if a not in active:print(' LIBRARY_ONLY',a[0],a[2],a[3],a[4],flush=True)
