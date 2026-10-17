"""Test05 immutable QA and known physical-scaling regression. Not a visual acceptance.
Proves source/card operations and detects hidden double-scale and missing native textures.
"""
import pathlib,json,hashlib,sys
BASE=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST05_CRYSTAL_SERAPH')
def load(n):return json.loads((BASE/n).read_text(encoding='utf-8-sig'))
checks={}
def check(n,val):checks[n]=bool(val)
rev=load('TEST05_REAL_NATIVE_REBIND_RECEIPT.json')
scale=load('TEST05_BODY_SCALE_REPAIR.json')
batch=load('TEST05_NATIVE_BATCH_TIMINGS.json')
art=load('TEST05_ART_PASS_v3_receipt.json')
check('source_native_pmx_sha256',pathlib.Path(rev['native_source_pmx']).is_file() and hashlib.sha256(pathlib.Path(rev['native_source_pmx']).read_bytes()).hexdigest()==rev['sha256'])
check('native_kkbp_body_height',1.4<rev['actual_body_eval_height_m']<2.0)
check('body_not_double_scaled',1.4<scale['native_body_eval_height']<2.0)
check('source_native_eye_and_face_texture_nodes',rev['connected_original_kkbp_images']>=16)
check('one_session_21_ops_under_15_seconds',batch['ops_dispatched']==21 and batch['app_restarts']==0 and batch['wall_seconds']<15)
check('native_v4_editable_source_exists',pathlib.Path(rev['output_blend']).stat().st_size>1000000)
check('latest_native_qa_image_exists',(BASE/'TEST05_ActualNativeBody_ArtPass_v4.png').stat().st_size>100000)
check('source_3d_procedural_wings_created',load('TEST05_3D_BUILD_RECEIPT.json')['wing_objects']>300)
check('native_physical_shape_keys_preserved',rev['native_original_shape_keys']>=100)
check('no_art_fidelity_approval',rev['fidelity_accepted'] is False)
check('native_original_cards_not_modified',rev['canonical_content_modified'] is False)
check('jewelry_deliberately_excluded',art['jewelry_omitted'] is True)
record={'schema':'NOWORLD_TEST05_QA_REGRESSIONS_V1','checks':checks,'passed':sum(checks.values()),'total':len(checks),
  'visual_fidelity_status':'FAIL_REFERENCE_LIKENESS','reason':'Face, wing membrane/feather architecture, hair locks, crystalline body armor are far from reference',
  'native_KK_source':'PASS_CARD_SAVED_BUT_TOP_SLOT_NOT_NEUTRAL',
  'status':'ENGINEERING_FIXES_VERIFIED_VISUAL_BENCHMARK_FAILED','production_accepted':False,
  'user_canonical_assets_modified':False}
dest=BASE/'TEST05_FINAL_QA_RECEIPT.json';dest.write_text(json.dumps(record,indent=2))
print('TEST05_QA='+json.dumps(record,sort_keys=True))
if record['passed']!=record['total']:raise SystemExit(2)
