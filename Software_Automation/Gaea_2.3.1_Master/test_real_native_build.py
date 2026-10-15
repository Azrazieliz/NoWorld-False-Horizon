"""NoWorld Gaea Native QA. Licensed installed Gaea 2.3.1 GUI + gaea-mcp helper.
Real build success requires Gaea report.json and at least one exported height PNG.
Writes time-stamped receipt. Does not modify source graph or game.
"""
import pathlib,json,os,sys,time,hashlib
from gaea_mcp.gaea import run_build
from gaea_mcp.config import find_gaea
folder=pathlib.Path(r"D:\UnrealProjects\NW_Gaea_Terrain_Sandbox_20261015\Unified_Master_Run_20261015")
src=folder/"NW_Gaea_QA_Graph.terrain"
stamp=time.strftime("%Y%m%d_%H%M%S")
result=folder/("NW_Gaea_NativeBuild_"+stamp+".json")
assert src.exists()
start=time.perf_counter()
try:
 install=find_gaea()
 build=run_build(str(src),mode="close_gui",wait_seconds=240,inst=install)
 data=build.to_dict()
 data["status"]="NATIVE_GAEA_BUILD_PASS" if build.ok and any(v["path"].lower().endswith(".png") for v in build.outputs) else "NATIVE_GAEA_INCOMPLETE"
 data["installed_version"]=str(install)
except Exception as exc:
 import traceback
 data={"status":"NATIVE_GAEA_FAIL","reason":repr(exc),"traceback":traceback.format_exc()}
data["seconds_elapsed"]=round(time.perf_counter()-start,1)
data["original_graph_sha256"]=hashlib.sha256(src.read_bytes()).hexdigest()
result.write_text(json.dumps(data,indent=2,default=str),encoding="utf-8")
print("NW_GAEA_NATIVE_BUILD_RESULT="+json.dumps({"status":data["status"],"seconds":data["seconds_elapsed"],"report":str(result),"outputs":len(data.get("outputs",[])),"stage":data.get("stage")}),flush=True)
if data["status"]!="NATIVE_GAEA_BUILD_PASS":sys.exit(2)
