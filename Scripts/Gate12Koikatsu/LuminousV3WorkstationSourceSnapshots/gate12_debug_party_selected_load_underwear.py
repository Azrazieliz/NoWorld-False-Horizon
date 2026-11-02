import ctypes,time,pathlib,hashlib
from PIL import ImageGrab
u=ctypes.windll.user32;old=u.GetForegroundWindow()
h=u.FindWindowW(None,"Koikatsu Party")
assert h>0
u.ShowWindow(h,9)
if old!=h:
 u.keybd_event(18,0,0,0);time.sleep(.1);u.SetForegroundWindow(h);time.sleep(.4);u.keybd_event(18,0,2,0)
assert u.GetForegroundWindow()==h
out=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011")
def screenshot(state):
 im=ImageGrab.grab()
 # area x780-1100, y160-730 contains character upper body
 crop=im.crop((760,185,1080,725))
 print(state,"CHAR_AREA_SHA256",hashlib.sha256(crop.tobytes()).hexdigest()[:20])
 im.thumbnail((1180,800));im.convert("RGB").save(out/(state+".jpg"),quality=76,optimize=True)
def click(x,y):
 u.SetCursorPos(x,y);time.sleep(.12);u.mouse_event(2,0,0,0,0);time.sleep(.12);u.mouse_event(4,0,0,0,0)
screenshot("KK_before_underwear_force")
# highlight first saved UNDERWEAR thumbnail
click(245,239);time.sleep(1);screenshot("KK_underwear_selected_before_load")
# explicitly click center inside bottom Load button
click(420,759);time.sleep(5);screenshot("KK_underwear_after_explicit_load")
print("STATE_DEBUG_COMPLETE")
if old and old!=h:u.SetForegroundWindow(old)
