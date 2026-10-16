"""Read-only: exact native atlas alpha distributions and downstream GLB PBR alpha semantics."""
import pathlib,struct,json,hashlib,collections,math
import numpy as np
from PIL import Image
ROOT=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011')
OUT=pathlib.Path(r'D:\UnrealProjects\NW_SoftwareMaster_P0_20261016')
def source_alpha(p):
 try:
  im=Image.open(p).convert('RGBA')
  alpha=np.array(im.getchannel('A'),dtype=np.uint8)
  return {'file':str(p),'shape':im.size,'alpha_min':int(alpha.min()),'alpha_max':int(alpha.max()),'zero_fraction':round(float((alpha==0).mean()),6),
   'partial_fraction':round(float(((alpha>0)&(alpha<255)).mean()),6),'solid_fraction':round(float((alpha==255).mean()),6),
   'alpha_quantiles':[int(x) for x in np.quantile(alpha,[0,.01,.1,.25,.5,.75,.9,.99,1])]}
 except Exception as e:return {'file':str(p),'error':str(e)}
res={}
for state in ('CLOTHED','UNDERWEAR','NUDE'):
 folder=ROOT/'FRESH_PARTY_EXPORTS'/state
 tex=list(folder.glob('*eyeline*'))+list(folder.glob('*_sirome*'))+list(folder.glob('*_hitomi*'))
 file=ROOT/'BLENDER_FRESH'/state/f'LUMINOUS_V3_{state}_skinned.glb'
 raw=file.read_bytes();n=struct.unpack_from('<I',raw,12)[0];j=json.loads(raw[20:20+n])
 mtl=[]
 for i,m in enumerate(j['materials']):
  n=m.get('name','').lower()
  if any(s in n for s in ('eyeline','sirome','hitomi','face','eye')):
   texslot=m.get('pbrMetallicRoughness',{}).get('baseColorTexture',{}).get('index')
   imgname=(j['images'][j['textures'][texslot]['source']].get('name') if texslot is not None else None)
   mtl.append({'index':i,'name':m.get('name'),'alpha_mode':m.get('alphaMode','OPAQUE'),'alpha_cutoff':m.get('alphaCutoff'),
    'double_sided':m.get('doubleSided',False),'base_color_factor':m.get('pbrMetallicRoughness',{}).get('baseColorFactor'),'base_color_image':imgname})
 res[state]={'source_alpha': [source_alpha(p) for p in tex[:75]],'glb_eye_face_materials':mtl}
 print('STATE',state,'source_eye_atlases',len(tex),'GLB_eye_materials',len(mtl))
 for m in mtl:
  if 'eyeline' in m['name'].lower():print('EYELINE_MATERIAL',m['name'],m['alpha_mode'],m['base_color_image'])
 for x in res[state]['source_alpha']:
  if 'eyeline' in pathlib.Path(x['file']).name.lower():print('NATIVE_EYELINE_ALPHA',pathlib.Path(x['file']).name,x.get('alpha_min'),x.get('alpha_max'),x.get('partial_fraction'))
dest=OUT/'P0_NATIVE_GLB_EYELINE_ALPHA_AUDIT.json';dest.write_text(json.dumps(res,indent=2))
print('REPORT',dest)
