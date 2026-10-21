import json,struct
p=r"D:\Tools\GameAssetPipeline\KoikatsuParty\characters\G01-KK-PIPELINE-TEST-0002\conversion\character_unreal_skeleton.glb"
with open(p,"rb") as f:
 f.seek(12);n,k=struct.unpack("<II",f.read(8));d=json.loads(f.read(n))
s=d["skins"][0]
print("BONE_COUNT",len(s["joints"]))
for i,j in enumerate(s["joints"]):
 n=d["nodes"][j]["name"]
 if i<30 or any(t in n.lower() for t in ["spine","arm","upper","shoulder","head","neck","eye","hand","leg","foot","hip"]):print(i,n)
print("ANIMATIONS",len(d.get("animations",[])))
