"""NoWorld Gaea 2.3 GUI File/Open focus recovery.
Repairs Windows foreground lock from always-on-top elapsed clock.
Does not alter the countdown or source project.
"""
import win32gui,win32con,win32process,ctypes,time,subprocess,pathlib,json,os,sys
out=pathlib.Path(r"D:\UnrealProjects\NW_Gaea_Terrain_Sandbox_20261015\Unified_Master_Run_20261015")
project=out/"NW_Gaea_QA_Graph.terrain"
u=pathlib.Path(r"D:\UnrealProjects\NW_Blender_ReferenceReview_20261015\external_sources\gaea-mcp\src\gaea_mcp\uia\bin\uia.exe")
def windows():
 result=[]
 def cb(hwnd,_):
  if win32gui.IsWindowVisible(hwnd):result.append((hwnd,win32gui.GetWindowText(hwnd)))
 win32gui.EnumWindows(cb,None)
 return result
# Release timer from always-on-top without stopping it.
for hwnd,title in windows():
 if "Integration 4h30 Deadline" in title:
  win32gui.SetWindowPos(hwnd,win32con.HWND_NOTOPMOST,0,0,0,0,
   win32con.SWP_NOMOVE|win32con.SWP_NOSIZE|win32con.SWP_NOACTIVATE)
gs=[(h,t) for h,t in windows() if t.startswith("Gaea -")]
if not gs:raise RuntimeError("Gaea GUI not launched")
hwnd,name=gs[0]
win32gui.ShowWindow(hwnd,win32con.SW_RESTORE)
try:win32gui.SetForegroundWindow(hwnd)
except Exception as e:print("FOREGROUND_RETRY",repr(e),flush=True)
time.sleep(.75)
fg=win32gui.GetWindowText(win32gui.GetForegroundWindow())
print("NOWORLD_GAEA_FOREGROUND="+json.dumps({"expected":name,"actual":fg}),flush=True)
# Verify foreground before using automated accelerator. No blind clicks.
if fg not in (name, "Gaea.Viewport"): raise RuntimeError("Unexpected foreground window")
user32=ctypes.windll.user32
def down(k):user32.keybd_event(k,0,0,0)
def up(k):user32.keybd_event(k,0,2,0)
down(0x11);down(0x4F);time.sleep(.11);up(0x4F);up(0x11)
time.sleep(1.6)
chk=subprocess.run([str(u),"hasfiledialog"],capture_output=True,text=True)
status=chk.stdout.strip()
print("NOWORLD_GAEA_FILE_OPEN="+status,flush=True)
if "filedialog" not in status or status=="nofiledialog":
 # Try MenuItem via UI automation; shortcut can be disabled.
 r=subprocess.run([str(u),"invoke","File"],capture_output=True,text=True)
 time.sleep(.4)
 print("NOWORLD_GAEA_FILEMENU="+r.stdout.strip(),flush=True)
 # Keyboard Enter while focus on File menu triggers dropdown, then inspect.
 down(0x0D);time.sleep(.08);up(0x0D)
 time.sleep(.6)
 dump=subprocess.run([str(u),"dump"],capture_output=True,text=True)
 (out/"NW_Gaea_UI_MenuState.txt").write_text(dump.stdout[-18000:],encoding="utf-8")
 chk=subprocess.run([str(u),"hasfiledialog"],capture_output=True,text=True)
 status=chk.stdout.strip()
 print("NOWORLD_GAEA_FILE_OPEN_SECOND="+status,flush=True)
if status=="filedialog":
 setp=subprocess.run([str(u),"openpath",str(project)],capture_output=True,text=True)
 time.sleep(2)
 title=subprocess.run([str(u),"title"],capture_output=True,text=True).stdout.strip()
 print("NOWORLD_GAEA_PROJECT_LOADED="+json.dumps({"uia":setp.stdout.strip(),"title":title}),flush=True)
 sys.exit(0 if project.name in title else 2)
sys.exit(3)
