from pathlib import Path
from PIL import Image,ImageDraw
root=Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST06_WHITE_GOLD_DANCER')
captures=Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014\BepInEx\SM01_SANDBOX\captures')
names=['TAKANE_LONG','ANNE_LONG','MINAMI_LONG','IORI_LONG']
sheet=Image.new('RGB',(1280,720),'white');pen=ImageDraw.Draw(sheet)
for i,n in enumerate(names):
 nonce=(root/(n+'_nonce.txt')).read_text().strip()
 im=Image.open(captures/('SM01_'+nonce+'_front.png')).convert('RGB')
 cr=im.crop((340,175,685,830));cr.thumbnail((315,680));sheet.paste(cr,(i*320+2,32))
 pen.text((i*320+20,8),n,fill=(0,0,0))
out=root/'FOUR_LONG_HAIR_ASSET_TRIALS.jpg';sheet.save(out,quality=88);print(out)
