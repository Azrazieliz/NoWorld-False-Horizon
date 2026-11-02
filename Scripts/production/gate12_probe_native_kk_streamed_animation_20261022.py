"""Inspect official Unity packed StreamedClip frame records losslessly."""
import gzip,json,pathlib,struct,sys
root=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
idx=json.loads((root/"SourceAssets/Gate12/Animations20261021/ImportedGate09OriginalUnityCurves/GATE09_ALL_SOURCE_INTAKE_PROGRESS.json").read_text())
j=json.loads(gzip.open(idx["source_files"][0]["raw_curve_path"],"rt").read())
a=j["streamed"]["packed_u32"]
def floatof(n):return struct.unpack("<f",struct.pack("<I",n))[0]
print("CLIP",j["name"],"WORDS",len(a),"STREAM_CURVES",j["streamed"]["curve_count"],"DENSE_CURVES",j["dense"]["curve_count"])
o=0;f=0
while o<len(a):
 if o+2>len(a):break
 time=floatof(a[o]);n=a[o+1];o+=2
 if n>4000 or o+n*5>len(a):
  print("BREAK_INVALID",f,time,n,o,len(a));break
 keys=[]
 for i in range(n):
  ix=a[o];coeffs=tuple(floatof(k) for k in a[o+1:o+5]);o+=5
  if i<4:keys.append((ix,coeffs))
 if f<9:print("STREAM_FRAME",f,"T",time,"N",n,"FIRST",keys)
 f+=1
print("PARSE_END",o,len(a),"FRAMES",f)
print("UNITY_DENSE_BEGIN",j["dense"]["begin_time"],"FRAMES",j["dense"]["frame_count"])
