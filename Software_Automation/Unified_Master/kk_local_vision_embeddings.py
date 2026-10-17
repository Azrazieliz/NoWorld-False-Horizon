#!/usr/bin/env python3
"""FREE LOCAL CPU MobileCLIP2 image retrieval for Koikatsu source/candidate screenshots.

Consumes locally stored reference art and Koikatsu native rendered images.
Runs ONNX Runtime OFFLINE on CPU. No credits, paid APIs, or upload to third parties.
Image embeddings are suggestive ranking features, NEVER automatic artistic approval.
"""
from __future__ import annotations
import argparse,hashlib,json,pathlib,sys,time
import numpy as np
from PIL import Image,ImageOps
from scipy.ndimage import gaussian_filter
BASE=pathlib.Path(r"D:\Tools\OfflineGameAI\models\KoikatsuVisualMobileCLIP2")
MODEL=BASE/"vision_model.onnx"
RUNTIME=BASE/"onnx_wheels"
CACHE=BASE/"image_embedding_cache"
MEAN=np.array([.48145466,.4578275,.40821073],dtype=np.float32).reshape(1,1,3)
STD=np.array([.26862954,.26130258,.27577711],dtype=np.float32).reshape(1,1,3)
def _digest(p):
 h=hashlib.sha256()
 with pathlib.Path(p).open('rb') as f:
  for buf in iter(lambda:f.read(1<<20),b''):h.update(buf)
 return h.hexdigest()
def _crop_subject(image):
 image=image.convert('RGB')
 a=np.asarray(image,dtype=np.uint8)
 # For white-source images only; avoid stripping arbitrary scene backgrounds.
 corner=np.concatenate((a[:22,:22].reshape(-1,3),a[-22:,-22:].reshape(-1,3)))
 if np.mean(np.abs(corner.astype(np.int16)-255))>40:return image
 dist=np.max(255-a,axis=2)
 foreground=dist>27
 yy,xx=np.nonzero(foreground)
 if len(xx)<100:return image
 x1=max(0,int(xx.min())-5);x2=min(image.width,int(xx.max())+6)
 y1=max(0,int(yy.min())-5);y2=min(image.height,int(yy.max())+6)
 if x2-x1<20 or y2-y1<20:return image
 return image.crop((x1,y1,x2,y2))
def _multiview(image):
 """Full, head+upper clothing, and lower clothing; avoid square center-crop loss."""
 im=_crop_subject(image)
 w,h=im.size
 pieces=[im,im.crop((0,0,w,max(16,int(h*.56)))),
           im.crop((0,min(h-16,int(h*.42)),w,h))]
 return pieces

def preprocess(piece):
 # Retain full silhouette with white margins, unlike destructive square center crop.
 square=ImageOps.contain(piece.convert('RGB'),(242,242),Image.Resampling.LANCZOS)
 bg=Image.new('RGB',(256,256),'white')
 bg.paste(square,((256-square.width)//2,(256-square.height)//2))
 x=np.asarray(bg,dtype=np.float32)/255.0
 return ((x-MEAN)/STD).transpose(2,0,1)

class LocalEncoder:
 def __init__(self,max_threads=4):
  if not MODEL.is_file():raise FileNotFoundError('Local ONNX model not installed; run kk_install_local_vision.py')
  if str(RUNTIME) not in sys.path:sys.path.insert(0,str(RUNTIME))
  import onnxruntime as ort
  opts=ort.SessionOptions()
  opts.intra_op_num_threads=max(1,min(8,int(max_threads)))
  opts.inter_op_num_threads=1
  self.session=ort.InferenceSession(str(MODEL),sess_options=opts,providers=['CPUExecutionProvider'])
  self.input=self.session.get_inputs()[0].name
  self.hash=_digest(MODEL)[:16]
  CACHE.mkdir(exist_ok=True,parents=True)
 def embed(self,path):
  source=pathlib.Path(path)
  if not source.is_file():raise FileNotFoundError(source)
  sha=_digest(source)
  cache=CACHE/(sha[:22]+'_'+self.hash+'_v1.npz')
  if cache.is_file():
   with np.load(cache,allow_pickle=False) as x:
    return x['embedding'].astype(np.float32),True
  pieces=_multiview(Image.open(source))
  arr=np.stack([preprocess(piece) for piece in pieces]).astype(np.float32)
  out=self.session.run(None,{self.input:arr})[0].astype(np.float32)
  if out.shape!=(3,512):raise RuntimeError('Wrong CLIP image feature dims')
  out=out/(np.linalg.norm(out,axis=1,keepdims=True)+1e-8)
  np.savez_compressed(cache,embedding=out)
  return out,False

def sim(a,b):
 if a.shape!=b.shape or a.shape!=(3,512):raise ValueError('Image representations incompatible')
 values=np.sum(a*b,axis=1)
 weights=np.array([.31,.43,.26],dtype=np.float32)
 return {"full":round(float(values[0]),5),"upper":round(float(values[1]),5),
    "lower":round(float(values[2]),5),
    "weighted_cosine":round(float(np.dot(weights,values)),5)}

def rank(reference,candidates,threads=4):
 if not candidates or len(candidates)>1000:raise ValueError('Expected 1..1000 candidates')
 start=time.perf_counter();encoder=LocalEncoder(threads)
 ref,hit=encoder.embed(reference)
 rows=[]
 for file in candidates:
  feat,cached=encoder.embed(file)
  rows.append({"candidate":str(file),"similarity":sim(ref,feat),
     "cached":cached,"rank_only_not_acceptance":True})
 rows.sort(key=lambda row:row["similarity"]["weighted_cosine"],reverse=True)
 return {"schema":"NW_LOCAL_CPU_CLIP_RETRIEVAL_V1","reference":str(reference),
  "source_model_sha256":encoder.hash,
  "images":rows,"elapsed_seconds":round(time.perf_counter()-start,3),
  "no_image_uploaded":True,"paid_calls":0,"fidelity_accepted":False,
  "warning":"CLIP similarity is a semantic proxy; it cannot determine 3D topology, rigging, exact face identity or apparel fidelity."}

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument("--reference",required=True)
 p.add_argument("--candidate",required=True,action='append')
 p.add_argument("--out",required=True)
 p.add_argument("--threads",type=int,default=4)
 a=p.parse_args()
 output=pathlib.Path(a.out)
 if output.exists():raise FileExistsError(output)
 data=rank(a.reference,a.candidate,a.threads)
 output.parent.mkdir(parents=True,exist_ok=True)
 output.write_text(json.dumps(data,indent=2),encoding='utf-8')
 print("LOCAL_CPU_VISION_RANK="+json.dumps({"model":data["source_model_sha256"],
     "seconds":data["elapsed_seconds"],
     "results":[(x["candidate"],x["similarity"]["weighted_cosine"]) for x in data["images"]]}))
if __name__=="__main__":main()
