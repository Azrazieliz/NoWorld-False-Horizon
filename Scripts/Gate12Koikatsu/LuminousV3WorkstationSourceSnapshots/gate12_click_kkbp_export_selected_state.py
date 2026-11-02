"""Click running Party KKBP on verified visual state. Never mislabel exported geometry."""
import argparse,ctypes,os,sys,time,pathlib,datetime,json,hashlib
from PIL import ImageGrab
arg=argparse.ArgumentParser()
arg.add_argument("--state",required=True,choices=("CLOTHED","UNDERWEAR","NUDE"))
args=arg.parse_args()
GAME=pathlib.Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party")
OUT=pathlib.Path(r"D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011")
screen=OUT/("KK_underwear_after_explicit_load.jpg" if args.state=="UNDERWEAR"
           else "NUDE_after_extended_loader.jpg" if args.state=="NUDE"
           else "Party_after_selected_character_load.jpg")
assert screen.exists(),"Missing verified viewport state screenshot"
with (OUT/"SEALED_INTAKE_RECEIPT.json").open(encoding="utf8") as f:
 sealed=json.load(f)
card=OUT/"SEALED_INPUTS"/(args.state+".png")
cardsha=hashlib.sha256(card.read_bytes()).hexdigest()
assert cardsha==sealed["three_states"][args.state]["sha256"]
exp=GAME/"Export_PMX"
before={p.name for p in exp.iterdir() if p.is_dir()}
u=ctypes.windll.user32
old=u.GetForegroundWindow();h=u.FindWindowW(None,"Koikatsu Party")
assert h>0
u.ShowWindow(h,9)
if old!=h:
 u.keybd_event(18,0,0,0);time.sleep(.1);u.SetForegroundWindow(h);time.sleep(.3);u.keybd_event(18,0,2,0)
assert u.GetForegroundWindow()==h,"No GUI focus"
start=datetime.datetime.now(datetime.timezone.utc)
u.SetCursorPos(891,52);u.mouse_event(2,0,0,0,0);time.sleep(.12);u.mouse_event(4,0,0,0,0)
time.sleep(5)
im=ImageGrab.grab();im.thumbnail((1180,800))
shot=OUT/("Party_"+args.state+"_after_export_click.jpg")
im.convert("RGB").save(shot,quality=72,optimize=True)
new=sorted({p.name for p in exp.iterdir() if p.is_dir()}-before)
receipt={"schema":"GATE12_GENUINE_KKBP_EXPORT_CLICK_1","state":args.state,
 "status":"REAL_PARTY_EXPORT_CLICKED_NOT_YET_FINISHED",
 "before_export_directories":sorted(before),"new_dirs":new,
 "source_card_sha256_reference":cardsha,
 "checked_visual_state_screenshot":str(screen),
 "screenshot":str(shot),"utc":start.isoformat()}
(OUT/("PARTY_"+args.state+"_EXPORT_CLICK.json")).write_text(json.dumps(receipt,indent=2)+"\n")
print(json.dumps(receipt,indent=2))
if old and old!=h:u.SetForegroundWindow(old)
