"""Extract three immutable authentic Koikatsu native standing clips; UnityPy read-only source."""
import pathlib,sys,hashlib,gzip,json
root=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty')
sys.path.insert(0,str(root/'pipeline/vendor_unitypy'))
import UnityPy
source=pathlib.Path(r'D:\Tools\Steam\steamapps\common\Koikatsu Party\abdata\action\animator\14.unity3d')
destination=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016\koikatsu_idle_sources_20261019')
if destination.exists():raise FileExistsError('immutable archive already exists')
destination.mkdir(parents=True)
names=['f_tachi_00_18_L','f_tachi_00_19_L','f_tachi_00_02']
def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
before=digest(source);env=UnityPy.load(str(source));manifest=[]
for o in env.objects:
 if o.type.name!='AnimationClip':continue
 c=o.read()
 if c.m_Name not in names:continue
 data=c.m_MuscleClip.m_Clip.data
 dense=data.m_DenseClip;stream=data.m_StreamedClip;constant=data.m_ConstantClip
 def binding(v):return {'path_crc32':int(v.path),'attribute':int(v.attribute),'typeID':int(v.typeID),'customType':int(v.customType)}
 output={'schema':'G09_AUTHENTIC_NATIVE_UNITY_CLIP_V1','clip_name':c.m_Name,'clip_path_id':int(o.path_id),
  'source_bundle_sha256':before,'fps':float(c.m_SampleRate),
  'duration_seconds':float(c.m_MuscleClip.m_StopTime-c.m_MuscleClip.m_StartTime),
  'bindings':[binding(x) for x in c.m_ClipBindingConstant.genericBindings],
  'streamed':{'count':int(stream.curveCount),'packed_u32':[int(v) for v in stream.data]},
  'dense':{'count':int(dense.m_CurveCount),'frames':int(dense.m_FrameCount),
           'start':float(dense.m_BeginTime),'rate':float(dense.m_SampleRate),
           'samples':[float(x) for x in dense.m_SampleArray]},
  'constant':[float(x) for x in constant.data],
  'binding_paths_resolved_from_authentic_prefab':True,
  'target_rig_bake_approved':False}
 n=output['dense']['count']+output['streamed']['count']+len(output['constant'])
 channels=sum(3 if b['attribute'] in (1,3) else 4 if b['attribute']==2 else 1 for b in output['bindings'])
 if n!=channels:raise RuntimeError('curve data component count not matching bindings')
 dest=destination/(c.m_Name+'.unitycurves.json.gz')
 with gzip.open(dest,'wt',encoding='utf-8',compresslevel=6) as f:json.dump(output,f,separators=(',',':'))
 with gzip.open(dest,'rt',encoding='utf-8') as f:assert json.load(f)['clip_name']==c.m_Name
 manifest.append({'name':c.m_Name,'file':dest.name,'sha256':digest(dest),'bytes':dest.stat().st_size,
  'fps':output['fps'],'duration_seconds':output['duration_seconds'],
  'bindings':len(output['bindings']),'float_components':n})
if len(manifest)!=3:raise RuntimeError('three unique native idles not found')
if digest(source)!=before:raise RuntimeError('installed source modified')
manifest.sort(key=lambda x:names.index(x['name']))
receipt={'status':'THREE_AUTHENTIC_KOIKATSU_SOURCE_CLIPS_EXTRACTED_NOT_BAKED','source_sha256':before,
'source':str(source),'clips':manifest,'face_expression_curves_from_source':'UNVERIFIED','unreal_native_animation':'NOT_CREATED','artistic_acceptance':False}
(destination/'MANIFEST.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
print('THREE_EXTRACTED='+json.dumps(receipt,ensure_ascii=False))
