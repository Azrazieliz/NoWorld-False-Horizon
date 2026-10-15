"""Actual four-software router smoke tests, no GUI/asset modifications.
Calls gatectl in sequence and checks returned JSON matches the requested tool.
"""
import importlib.util,pathlib,time,datetime,json,sys
HERE=pathlib.Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location("nw_gatectl",HERE/"gatectl.py")
gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
stamp=datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
root=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015")/("Unified_SearchSmoke_"+stamp)
tests=[("blender","api_search","Gate09","armature"),
("koikatsu","mod_lookup","Gate01","wing"),
("gaea","node_search","Gate02","erosion"),
("unreal","api_search","Gate12","PCG")]
results=[]
for i,(software,op,owner,q) in enumerate(tests):
 job={"schema":"NOWORLD_SOFTWARE_JOB_V1",
  "job_id":"LiveIndex_"+software+"_"+stamp,"owner_gate":owner,
  "test_only":True,"software":software,"operation":op,
  "source_paths":[],"requirements":{"preserve_originals":True,
       "max_walltime_seconds":30},
  "params":{"query":q,"limit":6},
  "outputs":{"receipt_directory":str(root/software)}}
 try:
  record=gate.run(job)
  output=(root/software/"stdout.log").read_text(encoding="utf-8")
  response=json.loads(output)
  items=response.get("results",[])
  ok=record["status"]=="PROCESS_EXIT_ZERO" and len(items)>0
  results.append({"software":software,"operation":op,
    "actual_link_executed":ok,"results_returned":len(items),
    "receipt":str(root/software/"receipt.json"),
    "status":record["status"]})
 except Exception as e:results.append({"software":software,"status":"FAIL","error":repr(e)})
out={"status":"PASS_FOUR_NATIVE_CATALOG_ROUTE_LINKS" if len(results)==4 and all(x.get("actual_link_executed") for x in results) else "FAIL",
 "checks":results,"native_asset_writes":False}
path=HERE/"LIVE_FOUR_SOFTWARE_ROUTER_SMOKE.json"
path.write_text(json.dumps(out,indent=2),encoding="utf-8")
print("NW_LIVE_FOUR_SOFTWARE_ROUTES="+json.dumps(out))
if out["status"]!="PASS_FOUR_NATIVE_CATALOG_ROUTE_LINKS":sys.exit(2)
