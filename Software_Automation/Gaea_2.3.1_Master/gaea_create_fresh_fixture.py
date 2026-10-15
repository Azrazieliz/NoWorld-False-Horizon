"""Create isolated fresh Gaea graph from tested 512 project to invalidate native cache.
Does not modify any existing Gaea project; preserves all node and export contracts.
"""
import json,pathlib,shutil,uuid,hashlib
source=pathlib.Path(r"D:\UnrealProjects\NW_Gaea_Terrain_Sandbox_20261015\Unified_Master_Run_20261015")
destination=source.parent/"Gaea_FreshNativeBuild_Seed72931"
if destination.exists():raise FileExistsError(destination)
destination.mkdir()
for name in ["NW_Gaea_Input_Height16.png","NW_Gaea_ErosionProtectMask16.png"]:
 shutil.copy2(source/name,destination/name)
d=json.loads((source/"NW_Gaea_QA_Graph.terrain").read_text(encoding="utf-8"))
asset=d["Assets"]["$values"][0]
asset["Terrain"]["Id"]=str(uuid.uuid4())
asset["BuildDefinition"]["Destination"]=str(destination/"exports")
node=asset["Terrain"]["Nodes"]["110"]
node["Seed"]=72931
# Distinct output paths for native Gaea Export nodes.
for id,name in [("130","NW_ErodedHeight"),("150","NW_RegionColors")]:
 asset["Terrain"]["Nodes"][id]["OutputPath"]=str(destination/"exports"/name)
output=destination/"NW_GaeaFreshBuild_Seed72931.terrain"
output.write_text(json.dumps(d,indent=2,ensure_ascii=False),encoding="utf-8")
print("GAEA_FRESH_PROJECT_CREATED="+str(output))
print("GAEA_FRESH_SOURCE_SHA256="+hashlib.sha256(output.read_bytes()).hexdigest())
