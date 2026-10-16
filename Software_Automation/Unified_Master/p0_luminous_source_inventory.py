"""P0 read-only source lock and saved Koikatsu card geometry/state fingerprint."""
import sys,pathlib,hashlib,json,os,collections
src=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\base_states')
steam=pathlib.Path(r'D:\Tools\Steam\steamapps\common\Koikatsu Party\UserData\chara\female')
master=pathlib.Path(r'D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Unified_Master')
sys.path.insert(0,str(master))
from native_kk_card_diff_v1 import load
files={
'BASE_CLOTHED':src/'LUMINOUS_BASE_CLOTHED_KK.png',
'BASE_UNDERWEAR':src/'LUMINOUS_BASE_UNDERWEAR_KK.png',
'BASE_NUDE':src/'LUMINOUS_BASE_NUDE_KK.png',
'ALTER_CANDIDATE_224249541':steam/'Koikatu_F_20261013224249541_Valentine Luminous.png',
'ALTER_CANDIDATE_224655353':steam/'Koikatu_F_20261013224655353_Valentine Luminous.png',
'ALTER_CANDIDATE_225235666':steam/'Koikatu_F_20261013225235666_Valentine Luminous.png'
}
res={}
for tag,path in files.items():
 row={'path':str(path),'exists':path.is_file()}
 if path.is_file():
  data=path.read_bytes();row.update({'sha256':hashlib.sha256(data).hexdigest(),'byte_size':len(data),'mtime_ns':path.stat().st_mtime_ns})
  try:
   parsed=load(path);row['parse']='PASS';row['coordinate_index']=int(parsed['Status']['coordinateType'])
   row['hair_ids']=[x['id'] for x in parsed['hair']['parts']]
   row['morph_face_count']=len(parsed['face']['shapeValueFace'])
   row['morph_body_count']=len(parsed['body']['shapeValueBody'])
   row['active_clothes']=[x['id'] for x in parsed['outfits'][row['coordinate_index']]['clothes']['parts']]
   row['seven_coordinates_clothes']=[[x['id'] for x in y['clothes']['parts']] for y in parsed['outfits']]
   row['seven_coordinates_accessory_count']=[sum(1 for x in y['accessory']['parts'] if x['id']) for y in parsed['outfits']]
   row['extension_keys']=list(parsed['KKEx'].keys())
  except Exception as e:row['parse']='FAIL';row['error']=repr(e)
 res[tag]=row
out=pathlib.Path(r'D:\UnrealProjects\NW_SoftwareMaster_P0_20261016')
out.mkdir(parents=True,exist_ok=True)
path=out/'SOURCE_IMMUTABLE_INPUT_MANIFEST.json'
path.write_text(json.dumps(res,indent=2))
for tag,r in res.items():print('NATIVE_SOURCE',tag,r.get('parse'),'sha',r.get('sha256','NA')[:22],'active',r.get('active_clothes'),'accessories',r.get('seven_coordinates_accessory_count'),flush=True)
print('MANIFEST',path,flush=True)
