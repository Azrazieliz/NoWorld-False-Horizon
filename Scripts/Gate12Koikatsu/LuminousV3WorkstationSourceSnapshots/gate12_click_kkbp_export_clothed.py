"""Click visually identified Luminous CLOTHED real Party KKBP button; no output claims."""
import ctypes,os,sys,time,pathlib,datetime,json
from PIL import ImageGrab
GAME=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party")
OUT=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011")
export=GAME/"Export_PMX"
before={p.name for p in export.iterdir() if p.is_dir()}
u=ctypes.windll.user32
previous=u.GetForegroundWindow()
h=u.FindWindowW(None,"Koikatsu Party")
assert h>0
u.ShowWindow(h,9)
if u.GetForegroundWindow()!=h:
 u.keybd_event(0x12,0,0,0);time.sleep(.08)
 u.SetForegroundWindow(h);time.sleep(.3)
 u.keybd_event(0x12,0,2,0)
assert u.GetForegroundWindow()==h
# Confirm previous frame manually: approved CLOTHED face/black dress visible,
# outfit top School Uniform (coordinate 0), KKBP "Export Model" button.
start=datetime.datetime.now(datetime.timezone.utc)
u.SetCursorPos(891,52)
u.mouse_event(2,0,0,0,0);time.sleep(.11);u.mouse_event(4,0,0,0,0)
time.sleep(5.0)
img=ImageGrab.grab();img.thumbnail((1180,800))
shot=OUT/"Party_CLOTHED_after_KKBP_export_click.jpg"
img.convert("RGB").save(shot,quality=75,optimize=True)
after={p.name for p in export.iterdir() if p.is_dir()}
receipt={"status":"EXPORT_BUTTON_PRESSED_NOT_YET_VERIFIED",
 "state":"CLOTHED","utc":start.isoformat(),
 "preexisting_dirs":sorted(before),"new_export_dirs":sorted(after-before),
 "screenshot":str(shot)}
(OUT/"CLOTHED_KKBP_CLICK_RECEIPT.json").write_text(json.dumps(receipt,indent=2)+"\n")
print(json.dumps(receipt,indent=2))
if previous and previous!=h:u.SetForegroundWindow(previous)
