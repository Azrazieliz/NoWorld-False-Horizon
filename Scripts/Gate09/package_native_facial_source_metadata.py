"""Package only Gate09's new native facial QA source, never mod payloads."""
import pathlib,hashlib,json,zipfile
root=pathlib.Path(r'D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019')
protected=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
gate12=pathlib.Path(r'D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010')
output=protected/'GATE09_NATIVE_FACE_AND_ADULT_SOURCE_METADATA_QA_20261019.zip'
if output.exists():raise FileExistsError(output)
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for block in iter(lambda:f.read(1048576),b''):h.update(block)
 return h.hexdigest()
uassets=sorted((root/'Content/Experimental/LuminousValentineV3/CLOTHED/Parts/Mesh003Prim000').rglob('*.uasset'))
uassets+=sorted((root/'Content/Gate09_QA_20261019/AuthenticFaceSource').rglob('*.uasset'))
assert len(uassets)==4,(len(uassets),uassets)
for f in uassets[:3]:
 original=gate12/'Content'/f.relative_to(root/'Content')
 assert original.exists() and sha(original)==sha(f)
meta=[
 root/'G09_AnimQA.uproject',
 root/'Saved/Gate09QA/unreal_04_authentic_source_face_morphs.json',
 root/'Saved/Gate09QA/unreal_05_native_facial_cold.json',
 protected/'G09_InstalledAdultNativeAnimation_MetadataOnly_20261019.json',
 protected/'G09_AdultNativeRigBindingCompatibility_20261019.json']
for f in meta:assert f.is_file(),f
config=sorted(x for x in (root/'Config').rglob('*') if x.is_file())
evidence={'schema':'G09_NATIVE_FACE_AND_ADULT_METADATA_PROOF_V1',
  'source_game_unmodified':True,'explicit_scene_imported':False,
  'source_payloads_included':False,
  'assets':[{'path':str(f.relative_to(root)),'sha256':sha(f),'bytes':f.stat().st_size} for f in uassets],
  'metadata':[{'path':f.name,'sha256':sha(f)} for f in meta]}
with zipfile.ZipFile(output,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=7) as z:
 for f in uassets+meta+config:
  name=(f.relative_to(root) if f.is_relative_to(root) else pathlib.Path('SourceMetadata')/f.name)
  z.write(f,arcname=str(name).replace('\\','/'))
 z.writestr('Gate09_Proof_Manifest.json',json.dumps(evidence,indent=2))
with zipfile.ZipFile(output,'r') as z:
 assert z.testzip() is None
 assert len([x for x in z.namelist() if x.endswith('.uasset')])==4
print('SOURCE_QA_PACKAGE',str(output),'BYTES',output.stat().st_size,
  'SHA256',sha(output),'NATIVE_UASSETS',len(uassets),flush=True)
