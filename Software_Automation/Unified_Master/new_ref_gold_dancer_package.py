from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import shutil,json,hashlib,zipfile
R=Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST06_WHITE_GOLD_DANCER')
S=Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014')
cap=S/'BepInEx'/'SM01_SANDBOX'/'captures'
src=S/'UserData'/'chara'/'female'/'SM01_AUTOTEST_T06_WHITE_GOLD_DANCER_NATIVE_20261017.png'
if not src.is_file():raise FileNotFoundError(src)
dst=R/'T06_WHITE_GOLD_DANCER_EDITABLE_NATIVE_KK_CARD.png'
if dst.exists():raise FileExistsError(dst)
shutil.copy2(src,dst)
views=['front','face','side','back']
sheet=Image.new('RGB',(1740,920),(250,249,247));pen=ImageDraw.Draw(sheet)
for i,n in enumerate(views):
 orig=cap/('SM01_DNC_REFIT_APPEARANCE_RENDER_20261017_'+n+'.png')
 im=Image.open(orig).convert('RGB')
 if n=='face':
  box=(35,235,740,960)
 else:
  box=(328,215,690,838)
 region=im.crop(box)
 region.thumbnail((420,818))
 x=i*435+(435-region.width)//2
 sheet.paste(region,(x,64))
 pen.text((i*435+20,23),n.upper(),fill=(20,20,20))
final=R/'T06_ACTUAL_NATIVE_KK_4_VIEW_QA.jpg';sheet.save(final,quality=93)
receipt={'schema':'NOWORLD_KOIKATSU_GOLD_DANCER_BENCHMARK_V1',
'status':'NATIVE_EDITABLE_CARD_SAVED_RELOADED_VISUAL_NOT_APPROVED',
'original_user_reference':'image shared in chat; not transferred to remote PC',
'original_steam_content_modified':False,
'character_card':str(dst),'sha256':hashlib.sha256(dst.read_bytes()).hexdigest(),
'qa_image':str(final),'quality_claim':'Partial resemblance only, no user acceptance',
'known_visual_mismatches':['arms-down pose','dancer skirt long/black side panels rather than flowing split sheer fabric','hair shape and length','veil fabric and translucency','face expression and clothing structure'],
'independent_native_reload_passed':True,'material_editor_hook_saved':True}
(R/'T06_RESULT_RECEIPT.json').write_text(json.dumps(receipt,indent=2))
z=R/'T06_WHITE_GOLD_DANCER_DISPOSABLE_QA.zip'
if z.exists():raise FileExistsError(z)
with zipfile.ZipFile(z,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=3) as arc:
 for f in [dst,final,R/'T06_RESULT_RECEIPT.json']:arc.write(f,f.name)
 print('PACKAGE',str(z),'MB',round(z.stat().st_size/1048576,2),flush=True)
print('REVIEW',final,flush=True)
