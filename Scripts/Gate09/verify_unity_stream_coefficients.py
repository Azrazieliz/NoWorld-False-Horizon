"""Analyze Unity streamed cubic coefficients; no modifications."""
import gzip,json,struct,pathlib
folder=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016\koikatsu_idle_sources_20261019')
for p in folder.glob('*.gz'):
 with gzip.open(p,'rt',encoding='utf-8') as f:d=json.load(f)
 a=d['streamed']['packed_u32'];i=0;frames=[];maxindex=0
 while i<len(a):
  t=struct.unpack('<f',struct.pack('<I',a[i]))[0];n=a[i+1];i+=2
  if n>500:raise RuntimeError(('bad frame header',t,n,i))
  keys=[]
  for k in range(n):
   idx=a[i];coeff=struct.unpack('<ffff',struct.pack('<IIII',*a[i+1:i+5]));i+=5
   maxindex=max(maxindex,idx);keys.append((idx,coeff))
  frames.append((t,keys))
 print('NATIVE',p.name,'streamframes',len(frames),'t0',frames[0][0],'t1',frames[1][0] if len(frames)>1 else None,
    'tlast',frames[-1][0],'samples first',frames[0][1][:3],
    'dense',d['dense']['count'],'constant',len(d['constant']),'max index',maxindex)
 assert i==len(a)
