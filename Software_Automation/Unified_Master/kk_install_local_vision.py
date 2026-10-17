"""Download a free licensed CPU-only model + lightweight ONNX runtime under QA tools.
No paid API, cloud inference, or user images sent to any service.
"""
import requests,hashlib,pathlib,subprocess,sys,time,json
BASE=pathlib.Path(r"D:\Tools\OfflineGameAI\models\KoikatsuVisualMobileCLIP2")
BASE.mkdir(parents=True,exist_ok=True)
model=BASE/"vision_model.onnx"
source="https://huggingface.co/felixhrdyn/mobileclip2-s0-onnx/resolve/main/vision_model.onnx"
license_uri="https://huggingface.co/felixhrdyn/mobileclip2-s0-onnx"
def digest(path):
 h=hashlib.sha256()
 with path.open("rb") as f:
  for b in iter(lambda:f.read(1<<20),b""):h.update(b)
 return h.hexdigest()
if not model.is_file():
 temp=model.with_suffix('.partial')
 with requests.get(source,stream=True,timeout=(10,30)) as r:
  r.raise_for_status()
  with temp.open("wb") as out:
   for chunk in r.iter_content(1<<20):
    if chunk:out.write(chunk)
 if not 40_000_000<temp.stat().st_size<65_000_000:raise RuntimeError("Unexpected model weight byte count")
 temp.replace(model)
runtime=BASE/"onnx_wheels"
if not (runtime/"onnxruntime").is_dir():
 command=[sys.executable,"-m","pip","install","--no-deps","--target",str(runtime),
   "onnxruntime==1.23.2"]
 result=subprocess.run(command,text=True,capture_output=True,timeout=180)
 print("PIP_INSTALL",result.returncode,result.stdout[-850:],result.stderr[-500:],flush=True)
 if result.returncode:raise RuntimeError("ONNX CPU runtime unavailable")
sys.path.insert(0,str(runtime))
import onnxruntime
session=onnxruntime.InferenceSession(str(model),providers=["CPUExecutionProvider"])
meta={'schema':'NW_CPU_CLIP_LOCAL_MODEL_V1','provider':'CPUExecutionProvider',
 'model_file':str(model),'model_sha256':digest(model),'size_bytes':model.stat().st_size,
 'runtime_version':onnxruntime.__version__,
 'input_shapes':[(x.name,x.shape,x.type) for x in session.get_inputs()],
 'output_shapes':[(x.name,x.shape,x.type) for x in session.get_outputs()],
 'origin_uri':license_uri,'license':'MIT','paid_api_calls':0,'local_only':True,
 'visual_reconstruction_validated':False}
receipt=BASE/'PINNED_MODEL_AND_RUNTIME.json'
receipt.write_text(json.dumps(meta,indent=2))
print("LOCAL_CLIP_READY",json.dumps(meta),flush=True)
