"""Derive a reusable neutral navigation rail from the approved Gacha's own
ornamental SVG raster, without copying fictional banner/character content.
Buttons and labels remain real native UMG and this is decoration only.
"""
from pathlib import Path
from PIL import Image
import hashlib
ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/"Saved/Gate12Readiness/Gate10_Figma_PixelMatched_V2/gacha_chrome_transparent.png"
DEST=ROOT/"SourceArt/Gate10/Figma/Gate10_SharedFiveNavChrome_Neutral.png"
APPROVED_GACHA_CHROME_SHA="32244160c86543c2a60a6ee51b81eb70a84fc6af6dbad2cc99f5e90b3df eede9".replace(" ","")
if hashlib.sha256(SOURCE.read_bytes()).hexdigest()!=APPROVED_GACHA_CHROME_SHA:
    raise RuntimeError("Original Figma source changed; refusing derivative")
im=Image.open(SOURCE).convert("RGBA")
if im.size != (864,1536):raise RuntimeError("Incorrect original screen geometry")
im=im.crop((0,1373,864,1536))
pixel=im.load()
for y in range(im.height):
    for x in range(im.width):
        red,green,blue,alpha=pixel[x,y]
        if not alpha:continue
        high=max(red,green,blue)
        if high-min(red,green,blue)>13 and (blue>red*1.1 or red>green*1.14):
            luminance=max(120,high)
            pixel[x,y]=(int(luminance*.87),int(luminance*.86),int(luminance*.94),alpha)
DEST.parent.mkdir(parents=True,exist_ok=True)
im.save(DEST,optimize=True)
print("GATE10_NEUTRAL_NAV_DERIVED",DEST,hashlib.sha256(DEST.read_bytes()).hexdigest())
