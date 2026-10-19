"""Source-level facial channel availability from original Luminous glTF and installed plugin."""
import pathlib,sys,hashlib,json,collections,re
sys.path.insert(0,str(pathlib.Path(__file__).parent))
from audit_luminous_rig import load
src=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011\BLENDER_FRESH\CLOTHED\LUMINOUS_V3_CLOTHED_skinned.glb')
plugin=pathlib.Path(r'D:\Tools\Steam\steamapps\common\Koikatsu Party\BepInEx\plugins\KK_SexFaces.dll')
raw,doc,blob=load(src)
faces=[];targets=0
for i,m in enumerate(doc.get('meshes',[])):
 names=m.get('extras',{}).get('targetNames',[])
 count=max([len(pr.get('targets',[])) for pr in m.get('primitives',[])] or [0])
 targets+=count
 if names or count:
  faces.append({'mesh_index':i,'name':m.get('name'),'target_count':count,'named_count':len(names),
      'facial_candidate_count':sum(bool(re.search('eye|brow|mouth|lip|cheek|tongue|face|blink|smile',n,re.I)) for n in names),
      'target_names':names[:160]})
report={'schema':'G09_LUMINOUS_FACE_CHANNEL_AVAILABILITY_V1',
'source_glb_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),
'KK_SexFaces_plugin_installed':plugin.is_file(),
'facial_plugin_sha256':hashlib.sha256(plugin.read_bytes()).hexdigest() if plugin.is_file() else None,
'meshes_with_shape_targets':len(faces),'source_morph_target_instances':targets,
'meshes':faces,'source_face_animation':'NOT_BAKED',
'unreal_morph_animation':'NOT_TESTED',
'face_mode_contract':'Separate named morph/eye/gaze curves from skeletal idles; never treat a plugin DLL as Unreal plugin',
'privacy_and_appearance':'No original wardrobe changes'}
dest=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016\G09_Facial_Source_Unreal_Contract_20261019.json')
if dest.exists():raise FileExistsError(dest)
dest.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
print('G09_FACIAL_AUDIT',{'plugin':report['KK_SexFaces_plugin_installed'],'meshes':len(faces),'targets':targets,
'sha256':hashlib.sha256(dest.read_bytes()).hexdigest()},flush=True)
