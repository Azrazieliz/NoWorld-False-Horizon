"""Verify all current package JSON, Python syntax, expected evidence and versioned manifest hashes.
A static verifier. Does NOT count as live Blender or native Unreal execution.
"""
import json,hashlib,os,pathlib,py_compile
base=pathlib.Path(__file__).resolve().parents[1]
manifest=json.loads((base/'MANIFEST.json').read_text(encoding='utf-8-sig'))
results={'manifest_expected':manifest['file_count'],'hash_failures':[],
 'json_checked':0,'scripts_compiled':0,'required_evidence':{},'verifier_version':1}
for entry in manifest['files']:
 path=base/entry['relative_path']
 if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest()!=entry['sha256']:
  results['hash_failures'].append(entry['relative_path'])
for path in base.rglob('*.json'):
 json.loads(path.read_text(encoding='utf-8-sig'));results['json_checked']+=1
for path in base.rglob('*.py'):
 py_compile.compile(str(path),doraise=True);results['scripts_compiled']+=1
needed=['VALIDATION/phase2/sanctuary_results.json','VALIDATION/phase2/beast_results.json',
 'VALIDATION/phase2/hybrid_results.json','VALIDATION/phase2/koikatsu_shards_summary.json',
 'VALIDATION/phase2/Unreal/ue_anim_only_results.json','VALIDATION/phase2/Unreal/ue_batch_results.json',
 'VALIDATION/phase2/DEMONSTRATION_STATUS.json']
for entry in needed:results['required_evidence'][entry]=(base/entry).is_file()
results['pass']=not results['hash_failures'] and all(results['required_evidence'].values()) and manifest['file_count']==len(manifest['files'])
print(json.dumps(results))
if not results['pass']:raise SystemExit(2)
