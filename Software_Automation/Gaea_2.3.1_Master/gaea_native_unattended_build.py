"""Fully unattended native Gaea 2.3.1 QA build on a preloaded isolated graph.
Uses UIAutomation for Build and Win32 only for selecting the existing eroded
node for export (keyboard F3). Source-hash guards and machine-resolution checks.
Real native Success/rebuilt output and genuine 16-bit values required.
"""
import time,pathlib,json,sys,hashlib,re
import numpy as np
from PIL import Image
import win32gui,win32api,win32con
from gaea_mcp.uia import Uia,activate_window
ROOT=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path(r"D:\UnrealProjects\NW_Gaea_Terrain_Sandbox_20261015\Gaea_FreshNativeBuild_Seed72931")
files=list(ROOT.glob("*.terrain"))
if len(files)!=1:raise RuntimeError("Expected exactly one Gaea terrain graph")
source=files[0]
record=ROOT/"NW_Gaea_FreshNative_AutomationReceipt.json"
if record.exists():raise FileExistsError(record)
before=hashlib.sha256(source.read_bytes()).hexdigest()
UI=Uia.ensure()
title=UI.run("title")
if title!="Gaea - "+source.name:raise RuntimeError("Refuse to work on another Gaea project: "+title)
win=[]
def enum(h,_):
 if win32gui.IsWindowVisible(h) and win32gui.GetWindowText(h)==title:win.append(h)
win32gui.EnumWindows(enum,None)
if len(win)!=1:raise RuntimeError("Cannot uniquely locate Gaea window")
hwnd=win[0]
win32gui.ShowWindow(hwnd,win32con.SW_MAXIMIZE)
activate_window(title)
time.sleep(.5)
l,t,r,b=win32gui.GetWindowRect(hwnd)
width,height=r-l,b-t
assert width>=1200 and height>=680,(width,height)
# Gaea 2.3.1 UI tree layout is WPF; graph has selected Erosion2 from file.
# All clicks relative to verified main window geometry, not global screen.
def click_relative(x,y):
 cx=l+int(x*width);cy=t+int(y*height)
 win32api.SetCursorPos((cx,cy))
 win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN,0,0,0,0)
 time.sleep(.07)
 win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP,0,0,0,0)
time.sleep(.6)
click_relative(.535,.927)
time.sleep(.4)
# Select native Erosion2 graph module, whose saved position is in this fixture.
click_relative(.12,.62)
time.sleep(.12)
def toggle_export():
 win32api.keybd_event(0x72,0,0,0)
 time.sleep(.14)
 win32api.keybd_event(0x72,0,win32con.KEYEVENTF_KEYUP,0)
 time.sleep(.4)
toggle_export()
status=UI.dump()
if "NaturalHydraulicErosion unmarked for export." in status:
 toggle_export()
 status=UI.dump()
if "NaturalHydraulicErosion marked for export." not in status:
 raise RuntimeError("Could not reliably mark Gaea Erosion2 node for export")
click_relative(.585,.927)
time.sleep(.6)
dump=UI.dump()
if "Build Settings" not in dump:raise RuntimeError("Native Build Settings unavailable")
if "NaturalHydraulicErosion" not in dump:raise RuntimeError("Native exported erosion target not listed")
result=UI.invoke("Build")
if "INVOKED" not in result:raise RuntimeError("Native Build button did not invoke: "+result)
# The Windows TaskDialog Start Build control is a Pane without InvokePattern.
# Resolve the named confirmation window, then click the centre of its command link.
confirm=[]
for _ in range(22):
 confirm.clear()
 def discover(h,_):
  if win32gui.IsWindowVisible(h) and win32gui.GetWindowText(h)=="Build and Export?":
   confirm.append(h)
 win32gui.EnumWindows(discover,None)
 if confirm:break
 time.sleep(.25)
if len(confirm)!=1:raise RuntimeError("Missing native Build and Export confirmation")
rect=win32gui.GetWindowRect(confirm[0])
dx,dy=rect[2]-rect[0],rect[3]-rect[1]
if not (350<dx<800 and 250<dy<700):raise RuntimeError("Unexpected confirmation size")
win32api.SetCursorPos((rect[0]+int(dx*.32),rect[1]+int(dy*.535)))
win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN,0,0,0,0)
time.sleep(.09)
win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP,0,0,0,0)
time.sleep(.4)
# Wait for Gaea's own report and required outputs.
target=ROOT/"exports"
required=["NW_ErodedHeight.png","NW_RegionColors.png","NaturalHydraulicErosion_Out.exr",
          "NaturalHydraulicErosion_Flow.exr","NaturalHydraulicErosion_Wear.exr",
          "NaturalHydraulicErosion_Deposits.exr","report.json"]
start=time.monotonic()
while time.monotonic()-start<135:
 if all((target/n).is_file() and (target/n).stat().st_size>1000 for n in required):break
 time.sleep(2)
else:
 raise TimeoutError("No complete Gaea native exports generated before 135s")
native_report=json.loads((target/"report.json").read_text(encoding="utf-8-sig"))
img=Image.open(target/"NW_ErodedHeight.png")
orig=Image.open(ROOT/"NW_Gaea_Input_Height16.png")
if img.size!=(512,512) or (target/"NW_ErodedHeight.png").read_bytes()[24]!=16:
 raise RuntimeError("Native height PNG must be true 512x512 16-bit")
a=np.asarray(orig,dtype=np.int64)
b=np.asarray(img,dtype=np.int64)
changed=int(np.count_nonzero(a!=b))
if changed<50000:raise RuntimeError("Terrain did not materially change with erosion")
if native_report.get("Result") not in ("Success","AlreadyBuilt"):
 raise RuntimeError("Gaea reported native failure "+str(native_report.get("Result")))
if hashlib.sha256(source.read_bytes()).hexdigest()!=before:
 raise RuntimeError("Input Gaea project was mutated")
out={"status":"PASS_NATIVE_GAEA_AUTOMATED",
 "native_result":native_report["Result"],
 "native_node_count":native_report["NodeCount"],
 "real_height_bitdepth":16,"height_size":[512,512],
 "changed_height_pixels":changed,"elapsed_seconds":round(time.monotonic()-start,1),
 "exported_outputs":[{"file":n,"bytes":(target/n).stat().st_size,
  "sha256":hashlib.sha256((target/n).read_bytes()).hexdigest()} for n in required],
 "source_sha256_unchanged":True,
 "game_geography_approved":False,"game_terrain_installed":False}
record.write_text(json.dumps(out,indent=2),encoding="utf-8")
print("NOWORLD_GAEA_NATIVE_AUTOMATED="+json.dumps({k:v for k,v in out.items() if k!="exported_outputs"}),flush=True)
