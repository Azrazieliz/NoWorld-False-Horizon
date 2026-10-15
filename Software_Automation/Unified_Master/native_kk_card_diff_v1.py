"""Read-only real Koikatsu native card comparison. No GUI, no character modifications.
General: python native_kk_card_diff_v1.py --baseline old.png --target new.png --out diff.json
Requires msgpack. Custom: face/body/hair; Coordinate: outfit+accessories each of 7 states.
"""
from pathlib import Path
import argparse,struct,msgpack,json,hashlib,math
def load(path):
 path=Path(path).resolve();b=path.read_bytes()
 end=b.find(b'\x00\x00\x00\x00IEND\xaeB\x60\x82')+12
 if end<12:raise ValueError('Not a supported PNG')
 i=end+4;n=b[i];i+=1+n;n=b[i];i+=1+n
 png_len=struct.unpack_from('<I',b,i)[0];i+=4+png_len
 header_len=struct.unpack_from('<I',b,i)[0];i+=4
 header=msgpack.unpackb(b[i:i+header_len],raw=False,strict_map_key=False);i+=header_len
 blocks={}
 for item in header['lstInfo']:
  name=item['name'];start=i+item['pos']+8;raw=b[start:start+item['size']]
  if name=='Custom':
   parts=[];pos=0
   for subname in ('face','body','hair'):
    length=struct.unpack_from('<I',raw,pos)[0];pos+=4
    parts.append(msgpack.unpackb(raw[pos:pos+length],raw=False,strict_map_key=False));pos+=length
   blocks.update(zip(('face','body','hair'),parts))
  else:blocks[name]=msgpack.unpackb(raw,raw=False,strict_map_key=False)
 coords=[]
 for raw in blocks['Coordinate']:
  pos=0;sections=[]
  for sub in ('clothes','accessory'):
   length=struct.unpack_from('<I',raw,pos)[0];pos+=4
   sections.append(msgpack.unpackb(raw[pos:pos+length],raw=False,strict_map_key=False));pos+=length
  coords.append(dict(zip(('clothes','accessory'),sections)))
 blocks['outfits']=coords
 blocks['filename']=path.name;blocks['sha256']=hashlib.sha256(b).hexdigest()
 return blocks
def clean(v):
 if isinstance(v,float):return round(v,4)
 if isinstance(v,bytes):return {'size':len(v),'sha256':hashlib.sha256(v).hexdigest()[:18]}
 if isinstance(v,(tuple,list)):return [clean(x) for x in v]
 if isinstance(v,dict):return {str(k):clean(x) for k,x in v.items()}
 return v
def mkdiff(a,b):
 old,new=load(a),load(b)
 result={'schema':'NW_KK_NATIVE_CARD_DIFF_V1','read_only':True,'baseline':old['filename'],'target':new['filename'],'baseline_sha256':old['sha256'],'target_sha256':new['sha256'],'face':{},'body':{},'hair':{},'coordinates':{},'extra_plugins':{}}
 for group,shape in [('face','shapeValueFace'),('body','shapeValueBody')]:
  xs,ys=old[group][shape],new[group][shape]
  result[group]['morphs']=[{'index':i,'before':round(float(x),4),'after':round(float(y),4),'delta':round(float(y)-float(x),4)} for i,(x,y) in enumerate(zip(xs,ys)) if abs(x-y)>1e-4]
  result[group]['changed_morphs']=len(result[group]['morphs'])
  fields=['bustWeight','bustSoftness','skinMainColor','skinSubColor','skinGlossPower'] if group=='body' else ['pupil','baseMakeup','headId','cheekGlossPower','eyelineColor','eyebrowId']
  result[group]['other_fields']={field:{'before':clean(old[group][field]),'after':clean(new[group][field])} for field in fields if field in old[group] and clean(old[group][field])!=clean(new[group][field])}
 result['hair']['parts']=[{'part':i,'changes':{key:{'before':clean(old['hair']['parts'][i][key]),'after':clean(new['hair']['parts'][i][key])} for key in old['hair']['parts'][i] if key!='ExtendedSaveData' and clean(old['hair']['parts'][i][key])!=clean(new['hair']['parts'][i][key])}} for i in range(min(len(old['hair']['parts']),len(new['hair']['parts'])))]
 result['hair']['kind']={'before':old['hair'].get('kind'),'after':new['hair'].get('kind')}
 for idx,(a0,b0) in enumerate(zip(old['outfits'],new['outfits'])):
  c_diff=[];acc_diff=[]
  for i,(x,y) in enumerate(zip(a0['clothes']['parts'],b0['clothes']['parts'])):
   relevant=('id','colorInfo','pattern','emblemeId')
   changed={field:{'before':clean(x.get(field)),'after':clean(y.get(field))} for field in relevant if field in x or field in y if clean(x.get(field))!=clean(y.get(field))}
   if changed:c_diff.append({'part':i,'changes':changed})
  for i,(x,y) in enumerate(zip(a0['accessory']['parts'],b0['accessory']['parts'])):
   fields=('type','id','parentKey','addMove','color')
   changed={field:{'before':clean(x.get(field)),'after':clean(y.get(field))} for field in fields if clean(x.get(field))!=clean(y.get(field))}
   if changed:
    acc_diff.append({'slot':i,'baseline':{'type':x.get('type'),'id':x.get('id'),'parent':x.get('parentKey')},'target':{'type':y.get('type'),'id':y.get('id'),'parent':y.get('parentKey')},'changes':changed})
  if c_diff or acc_diff:result['coordinates'][idx]={'clothes':c_diff,'accessories':acc_diff}
 for k in sorted(set(old['KKEx'])|set(new['KKEx'])):
  aext=clean(old['KKEx'].get(k));bext=clean(new['KKEx'].get(k))
  if aext!=bext:result['extra_plugins'][k]={'before':aext,'after':bext}
 result['summaries']={'face_morphs':result['face']['changed_morphs'],'body_morphs':result['body']['changed_morphs'],'hair_parts_changed':sum(bool(v['changes']) for v in result['hair']['parts']),'outfit_coordinates_changed':len(result['coordinates']),'current_clothes_changes':len(result['coordinates'].get(0,{}).get('clothes',[])),'current_accessory_changes':len(result['coordinates'].get(0,{}).get('accessories',[]))}
 return result
def main():
 p=argparse.ArgumentParser();p.add_argument('--baseline',required=True);p.add_argument('--target',required=True);p.add_argument('--out',required=True);v=p.parse_args()
 dst=Path(v.out).resolve()
 if dst.exists():raise FileExistsError(dst)
 r=mkdiff(v.baseline,v.target);dst.parent.mkdir(parents=True,exist_ok=True);dst.write_text(json.dumps(r,indent=2,ensure_ascii=False))
 print('DIFF_SAVED',dst)
 print('SUMMARY',json.dumps(r['summaries']))
 for part in r['hair']['parts']:
  if 'id' in part['changes']:print('HAIR_ID',part['part'],part['changes']['id'])
 for x in r['coordinates'].get(0,{}).get('clothes',[]):print('CLOTHING',x['part'],x['changes'].get('id',{}))
 for x in r['coordinates'].get(0,{}).get('accessories',[]):print('ACCESSORY',x['slot'],'BASE',x['baseline'],'TARGET',x['target'])
 for group in ('face','body'):
  print('TOP',group,sorted(r[group]['morphs'],key=lambda v:abs(v['delta']),reverse=True)[:12])
 print('MOD_PLUGIN_DIFFERENCES',list(r['extra_plugins']))
if __name__=='__main__':main()
