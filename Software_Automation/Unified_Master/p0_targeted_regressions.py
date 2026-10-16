"""Read-only P0 material and effective morph parity regression from authentic
hash-locked Luminous source Blender + archived GLB + exported source images.
This is NOT an Unreal/Unity acceptance test.
"""
import pathlib,json,hashlib,statistics
OUT=pathlib.Path(r'D:\UnrealProjects\NW_SoftwareMaster_P0_20261016')
def J(file):return json.loads((OUT/file).read_text(encoding='utf-8-sig'))
tests={}
def verify(n,v):tests[n]=bool(v)
e=J('P0_MORPH_SOURCE_TO_GLB_NAME_DELTA_PARITY.json')
g=J('P0_GLB_EFFECTIVE_SPARSE_MORPH_DELTAS.json')
reimport=J('P0_BLENDER_GLB_REIMPORT_EFFECTIVE_MORPHS.json')
a=J('P0_NATIVE_GLB_EYELINE_ALPHA_AUDIT.json')
i=J('P0_NATIVE_GLB_EYELINE_EMBEDDED_COMPARISON.json')
cards=J('SOURCE_IMMUTABLE_INPUT_MANIFEST.json')
for state,count in [('CLOTHED',247),('UNDERWEAR',242),('NUDE',242)]:
 row=e[state]
 verify('morph_count_'+state,row['source_effective_shape_keys']==count and row['found_effective_glb_keys']==count)
 verify('clean_blender_import_effective_'+state,reimport[state]['effective_imported_keyblocks']==count)
 verify('no_effective_morph_loss_'+state,len(row['missing_effective_keys'])==0)
 verify('effective_name_indices_stable_'+state,len(row['index_inconsistent_keys'])==0)
 verify('morph_max_delta_median_unit_'+state,abs(row['delta_ratio_median']-1.0)<1e-6)
 verify('glb_has_sparse_morphs_'+state,sum(1 for x in g[state]['per_target'] if x['is_sparse'])>5000)
 em=i[state];native=a[state]['glb_eye_face_materials']
 up=[m for m in native if m['name']=='KK cf_m_eyeline_00_up']
 verify('source_glb_upper_eyeline_blend_'+state,len(up)==1 and up[0]['alpha_mode']=='BLEND')
 verify('embedded_upper_eyeline_gradient_'+state,em['KK cf_m_eyeline_00_up']['partial_alpha_fraction']>.1)
 at=[x for x in a[state]['source_alpha'] if pathlib.Path(x['file']).name=='cf_m_eyeline_00_up_MT_CT.png']
 verify('source_upper_eyeline_alpha_present_'+state,len(at)==1 and at[0]['partial_fraction']>.01)
# independent stage verification and hash locks:
for name,r in cards.items():
 file=pathlib.Path(r['path'])
 verify('immutable_input_'+name,file.is_file() and hashlib.sha256(file.read_bytes()).hexdigest()==r['sha256'])
mat=J('P0_GATE12_MATERIAL_AND_EFFECTIVE_MORPH_CONTRACT.json')
verify('gate12_contract_source_not_accepted',mat['unreal_native_execution']=='NOT_RUN_IN_SOFTWARE_MASTERY' and not mat['production_default_changed'])
verify('nine_eyeline_constraints',len(mat['material_contracts'])==9 and sum(x['alpha_gradient_must_survive'] for x in mat['material_contracts'])==3)
restore=OUT/'B_CAPTURE_PLUGIN'/'SM01RuntimeController_SandboxOriginal_Backup.dll'
sb=pathlib.Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014\BepInEx\plugins\SM01RuntimeController.dll')
verify('native_capture_sandbox_original_untouched',restore.is_file() and sb.is_file() and hashlib.sha256(restore.read_bytes()).digest()==hashlib.sha256(sb.read_bytes()).digest())
qa=J('A_REPAIR_TEST3/A_TEXTURE_IMPORT_REPAIR_RECEIPT.json')
verify('zero_texture_defect_repaired_with_original_atlases',qa['before_image_nodes']==0 and qa['after_image_nodes']>=45 and qa['restored_count']>=40)
result={'schema':'NW_P0_TARGETED_FOLLOWUP_REGRESSION_V1','checks':tests,'total':len(tests),'passed':sum(tests.values()),
  'native_capture':'NOT_VALIDATED_ORIGINAL_STEAM_IN_USE','unreal_material_and_morph':'GATE12_NATIVE_TEST_REQUIRED',
  'production_default_changed':False}
path=OUT/'P0_TARGETED_FOLLOWUP_REGRESSIONS.json';path.write_text(json.dumps(result,indent=2))
print('P0_TARGETED_REGRESSIONS',result['passed'],'/',result['total'],'FAILED',[k for k,v in tests.items() if not v],'receipt',path,flush=True)
if result['passed']!=result['total']:raise SystemExit(2)
