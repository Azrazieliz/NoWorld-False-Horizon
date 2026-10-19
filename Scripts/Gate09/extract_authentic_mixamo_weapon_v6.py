"""Gate09 isolated authentic Mixamo-for-Koikatsu weapon-source extraction.
Original mod stays installed, original approved Luminous character untouched.
Saves decoded transform samples privately; no public third-party binaries.
"""
import pathlib,sys,hashlib,zipfile,json,gzip,struct,collections
import numpy as np
ROOT=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
sys.path.insert(0,r'D:\Tools\GameAssetPipeline\KoikatsuParty\pipeline\vendor_unitypy')
import UnityPy
src=next(pathlib.Path(r'D:\Tools\Steam\steamapps\common\Koikatsu Party\mods\Sideloader Modpack - Animations').rglob('*Mixamo animations*'))
inner='abdata/studio/anime/mixamo_weapon_part1.unity3d'
clips_wanted=['Slash Advance','Great Sword Slash 2','Thrust Slash']
base=json.loads((ROOT/'G09_Unity_Native_Transform_Hash_Map_20261019.json').read_text(encoding='utf8'))['resolved']
mapping={int(k):sorted({r['path'] for r in v})[0] for k,v in base.items() if len({r['path'] for r in v})==1}
restbasis=json.loads((ROOT/'G09_Unity_Native_Prefab_RestBasis_20261019.json').read_text(encoding='utf8'))['joints']
prefab=pathlib.Path(r'D:\Tools\Steam\steamapps\common\Koikatsu Party\abdata\chara\oo_base.unity3d')
env0=UnityPy.load(str(prefab));native={}
for ob in env0.objects:
 if ob.type.name!='Transform':continue
 try:
  x=ob.read();q=x.m_LocalRotation
  native[int(ob.path_id)]=[float(q.x),float(q.y),float(q.z),float(q.w)]
 except Exception:pass
rest={}
for h,recs in base.items():
 rots=[native[r['node']] for r in recs if r['node'] in native]
 if rots and int(h) in mapping:rest[mapping[int(h)]]=np.median(np.array(rots),axis=0).tolist()
with zipfile.ZipFile(src) as z:blob=z.read(inner)
bundle_sha=hashlib.sha256(blob).hexdigest()
unity=UnityPy.load(blob)
outdir=ROOT/'G09_DharmaMixamoSource_v6_20261019'
if outdir.exists():raise FileExistsError(outdir)
outdir.mkdir()
entries=[]
for obj in unity.objects:
 if obj.type.name!='AnimationClip':continue
 c=obj.read()
 if c.m_Name not in clips_wanted:continue
 cdata=c.m_MuscleClip.m_Clip.data
 streams=cdata.m_StreamedClip;dense=cdata.m_DenseClip;constant=cdata.m_ConstantClip
 binds=[{'path':int(x.path),'attribute':int(x.attribute)} for x in c.m_ClipBindingConstant.genericBindings]
 sdata=[int(x) for x in streams.data]
 expected=sum(3 if x['attribute'] in (1,3) else 4 if x['attribute']==2 else 1 for x in binds)
 scount=int(streams.curveCount);dcount=int(dense.m_CurveCount);const=np.array(constant.data,dtype=float)
 assert expected==scount+dcount+len(const)
 events=[];i=0
 while i<len(sdata):
  t=struct.unpack('<f',struct.pack('<I',sdata[i]))[0];n=sdata[i+1];i+=2;keys={}
  for _ in range(n):
   j=sdata[i];k=struct.unpack('<ffff',struct.pack('<IIII',*sdata[i+1:i+5]));i+=5
   keys[j]=float(k[3])
  events.append((t,keys))
 assert i==len(sdata)
 frames=[];cur={};j=0
 ds=np.array(dense.m_SampleArray,dtype=float).reshape((int(dense.m_FrameCount),dcount))
 fps=30
 num=round(float(c.m_MuscleClip.m_StopTime-c.m_MuscleClip.m_StartTime)*fps)+1
 values=np.zeros(expected,dtype=float);values[scount+dcount:]=const
 real_unique={int(x['path']) for x in binds}
 mapped_unique=real_unique & set(mapping)
 assert len(mapped_unique)>=70,(c.m_Name,len(mapped_unique))
 for f in range(num):
  t=f/fps
  while j<len(events) and events[j][0]<=t+0.000001:
   cur.update(events[j][1]);j+=1
  for k,v in cur.items():
   if k<scount:values[k]=v
  idx=min(int(dense.m_FrameCount)-1,max(0,round((t-float(dense.m_BeginTime))*float(dense.m_SampleRate))))
  values[scount:scount+dcount]=ds[idx]
  row={};off=0
  for b in binds:
   n={1:3,2:4,3:3}.get(b['attribute'],1);v=values[off:off+n].tolist();off+=n
   path=mapping.get(b['path'])
   if path is not None:row.setdefault(path,{})[{1:'local_position',2:'local_rotation_xyzw',3:'local_scale'}[b['attribute']]]=v
  assert off==len(values)
  frames.append(row)
 used=set().union(*(set(x.keys()) for x in frames))
 qt=[np.linalg.norm(b['local_rotation_xyzw']) for x in frames for b in x.values() if 'local_rotation_xyzw' in b]
 assert all(0.98<=x<=1.02 for x in qt)
 d={'schema':'G09_AUTHENTIC_KOIKATSU_COMPATIBLE_WEAPON_MOTION_SAMPLES_V1',
    'source_clip':c.m_Name,'source_mod_path':str(src),'bundle_sha256':bundle_sha,
    'sampled_fps':fps,'original_fps':float(c.m_SampleRate),'seconds':float(c.m_MuscleClip.m_StopTime-c.m_MuscleClip.m_StartTime),
    'source_unique_bindings':len(real_unique),'resolved_native_bindings':len(mapped_unique),
    'paths_mapped':len(used),'unresolved_source_paths':len(real_unique-mapped_unique),
    'native_prefab_rest_quaternions_xyzw':{p:rest[p] for p in used if p in rest},
    'frames':frames,'not_approved_combat_art':True,'source_character_modified':False}
 filename='G09_WeaponSource_'+c.m_Name.replace(' ','_')+'_30fps.json.gz';out=outdir/filename
 with gzip.open(out,'wt',encoding='utf8',compresslevel=6) as f:json.dump(d,f,separators=(',',':'))
 entries.append({'source_clip':c.m_Name,'source_file':filename,
  'sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'frames':num,'seconds':d['seconds'],
  'resolved_unique_native_paths':len(mapped_unique),'source_unique_paths':len(real_unique),
  'paths_mapped_to_known_native':len(used),'verified_normalized_quaternion':True})
 print('WEAPON_SOURCE',c.m_Name,'FRAMES',num,'KNOWN_PATHS',len(used),'DURATION',round(d['seconds'],2),flush=True)
assert len(entries)==3,entries
manifest={'schema':'G09_DHARMA_AUTHENTIC_WEAPON_SOURCE_V1','source_mod_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),
 'source_bundle_sha256':bundle_sha,'source_skeleton':'Mixamo-for-Koikatsu source, native path subset only',
 'clips':entries,'gate07_equipment_mutated':False,'gameplay_ready':False,
 'v6_mechanics_hit_times_ms':[180,390,650],'gate04_hit_scaling_bps':[4000,6000,9000],
 'artistic_quality_not_yet_validated':True}
(outdir/'MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf8')
print('WEAPON_SOURCE_EXTRACTION_COMPLETE',outdir,flush=True)
