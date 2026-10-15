import sqlite3
db=r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME\CATALOG\FULL_INSTALLED_MOD_LIBRARY.sqlite"
c=sqlite3.connect(db)
searches=[
("braid","cast(a.category_no as int) between 100 and 130",["braid","braided","plait","rapunzel"]),
("shirt","a.category_no in ('105','106')",["t-shirt","tshirt","shirt","wet","see through","transparent"]),
("hoops","cast(a.category_no as int) between 120 and 130",["hoop","earring","ear ring","circle"]),
("socks","a.category_no in ('110','111')",["thigh","high","stocking","socks"]),
]
for title,limit,keywords in searches:
 qry="select p.guid,a.category_no,a.original_id,a.display_name from assets a join packages p on a.package_id=p.id where ("+limit+") and ("+" or ".join(["lower(a.display_name) like ?"]*len(keywords))+") limit 85"
 print("\n##",title)
 for x in c.execute(qry,["%"+k+"%" for k in keywords]):print("|".join(str(v) for v in x))
