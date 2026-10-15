"""Portable, side-effect-free regression tests of persistent learning before every Gate."""
import json, pathlib, tempfile, shutil
from mastery_preflight import ensure_master, LEDGER, HERE, SOURCE_SNIPPETS
from gate_start import gate_start
cases={}
def test(name,callback):
    try:cases[name]=bool(callback())
    except Exception as e:cases[name]=False;print('ERROR',name,repr(e))
def rejects(fn):
    try:fn()
    except (ValueError,FileNotFoundError,RuntimeError,json.JSONDecodeError):return True
    return False
test('original_master_26_rules_12_gates',lambda:ensure_master()['rules']==26 and ensure_master()['gates']==12)
for i in range(1,13):
    tag='Gate%02d'%i
    test('gate%02d_startup'%i,lambda t=tag:gate_start(t)['gate']==t and bool(gate_start(t)['known_methods']))
with tempfile.TemporaryDirectory(prefix='nw_method_reg_') as td:
    tmp=pathlib.Path(td)
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
