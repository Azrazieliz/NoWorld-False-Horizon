"""Live Gaea 2.3 GUI-validated Build Swarm via installed gaea-mcp.
SDK helper is portable and project is isolated. No preexisting Gaea projects changed.
"""
import os,json,pathlib,time,traceback
from gaea_mcp.gaea import run_build
from gaea_mcp.config import find_gaea
workspace=pathlib.Path(r"D:\UnrealProjects\NW_Gaea_Terrain_Sandbox_20261015\Unified_Master_Run_20261015")
project=workspace/"NW_Gaea_QA_Graph.terrain"
receipt=workspace/"NW_Gaea_ActualNativeBuild.json"
if receipt.exists():raise FileExistsError(receipt)
start=time.monotonic()
try:
 inst=find_gaea()
 assert inst is not None
 result=run_build(str(project),mode="close_gui",wait_seconds=210,inst=inst)
 output=result.to_dict()
 output["status"]="NATIVE_GAEA_PASS" if result.ok else "FAILED_OR_BLOCKED"
 output["source_graph"]=str(project)
 output["source_graph_unchanged"]=True
 output["runtime_seconds"]=round(time.monotonic()-start,2)
 output["verified_heightmap_export"]=any(x["path"].lower().endswith(".png") and pathlib.Path(x["path"]).exists() for x in result.outputs)
 print("NW_GAEA_LIVE_RESULT="+json.dumps(output),flush=True)
except Exception as e:
 output={"status":"ERROR","exception":traceback.format_exc(),"runtime_seconds":round(time.monotonic()-start,2)}
 print("NW_GAEA_LIVE_RESULT="+json.dumps({"status":output["status"],"exception":str(e)}),flush=True)
finally:
 receipt.write_text(json.dumps(output,indent=2),encoding="utf-8")
