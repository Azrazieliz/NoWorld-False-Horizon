"""Independent Gaea 2.3.1 terrain fixture, one new seed. No original overwrite."""
import json,pathlib,shutil,uuid,sys
base=pathlib.Path(r"D:\UnrealProjects\NW_Gaea_Terrain_Sandbox_20261015\Unified_Master_Run_20261015")
d=base.parent/"Gaea_FreshNativeBuild_Seed39813"
if d.exists():raise FileExistsError(d)
d.mkdir()
for name in ["NW_Gaea_Input_Height16.png","NW_Gaea_ErosionProtectMask16.png"]:
 shutil.copy2(base/name,d/name)
doc=json.loads((base/"NW_Gaea_QA_Graph.terrain").read_text(encoding="utf-8"))
asset=doc["Assets"]["$values"][0]
asset["Terrain"]["Id"]=str(uuid.uuid4())
asset["BuildDefinition"]["Destination"]=str(d/"exports")
asset["Terrain"]["Nodes"]["110"]["Seed"]=39813
for ident,name in [("130","NW_ErodedHeight"),("150","NW_RegionColors")]:
 asset["Terrain"]["Nodes"][ident]["OutputPath"]=str(d/"exports"/name)
p=d/"NW_GaeaFreshBuild_Seed39813.terrain"
p.write_text(json.dumps(doc,indent=2),encoding="utf-8")
print("NW_FRESH_FIXTURE="+str(p))
