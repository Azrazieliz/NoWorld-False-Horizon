"""One native Gaea 2.3.1 Build after proving fresh QA project is already loaded.
Avoid gaea-mcp file dialog opening twice: launch Gaea.exe project path, verify title,
then use gaea-mcp's existing UI API for Build/Start Build. Evidence is Gaea build reports.
"""
import pathlib,time,json,hashlib,sys
import gaea_mcp.gaea as gaea
from gaea_mcp.config import find_gaea
from gaea_mcp.uia import Uia
root=pathlib.Path(r"D:\UnrealProjects\NW_Gaea_Terrain_Sandbox_20261015\Unified_Master_Run_20261015")
project=root/"NW_Gaea_QA_Graph.terrain"
report=root/"NW_Gaea_GenuineNativeBuild_Checked.json"
if report.exists():raise FileExistsError(report)
uia=Uia.ensure()
before=uia.run("title")
if project.name not in before:raise RuntimeError("Wrong active Gaea project: "+before)
hash_before=hashlib.sha256(project.read_bytes()).hexdigest()
def already_open(inst,name,ui):
 title=ui.run("title")
 if pathlib.Path(name).name not in title:raise RuntimeError("Active GUI project changed: "+title)
 return True,[]
gaea.open_project=already_open
start=time.perf_counter()
try:
 result=gaea.run_build(str(project),mode="keep_gui",wait_seconds=145,inst=find_gaea(),uia=uia)
 payload=result.to_dict()
except Exception as ex:
 import traceback
 payload={"ok":False,"stage":"uncaught","errors":[traceback.format_exc()]}
payload["wall_s"]=round(time.perf_counter()-start,2)
payload["source_unchanged"]=hashlib.sha256(project.read_bytes()).hexdigest()==hash_before
payload["loaded_title_before"]=before
payload["native_height_export_confirmed"]=bool(payload.get("ok") and any(str(x.get("path","")).lower().endswith((".png",".raw",".r16")) for x in payload.get("outputs",[])))
report.write_text(json.dumps(payload,indent=2,default=str),encoding="utf-8")
print("NW_GAEA_TRUE_NATIVE="+json.dumps({"ok":payload.get("ok"),"stage":payload.get("stage"),
 "seconds":payload["wall_s"],"outputs":len(payload.get("outputs",[])),
 "source_unchanged":payload["source_unchanged"],"report":str(report)}),flush=True)
sys.exit(0 if payload["native_height_export_confirmed"] else 2)
