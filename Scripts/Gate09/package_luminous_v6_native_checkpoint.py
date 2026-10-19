"""Private portable Gate09 native UE checkpoint: authentic sword motions + initial 16m field geometry.
QA archive contains no original third-party zipmod, no source-only Gate12 mutations.
"""
import pathlib,hashlib,json,zipfile,time
P=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
R=pathlib.Path(r'D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019')
G=pathlib.Path(r'D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010')
out=P/'GATE09_LUMINOUS_V6_REAL_COMBAT_SOURCES_UNREAL_CHECKPOINT_20261019.zip'
if out.exists():raise FileExistsError(out)
def hash(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for block in iter(lambda:f.read(1048576),b''):h.update(block)
 return h.hexdigest()
def receipt(name):return json.loads((R/'Saved/Gate09QA'/name).read_text(encoding='utf8'))
imp=receipt('unreal_06_dharma_native_sword_sources.json')
cold=receipt('unreal_07_dharma_native_cold_pose.json')
field=receipt('unreal_08_sovereign_geometry_import.json')
fieldcold=receipt('unreal_09_sovereign_geometry_cold_qa.json')
assert imp['status']=='PASS_THREE_AUTHENTIC_SWORD_SOURCE_NATIVE_UNREAL_ASSETS'
assert cold['status']=='PASS_NATIVE_SWORD_SOURCES_COLD_MOTION'
assert field['status']=='PASS_SOURCE_NATIVE_UE_FIELD_GEOMETRY_IMPORT_ONLY'
assert fieldcold['status']=='PASS_NATIVE_SOVEREIGN_GEOMETRY_COLD_RELOAD'
assert len(imp['assets'])==3 and len(field['native_effect_assets'])==4
assert all(x['track_count']==975 for x in imp['assets'])
assert all(x['tracks']==975 and x['motion_nonstatic'] for x in cold['records'])
assert len(fieldcold['materials'])==8
# Strict protection of existing four native UE animation files preserved since prior accepted proof archive
old=P/'GATE09_UNREAL583_NATIVE_FOUR_CLIPS_20261019.zip'
with zipfile.ZipFile(old) as z:
 oldfiles=[x for x in z.namelist() if x.startswith('Content/Gate09_QA_20261019') and x.endswith('.uasset')]
 assert len(oldfiles)==4,oldfiles
 for rel in oldfiles:
  present=R/rel
  assert present.exists() and hashlib.sha256(present.read_bytes()).digest()==hashlib.sha256(z.read(rel)).digest(),rel
reference=list((R/'Content/Experimental/LuminousValentineV3/CLOTHED/Parts/Mesh000Prim000').rglob('*.uasset'))
reference+=list((R/'Content/Experimental/LuminousValentineV3/CLOTHED/Parts/Mesh003Prim000').rglob('*.uasset'))
assert len(reference)==6
for f in reference:
 original=G/'Content'/f.relative_to(R/'Content')
 assert original.exists() and hash(original)==hash(f),f
newAssets=list((R/'Content/Gate09_QA_20261019/DharmaWeaponSource').rglob('*.uasset'))
fieldAssets=list((R/'Content/Gate09_QA_20261019/SovereignEclipseGeometry').rglob('*.uasset'))
assert len(newAssets)==3
assert len(fieldAssets)==12
manifest=json.loads((P/'DharmaWeaponInterchange_v6_20261019/INTERCHANGE_MANIFEST.json').read_text(encoding='utf8'))
source=[
 P/'G09_Dharma_WeaponSourceRetarget_v6_20261019.blend',
 P/'G09_SovereignEclipse_Architectural_SOURCE_v6.blend',
 P/'G09_SovereignEclipse_Architectural_INTERCHANGE_v6.glb',
 P/'G09_SovereignEclipse_Architectural_UE_GROUPED_v6.glb',
 P/'G09_SovereignEclipse_Architectural_PREVIEW_v6.png',
 P/'G09_SovereignEclipse_Architectural_MANIFEST_v6.json',
 P/'DharmaWeaponInterchange_v6_20261019/INTERCHANGE_MANIFEST.json',
 P/'DharmaSourceMotionVisualQA_v6_20261019/VISUAL_QA_MANIFEST.json',
 P/'G09_DharmaMixamoSource_v6_20261019/MANIFEST.json']
source+=list((P/'DharmaWeaponInterchange_v6_20261019').glob('*.fbx'))
source+=list((P/'DharmaSourceMotionVisualQA_v6_20261019').glob('*.png'))
source+=list((P/'G09_DharmaMixamoSource_v6_20261019').glob('*.gz'))
source+=list((R/'Saved/Gate09QA').glob('unreal_0[6789]*.json'))
source+=[R/'G09_AnimQA.uproject']
source+=list((R/'Config').rglob('*.ini'))
files=source+reference+newAssets+fieldAssets
assert len({str(x) for x in files})==len(files)
for x in files:assert x.exists(),x
def arc(f):
 if f.is_relative_to(R):return str(f.relative_to(R)).replace('\\','/')
 return 'PrivateGate09Sources/'+str(f.relative_to(P)).replace('\\','/')
records=[{'name':arc(f),'sha256':hash(f),'bytes':f.stat().st_size} for f in files]
proof={'schema':'GATE09_LUMINOUS_V6_COMBAT_REAL_ASSETS_PORTABLE_QA_V1',
'status':'PARTIAL_SOURCE_AND_NATIVE_TECHNICAL_PASS_ART_NOT_APPROVED',
'actual_native_attack_source_sequences':3,
'original_unmodified_animation_sequences':4,
'original_gate12_copied_unchanged_skeletal_assets':6,
'new_native_unreal_field_static_meshes':4,'new_native_unreal_field_materials':8,
'new_full_authored_A1_sequence':False,'real_sword_equipped':False,
'three_hit_notifies_implemented':False,'native_niagara_implemented':False,
'world_field_authority_logic_implemented':False,'field_animated':False,
'visual_acceptance':False,'files':records}
with zipfile.ZipFile(out,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=6,allowZip64=True) as z:
 for f in files:z.write(f,arc(f))
 z.writestr('GATE09_CHECKPOINT_MANIFEST.json',json.dumps(proof,indent=2))
with zipfile.ZipFile(out,'r') as z:
 assert z.testzip() is None
 assert sum(n.endswith('.uasset') for n in z.namelist())==21
 print('PACKAGE_ENTRIES',len(z.namelist()))
print('GATE09_COMBAT_CHECKPOINT_PACKAGE',out,'BYTES',out.stat().st_size,'SHA256',hash(out),flush=True)
