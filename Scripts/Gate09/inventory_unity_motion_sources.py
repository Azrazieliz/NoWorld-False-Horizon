"""Gate 09 read-only authentic Koikatsu Unity animation availability probe."""
import pathlib,sys,json,collections,hashlib,zipfile,re
base=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty')
sys.path.insert(0,str(base/'pipeline/vendor_unitypy'))
import UnityPy
game=pathlib.Path(r'D:\Tools\Steam\steamapps\common\Koikatsu Party')
out=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
bundles=sorted((game/'abdata/action/animator').glob('*.unity3d'))+sorted((game/'abdata/studio/anime').glob('*.unity3d'))
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for c in iter(lambda:f.read(1024*1024),b''):h.update(c)
 return h.hexdigest()
records=[]
for p in bundles:
 env=UnityPy.load(str(p));records_by_bundle=[]
 for obj in env.objects:
  if obj.type.name!='AnimationClip':continue
  c=obj.read();name=str(c.m_Name)
  kind=('idle' if re.search(r'tachi|stand|idle|pose|suwari',name,re.I) else 
        'swim' if re.search(r'suiei|swim',name,re.I) else
        'dance' if re.search(r'dance',name,re.I) else
        'movement_candidate' if re.search(r'run|walk|turn|jump|fly|climb|action|dash',name,re.I) else
        'other')
  bb=getattr(c,'m_ClipBindingConstant',None)
  bindings=list(getattr(bb,'genericBindings',[])) if bb else []
  m=getattr(c,'m_MuscleClip',None)
  duration=(float(m.m_StopTime)-float(m.m_StartTime)) if m else None
  records_by_bundle.append({'name':name,'path_id':int(obj.path_id),'category_hint':kind,
      'fps':float(getattr(c,'m_SampleRate',0)), 'duration_seconds':duration,
      'binding_count':len(bindings),
      'serialized_rotation_curves':len(getattr(c,'m_RotationCurves',[]) or []),
      'serialized_position_curves':len(getattr(c,'m_PositionCurves',[]) or []),
      'serialized_float_curves':len(getattr(c,'m_FloatCurves',[]) or [])})
 records.append({'path':str(p),'sha256':sha(p),'bytes':p.stat().st_size,
   'count':len(records_by_bundle),'categories':dict(collections.Counter(x['category_hint'] for x in records_by_bundle)),
   'clips':records_by_bundle})
 print('BUNDLE',p.name,len(records_by_bundle),dict(collections.Counter(x['category_hint'] for x in records_by_bundle)),flush=True)
catalog={'schema':'G09_INSTALLED_UNITY_CLIP_INVENTORY_V2','status':'SOURCE_INSPECTED_NOT_IMPORTED_TO_UNREAL','bundles':records,
 'clip_count':sum(r['count'] for r in records)}
dest=out/'G09_Installed_Unity_Animation_Inventory_20261019.json'
if dest.exists():raise FileExistsError(dest)
dest.write_text(json.dumps(catalog,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
print('TOTAL',catalog['clip_count'],'SHA',sha(dest),flush=True)
