"""Gate09 technical-only provenance and channel schema for installed native adult-mode Unity clips.
No motion key data extraction, no animation export, no visual depiction.
"""
import pathlib,sys,hashlib,json,collections
base=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty')
sys.path.insert(0,str(base/'pipeline/vendor_unitypy'))
import UnityPy
native=pathlib.Path(r'D:\Tools\Steam\steamapps\common\Koikatsu Party\abdata\h\anim')
out=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
files=sorted(native.rglob('*.unity3d'),key=lambda x:(x.stat().st_size,x.name))
rows=[];total=0
for p in files[:25]:
 try:
  env=UnityPy.load(str(p));clips=[]
  for obj in env.objects:
   if obj.type.name!='AnimationClip':continue
   a=obj.read();bb=getattr(a,'m_ClipBindingConstant',None)
   bindings=list(getattr(bb,'genericBindings',[])) if bb else []
   m=getattr(a,'m_MuscleClip',None)
   t=(float(m.m_StopTime-m.m_StartTime)) if m else 0
   attrs=collections.Counter(str(getattr(x,'attribute',None)) for x in bindings)
   typ=collections.Counter(str(getattr(x,'typeID',None)) for x in bindings)
   clips.append({'fps':float(getattr(a,'m_SampleRate',0)),'seconds':round(t,4),'bindings':len(bindings),'attribute_ids':dict(attrs),'type_ids':dict(typ)})
  digest=hashlib.sha256(p.read_bytes()).hexdigest()
  rows.append({'path':str(p),'size':p.stat().st_size,'sha256':digest,'clip_count':len(clips),
       'clips':clips,'motion_values_exported':False})
  total+=len(clips)
  print('SOURCE_SCHEMA',p.name,'clips',len(clips),'bytes',p.stat().st_size,flush=True)
 except Exception as e:rows.append({'path':str(p),'error':str(e)[:220]})
manifest={'schema':'G09_ADULT_SOURCE_READONLY_COMPAT_V1','source_directory':str(native),'sampled_bundle_count':len(rows),
'available_bundle_count':len(files),'sampled_native_clip_count':total,'sampled_bundles':rows,
'actor_age_verified':False,'asset_rights_cleared':False,'native_interchange_status':'BLOCKED_ELIGIBILITY_AND_RIGHTS',
'animation_curve_values_exported':False,'sexual_scene_rendered':False,
'provenance_only':True}
f=out/'G09_InstalledAdultNativeAnimation_MetadataOnly_20261019.json'
if f.exists():raise FileExistsError(f)
f.write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf8')
print('MANIFEST',f,'BUNDLES',len(rows),'CLIPS',total,flush=True)
