"""Portable, side-effect-free regression tests of persistent learning before every Gate."""
import json, pathlib, tempfile, shutil
from mastery_preflight import ensure_master, LEDGER, HERE, SOURCE_SNIPPETS
from gate_start import gate_start
from coordinate_inheritance_guard import evaluate as check_inherited_coordinates
from native_3d_material_scale_guard import assess as check_character_material_scale
from intake_native_kk_card import ingest as ingest_native_card, hash_file as hash_native_card
from PIL import Image
cases={}
def test(name,callback):
    try:cases[name]=bool(callback())
    except Exception as e:cases[name]=False;print('ERROR',name,repr(e))
def rejects(fn):
    try:fn()
    except (ValueError,FileNotFoundError,RuntimeError,json.JSONDecodeError):return True
    return False
test('original_master_26_rules_12_gates',lambda:ensure_master()['rules']>=26 and ensure_master()['gates']==12)
test('new_K027_guard_persisted',lambda:any(r['id']=='K027' and r.get('regression_test') for r in json.loads(LEDGER.read_text(encoding='utf-8-sig'))['rules']))
test('inherited_six_presets_blocked',lambda:(lambda r:r['status']=='REVIEW_INHERITED_COORDINATES' and r['no_auto_promotion'] and r['baseline_matching_indices']==[1,2,3,4,5,6])(check_inherited_coordinates({'coordinates':{'0':{'clothes':[{'part':0}]}}})))
test('all_seven_changed_no_false_inheritance',lambda:not check_inherited_coordinates({'coordinates':{str(i):{'clothes':[{'part':0}]} for i in range(7)}})['no_auto_promotion'])
test('K029_persistent',lambda:any(r['id']=='K029' for r in json.loads(LEDGER.read_text(encoding='utf-8-sig'))['rules']))
test('K029_normal_channel_loss_blocks_art_acceptance',lambda:(lambda r:r['status']=='P0_HOLD_MATERIAL_PARITY' and r['scale']=='PASS_RELATIVE_BOUNDING_DIMENSIONS' and r['material_normal_channel']=='FAIL_SOURCE_NORMAL_MAPS_NOT_BOUND' and not r['visual_fidelity_accepted'])(check_character_material_scale(source_normal_maps=29,target_normal_bindings=0,source_dimensions=[1.2383,1.5217,0.6701],target_dimensions=[1.2383,1.5217,0.6701])))
test('K029_scale_change_separate_from_shader_loss',lambda:(lambda r:r['scale']=='FAIL_BOUNDING_DIMENSIONS_CHANGED' and r['status']=='P0_HOLD_SCALE')(check_character_material_scale(source_normal_maps=0,target_normal_bindings=0,source_dimensions=[1,2,3],target_dimensions=[0.01,0.02,0.03])))
test('K029_bad_dimensions_fail_closed',lambda:rejects(lambda:check_character_material_scale(source_normal_maps=0,target_normal_bindings=0,source_dimensions=[1,0,3],target_dimensions=[1,2,3])))
for i in range(1,13):
    tag='Gate%02d'%i
    test('gate%02d_startup'%i,lambda t=tag:gate_start(t)['gate']==t and bool(gate_start(t)['known_methods']))
with tempfile.TemporaryDirectory(prefix='nw_method_reg_') as td:
    tmp=pathlib.Path(td)
    test('new_K028_revision_rule_persisted',lambda:any(r['id']=='K028' and r.get('regression_test') for r in json.loads(LEDGER.read_text(encoding='utf-8-sig'))['rules']))
    def revision_capture_guard():
        old=tmp/'old.png'; new=tmp/'new.png'
        Image.new('RGB',(256,352),(17,28,39)).save(old)
        Image.new('RGB',(256,352),(93,47,15)).save(new)
        first=ingest_native_card(old,tmp/'revision_previous')
        second=ingest_native_card(new,tmp/'revision_current')
        try:ingest_native_card(new,tmp/'revision_previous'); refused=False
        except FileExistsError:refused=True
        return refused and first['sha256_original']!=second['sha256_original'] and hash_native_card(tmp/'revision_previous'/'SOURCE_CARD_UNMODIFIED.png')==first['sha256_original'] and hash_native_card(tmp/'revision_current'/'SOURCE_CARD_UNMODIFIED.png')==second['sha256_original']
    test('source_revisions_immutable_and_no_overwrite',revision_capture_guard)
    original=json.loads(LEDGER.read_text(encoding='utf-8-sig'))
    def altered(name,edit):
        x=json.loads(json.dumps(original));edit(x)
        path=tmp/(name+'.json');path.write_text(json.dumps(x))
        return path
    missing=altered('missing_rule',lambda d:d['rules'].pop(10))
    duplicate=altered('dup_rule',lambda d:d['rules'].append(dict(d['rules'][0])))
    no_gate=altered('gate_deleted',lambda d:d['gates'].pop('Gate12'))
    no_mission=altered('mission_lost',lambda d:d.update(mission='unrelated files'))
    no_steps=altered('steps_lost',lambda d:d['startup_contract'].update(mandatory=[]))
    empty=altered('empty_method',lambda d:d['rules'][5].update(procedure=''))
    for name,path in [('missing_required_lesson',missing),('duplicated_lesson',duplicate),('missing_gate12',no_gate),
                      ('lost_automation_mission',no_mission),('lost_mandatory_startup',no_steps),
                      ('empty_known_method',empty)]:
        test(name,lambda p=path:rejects(lambda:ensure_master(p,HERE)))
    # Simulate source regression in a throwaway copy; never touch production source.
    shadow=tmp/'shadow'
    for rel in SOURCE_SNIPPETS:
        to=shadow/rel;to.parent.mkdir(parents=True,exist_ok=True)
        to.write_text((HERE/rel).read_text(encoding='utf-8-sig'),encoding='utf-8')
    test('complete_shadow_sources_pass',lambda:ensure_master(LEDGER,shadow)['status']=='PASS_MASTER_METHODS')
    (shadow/'SM01_Runtime_Source/SM01RuntimeController.cs').write_text('accidentally removed AssignCoordinate')
    test('missing_native_card_roundtrip_rejected',lambda:rejects(lambda:ensure_master(LEDGER,shadow)))
total=len(cases);passed=sum(cases.values())
print('NW_MASTER_LEARNING_REGRESSION='+json.dumps({'total':total,'passed':passed,'checks':cases,'native_execution':False},sort_keys=True))
if passed!=total:raise SystemExit(2)
