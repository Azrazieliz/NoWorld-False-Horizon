"""Visual card thumbnail verified against three primary user-saved card previews.

Game GUI 1366x768, char list first row first three:
 UNDERWEAR x244 y243; NUDE x346 y243; CLOTHED x440 y243.
No source card is overwritten. Does NOT auto-export.
"""
import argparse,ctypes,time,pathlib,json
from PIL import ImageGrab
parser=argparse.ArgumentParser()
parser.add_argument("--state",required=True,choices=["CLOTHED","UNDERWEAR","NUDE"])
args=parser.parse_args()
xy={"UNDERWEAR":(245,240),"NUDE":(347,240),"CLOTHED":(445,240)}
u=ctypes.windll.user32
before=u.GetForegroundWindow()
h=u.FindWindowW(None,"Koikatsu Party")
assert h>0
u.ShowWindow(h,9)
if before!=h:
 u.keybd_event(0x12,0,0,0);time.sleep(.09)
 u.SetForegroundWindow(h);time.sleep(.32);u.keybd_event(0x12,0,2,0)
assert u.GetForegroundWindow()==h,"Party not foreground; no blind click"
def click(x,y):
 u.SetCursorPos(x,y);u.mouse_event(2,0,0,0,0);time.sleep(.12);u.mouse_event(4,0,0,0,0)
click(*xy[args.state]);time.sleep(1.0)
# Explicitly apply selected character using current Char List LOAD control.
click(420,759);time.sleep(5.0)
out=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011")
file=out/("Party_selected_"+args.state+"_before_export.jpg")
img=ImageGrab.grab();img.thumbnail((1180,800));img.convert("RGB").save(file,quality=73,optimize=True)
print(json.dumps({"status":"VISUAL_STATE_SELECTED_SCREENSHOT_REVIEW_REQUIRED",
 "state":args.state,"load_button_clicked":True,"preview":str(file),
 "source_card_sha256":json.loads((out/"SEALED_INTAKE_RECEIPT.json").read_text(encoding="utf8"))["three_states"][args.state]["sha256"]},indent=2))
if before and before!=h:u.SetForegroundWindow(before)
