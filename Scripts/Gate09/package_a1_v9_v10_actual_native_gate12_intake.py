"""Private Gate12 integration intake for genuine A1 Luminous Base v6 authored source.
An actual packaged source/runtime content checkpoint, NOT artistic/gameplay signoff.
Does not replace approved Luminous character or reproduce third-party mod archives.
"""
import pathlib,json,hashlib,zipfile,time
P=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
Q=pathlib.Path(r'D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019')
out=P/'G09_Luminous_A1_BaseV6_GroundedRealBlade_UE583_PRIVATE_INTAKE_20261020.zip'
if out.exists():raise FileExistsError(out)
def sha(f):
 h=hashlib.sha256()
 with f.open('rb') as src:
  for buf in iter(lambda:src.read(1<<20),b''):h.update(buf)
 return h.hexdigest()
qa=Q/'Saved/Gate09QA'
def receipt(n):return json.loads((qa/n).read_text(encoding='utf8'))
original=receipt('unreal_21_a1_v9_grounded_original_native.json')
cold=receipt('unreal_22_a1_v9_FINAL_cold_root_notifies.json')
fx=receipt('unreal_24_a1_v10_physical_blade_trails.json')
last=receipt('unreal_25_a1_v10_final_mesh_cold.json')
video=json.loads((P/'Luminous_A1_ScarletCadence_v10_CompositedProductionQA.json').read_text(encoding='utf8'))
assert original['status'].startswith('PASS_975_TRACKS_')
assert cold['status']=='PASS_GROUNDED_V9_35CM_AND_FIVE_CONCRETE_EVENTS_COLD'
assert fx['status']=='PASS_NINE_NATIVE_REAL_MOTION_SWEEP_MESHES_THREE_MATERIALS'
assert last['status']=='PASS_UE5_V10_PHYSICAL_FX_NINE_MESHES_THREE_MATERIALS_COLD'
assert cold['native_bone_tracks']==975 and len(cold['events'])==5
assert len(fx['static_mesh_fx'])==9 and len(fx['materials'])==3
assert video['video_stream']['nb_frames']=='33'
groups=('LuminousA1Grounded_v9','LuminousA1NotifyClasses_v7','LuminousSwordVisualSource_v7','LuminousA1SweptFX_v10')
native=[]
for grp in groups:
 root=Q/'Content/Gate09_QA_20261019'/grp
 assert root.is_dir(),root
 files=sorted(root.rglob('*.uasset'))
 assert files,(grp,files)
 native+=files
n=[len(list((Q/'Content/Gate09_QA_20261019'/g).rglob('*.uasset'))) for g in groups]
assert n[0]==1 and n[1]==5 and n[2]>=16 and n[3]>=12,n
sources=[
 P/'Luminous_A1_ScarletCadence_GroundedRootMotion_v9.blend',
 P/'Luminous_A1_ScarletCadence_ActualSwordSweptFX_v10.blend',
 P/'A1_ActualGrounded_UnrealExports_v9/G09_LuminousV3_A1_ScarletCadence_RealSword_Grounded_v9.fbx',
 P/'Luminous_A1_ScarletCadence_ActualSwordSweptFX_Unreal_v10.glb',
 P/'Luminous_A1_ScarletCadence_GroundedRootMotion_v9.json',
 P/'Luminous_A1_ScarletCadence_ActualSwordSweptFX_v10.json',
 P/'Luminous_A1_ScarletCadence_v10_Composited_RealBladeTrail_30fps.mp4',
 P/'Luminous_A1_ScarletCadence_v10_CompositedProductionQA.json',
 P/'G09_A1_v10_MotionBasedFX_BeforeAfter_QA.jpg'
]
sources+=sorted((P/'Luminous_A1_ScarletCadence_GroundedRootMotion_v9_Captures').glob('*.png'))
sources += [qa/n for n in (
'unreal_21_a1_v9_grounded_original_native.json',
'unreal_22_a1_v9_FINAL_cold_root_notifies.json',
'unreal_24_a1_v10_physical_blade_trails.json',
'unreal_25_a1_v10_final_mesh_cold.json')]
for x in native+sources:assert x.exists() and x.stat().st_size>100,x
manifest={'schema':'GATE09_LUMINOUS_A1_NATIVE_REAL_ASSETS_GATE12_INTAKE_V9V10',
 'status':'AUTHORED_SOURCE_NATIVE_UE_ANIMATION_FX_PASS_RUNTIME_GAMEPLAY_UNFINISHED',
 'assembled':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),
 'production_project':'NoWorld False Horizon',
 'preserved_foundation':True,'approved_luminous_character_assets_unchanged':True,
 'references_canonical_existing_Luminous_CLOTHED_975_skeleton':True,
 'native_animation':original['native_asset'],
 'center_root_movement_native_cm':cold['root_motion_cm'],
 'physical_hit_notify_classes':cold['events'],
 'native_FX_meshes':fx['static_mesh_fx'],
 'native_FX_materials':fx['materials'],
 'source_sword_equipment':'TEMPORARY_VISUAL_FITTING_NONCANONICAL_GATE07_REQUIRED',
 'niagara_system':'NOT_IMPLEMENTED',
 'gate12_runtime_damage_collision_cancel_windows':'NOT_IMPLEMENTED',
 'full_character_kit_complete':False,
 'rendered_preview':str(P/'Luminous_A1_ScarletCadence_v10_Composited_RealBladeTrail_30fps.mp4'),
 'original_skinned_reference_assets_included':False,
 'source_private_third_party_mod_archives_included':False,
 'proposed_intake_instruction':'Extract Content/Gate09_QA_20261019 assets into Gate12 Content at matching paths; use EXISTING /Game/Experimental/LuminousValentineV3/CLOTHED skeleton; DO NOT overwrite canonical character assets; Gate12 owns gameplay and Gate07 sword equipment acceptance.',
 'native_groups_assets_counts':dict(zip(groups,n))}
files=native+sources
arcs=[(f.relative_to(Q) if f.is_relative_to(Q) else pathlib.Path('ProtectedGate09Sources')/f.relative_to(P)).as_posix() for f in files]
assert len(arcs)==len(set(arcs))
manifest['files']=[{'path':n,'bytes':f.stat().st_size,'sha256':sha(f)} for f,n in zip(files,arcs)]
with zipfile.ZipFile(out,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=6,allowZip64=True) as z:
 for f,n in zip(files,arcs):z.write(f,n)
 z.writestr('G09_GATE12_NATIVE_REAL_A1_MANIFEST.json',json.dumps(manifest,indent=2))
with zipfile.ZipFile(out) as z:
 assert z.testzip() is None
 assert sum(x.endswith('.uasset') for x in z.namelist())==len(native)
print('G09_V9V10_PRIVATE_GATE12_REAL_ARTIFACT_PACKAGE',out,'BYTES',out.stat().st_size,
'SHA256',sha(out),'UE_ASSETS',len(native),'SOURCES',len(sources),flush=True)
