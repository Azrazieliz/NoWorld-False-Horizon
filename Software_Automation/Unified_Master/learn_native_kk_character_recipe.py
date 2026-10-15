"""Extract a reusable, non-destructive Koikatsu construction recipe from any saved native character card.
The recipe includes full face/body geometry and both accessory correction vectors.
All operations are proposals until applied to a separate test character.
"""
import argparse,json,pathlib,hashlib
from native_kk_card_diff_v1 import load,clean
def extract(path):
 d=load(path);coordinate_index=int(d['Status']['coordinateType'])
 c=d['outfits'][coordinate_index]; accessories=[];operations=[]
 for i,part in enumerate(c['accessory']['parts']):
  if not part['id']:continue
  move=part['addMove'][2]
  params={'slot':i,'category':part['type'],'effective_id':part['id'],'parent':part['parentKey'],
          'colors':clean(part['color']),'pos':clean(move[0]),'rot':clean(move[1]),'scale':clean(move[2]),
          'pos2':clean(move[3]),'rot2':clean(move[4]),'scale2':clean(move[5])}
  accessories.append(params)
  operations.append({'op':'apply_accessory_native','slot':i,'category':part['type'],'effective_id':part['id'],'parent':part['parentKey']})
  for kind,defaults in (('pos',[0,0,0]),('rot',[0,0,0]),('scale',[1,1,1]),('pos2',[0,0,0]),('rot2',[0,0,0]),('scale2',[1,1,1])):
   vec=params[kind]
   if any(abs(x-y)>.0001 for x,y in zip(vec,defaults)):
    operations.append({'op':'set_accessory_vector','slot':i,'group':kind,'xyz':vec})
 hair=[]
 for i,item in enumerate(d['hair']['parts']):
  hair.append({'slot':i,'native_id':item['id'],'base_rgba':clean(item['baseColor']),
              'root_rgba':clean(item['startColor']),'tip_rgba':clean(item['endColor']),
              'length':clean(item['length']),'position':clean(item['pos']),
              'rotation':clean(item['rot']),'scale':clean(item['scl'])})
 return {'schema':'NW_KOIKATSU_CARD_LEARNED_RECIPE_V1','source':d['filename'],'source_sha256':d['sha256'],
   'coordinate_index':coordinate_index,
   'face_shape':clean(d['face']['shapeValueFace']),'body_shape':clean(d['body']['shapeValueBody']),
   'face_makeup':clean(d['face']['baseMakeup']),'irises':clean(d['face']['pupil']),
   'hair':hair,'hair_kind':d['hair']['kind'],
   'clothing_parts':clean(c['clothes']['parts']),
   'accessories':accessories,'native_accessory_operations':operations,
   'contains_kkabmx_data':'KKABMPlugin.ABMData' in d['KKEx'],
   'card_is_user_reference_not_an_autonomous_generation':True}
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--card',required=True);parser.add_argument('--out',required=True);x=parser.parse_args()
 out=pathlib.Path(x.out).resolve()
 if out.exists():raise FileExistsError(out)
 out.parent.mkdir(parents=True,exist_ok=True)
 r=extract(x.card);out.write_text(json.dumps(r,indent=2,ensure_ascii=False))
 print('LEARNED_RECIPE',out,'face',len(r['face_shape']),'body',len(r['body_shape']),'accessories',len(r['accessories']),'non_default_accessory_operations',len(r['native_accessory_operations']))
 for a in r['accessories']:print('ATTACHMENT',a['slot'],a['category'],a['effective_id'],a['parent'],'vectors',[(k,a[k]) for k in ('pos','rot','scale','scale2')])
if __name__=='__main__':main()
