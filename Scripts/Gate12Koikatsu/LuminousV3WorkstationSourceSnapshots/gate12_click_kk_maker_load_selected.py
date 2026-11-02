"""Click currently visually confirmed Luminous thumbnail LOAD, preserve prior desktop focus."""
import ctypes,time,pathlib
from PIL import ImageGrab
u=ctypes.windll.user32
orig=u.GetForegroundWindow()
h=u.FindWindowW(None,"Koikatsu Party")
assert h>0
u.ShowWindow(h,9)
if u.GetForegroundWindow()!=h:
    u.keybd_event(0x12,0,0,0);time.sleep(.1)
    u.SetForegroundWindow(h);time.sleep(.3)
    u.keybd_event(0x12,0,2,0)
assert u.GetForegroundWindow()==h,"Not focused, no click"
# 1366x768 Character List overlay shown, bottom "Load" button at (416,756).
u.SetCursorPos(416,754)
u.mouse_event(2,0,0,0,0);time.sleep(.11);u.mouse_event(4,0,0,0,0)
time.sleep(2.7)
p=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011\Party_after_selected_character_load.jpg")
img=ImageGrab.grab();img.thumbnail((1180,800));img.convert("RGB").save(p,quality=73,optimize=True)
print("CHARACTER_LOAD_BUTTON_CLICKED",str(p),"PREVIOUS_FOREGROUND_HANDLE",orig)
if orig and orig!=h:u.SetForegroundWindow(orig)
