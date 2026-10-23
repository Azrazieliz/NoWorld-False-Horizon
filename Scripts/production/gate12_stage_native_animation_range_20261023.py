import argparse,json,pathlib,subprocess
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
PLAN=R/"SourceAssets/Gate12/Animations20261023/KoikatsuExtendedSourcePlan.json"
a=argparse.ArgumentParser()
a.add_argument("--start",type=int,required=True)
a.add_argument("--end",type=int,required=True)
b=a.parse_args()
rows=json.loads(PLAN.read_text("utf8"))["motions"]
assert 0<=b.start<b.end<=len(rows)
paths=["Content/NoWorld/Animation/SharedMotionCatalog.json"]
prefix="Content/NoWorld/Characters/Luminous/Alter/KoikatsuOriginalStudioMotionQA/AN_G12_KK_Original_"
paths += [prefix+s+"_"+item["source_clip"]+".uasset"
          for item in rows[b.start:b.end] for s in ("CLOTHED","UNDERWEAR","NUDE")]
missing=[x for x in paths if not (R/x).is_file()]
assert not missing,missing[:6]
for i in range(0,len(paths),12):
    subprocess.run(["git","-C",str(R),"add","--",*paths[i:i+12]],check=True,capture_output=True)
subprocess.run(["git","-C",str(R),"diff","--cached","--check"],check=True)
print("G12_STAGED_REAL_UNREAL_ASSETS",len(paths)-1,"clips",b.end-b.start,flush=True)
