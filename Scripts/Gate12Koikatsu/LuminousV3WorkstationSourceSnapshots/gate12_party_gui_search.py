import ctypes,time,sys,pathlib
from PIL import ImageGrab
u=ctypes.windll.user32
h=u.FindWindowW(None,"Koikatsu Party")
assert h>0 and u.GetForegroundWindow()==h,"Game not foreground; abort"
term=sys.argv[1] if len(sys.argv)>1 else "Luminous"
assert term.isascii()
# Full monitor 1366x768, Character Maker character-list Search ... x185-720 y151-173
u.SetCursorPos(310,153)
u.mouse_event(2,0,0,0,0);time.sleep(.1);u.mouse_event(4,0,0,0,0)
time.sleep(.15)
for ch in term:
 v=u.VkKeyScanW(ord(ch))
 vk=v&0xff; mod=(v>>8)&0xff
 if mod&1:u.keybd_event(0x10,0,0,0)
 u.keybd_event(vk,0,0,0);time.sleep(.07);u.keybd_event(vk,0,2,0)
 if mod&1:u.keybd_event(0x10,0,2,0)
time.sleep(.9)
target=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011")/"Party_filtered_Luminous.jpg"
img=ImageGrab.grab();img.thumbnail((1180,800));img.convert("RGB").save(target,quality=69,optimize=True)
print("GAME_SEARCH_TERM",term,"SCREEN",str(target))
