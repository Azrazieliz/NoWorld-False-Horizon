"""Private procedural 2D skin material enhancement, preserves original alpha."""
import argparse,hashlib,json,random,pathlib
from PIL import Image,ImageDraw,ImageFilter
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run(source,output):
 source=pathlib.Path(source).resolve();output=pathlib.Path(output).resolve()
 if not source.is_file() or output.exists():raise FileExistsError('Invalid source or existing output')
 im=Image.open(source).convert('RGBA'); w,h=im.size
 if (w,h)!=(2048,2048):raise ValueError('Expected actual native KK 2048 skin texture')
 rng=random.Random(21015)
 nebula=Image.new('RGBA',im.size,(0,0,0,0));draw=ImageDraw.Draw(nebula)
 for i in range(16):
  cx=int(w*(.15+rng.random()*.7));cy=int(h*(.15+rng.random()*.7))
  rad=rng.randrange(110,400)
  color=rng.choice([(82,38,135,15),(75,38,140,17),(35,65,150,12),(116,69,167,11)])
  draw.ellipse((cx-rad,cy-rad*.45,cx+rad,cy+rad*.45),fill=color)
 nebula=nebula.filter(ImageFilter.GaussianBlur(85))
 im=Image.alpha_composite(im,nebula)
 stars=Image.new('RGBA',im.size,(0,0,0,0));pen=ImageDraw.Draw(stars)
 for i in range(850):
  x=rng.randrange(w);y=rng.randrange(h)
  s=rng.choices([1,2,3],[.67,.29,.04])[0]
  a=rng.randrange(40,125)
  c=(178+rng.randrange(55),170+rng.randrange(65),230+rng.randrange(25),a)
  pen.ellipse((x,y,x+s,y+s),fill=c)
  if s==3 and i%3==0:
   pen.line((x-4,y+1,x+6,y+1),fill=(188,180,250,40))
   pen.line((x+1,y-4,x+1,y+6),fill=(188,180,250,40))
 im=Image.alpha_composite(im,stars)
 out_alpha=Image.open(source).convert('RGBA').getchannel('A')
 im.putalpha(out_alpha)
 output.parent.mkdir(parents=True,exist_ok=True)
 im.save(output,compress_level=6)
 report={'status':'EDITABLE_NATIVE_SKIN_TEXTURE_PREPARED','source_sha256':sha(source),
 'output_sha256':sha(output),'pixels':[w,h],'source_alpha_preserved':True,
 'seed':21015,'star_marks':850,'nebula_layers':16,'source':str(source),'output':str(output)}
 (output.parent/('RECEIPT_'+output.stem+'.json')).write_text(json.dumps(report,indent=2))
 print('NW_COSMIC_SKIN='+json.dumps(report))
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--source',required=True);ap.add_argument('--output',required=True)
 a=ap.parse_args();run(a.source,a.output)
