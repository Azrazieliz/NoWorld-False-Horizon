"""Compare actual source and GLB-embedded eyeline alpha distributions, full RGBA."""
import pathlib,struct,json,io,hashlib
import numpy as np
from PIL import Image
ROOT=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011')
OUT=pathlib.Path(r'D:\UnrealProjects\NW_SoftwareMaster_P0_20261016')
report={}
for state in ('CLOTHED','UNDERWEAR','NUDE'):
 f=ROOT/'BLENDER_FRESH'/state/f'LUMINOUS_V3_{state}_skinned.glb'
 b=f.read_bytes();n=struct.unpack_from('<I',b,12)[0];j=json.loads(b[20:20+n])
 pos=20+n;pos+=(4-pos%4)%4
 length,magic=struct.unpack_from('<I4s',b,pos);assert magic==b'BIN\x00';bp=pos+8
 r={}
 for m in j['materials']:
  name=m.get('name','')
  if not any(k in name.lower() for k in ('eyeline','hitomi','sirome','face')):continue
  tid=m.get('pbrMetallicRoughness',{}).get('baseColorTexture',{}).get('index')
  if tid is None:continue
  im=j['images'][j['textures'][tid]['source']]
  if 'bufferView' not in im:continue
  v=j['bufferViews'][im['bufferView']]
  payload=b[bp+v.get('byteOffset',0):bp+v.get('byteOffset',0)+v['byteLength']]
  try:img=Image.open(io.BytesIO(payload)).convert('RGBA')
  except Exception as e:continue
  a=np.asarray(img.getchannel('A'))
  r[name]={'embedded_image':im.get('name'),'format':im.get('mimeType'),'embedded_sha256':hashlib.sha256(payload).hexdigest(),
            'embedded_alpha_min':int(a.min()),'embedded_alpha_max':int(a.max()),
            'partial_alpha_fraction':round(float(((a>0)&(a<255)).mean()),6),
            'zero_alpha_fraction':round(float((a==0).mean()),6),'mode':m.get('alphaMode','OPAQUE')}
 report[state]=r
 print('EMBEDDED_ALPHA',state,[(k,z['mode'],z['partial_alpha_fraction'],z['embedded_alpha_min'],z['embedded_alpha_max']) for k,z in r.items() if 'eyeline' in k.lower()],flush=True)
out=OUT/'P0_NATIVE_GLB_EYELINE_EMBEDDED_COMPARISON.json';out.write_text(json.dumps(report,indent=2))
print('SAVED',out)
