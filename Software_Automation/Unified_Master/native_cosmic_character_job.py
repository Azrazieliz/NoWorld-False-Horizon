"""One-command production-tool prototype: live KK Maker -> protected baseline -> native cosmic variant.
Verifies live pixels, saved card, reload and exact embedded textures. Does not use screen clicks.
"""
import argparse,datetime,hashlib,json,pathlib,subprocess,sys,uuid,importlib.util
HERE=pathlib.Path(__file__).resolve().parent
SB=pathlib.Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014')
MASTER=pathlib.Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME')
CH=SB/'BepInEx'/'SM01_SANDBOX'
CLI=MASTER/'AUTOMATION'/'sm01_sandbox_client.py'
def h(path):return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()
def call(op,kwargs=None):
 nonce='auto_'+uuid.uuid4().hex[:15]
 cmd=[sys.executable,str(CLI),'--op',op,'--nonce',nonce,'--timeout','30']
 for k,v in (kwargs or {}).items():cmd.extend(['--'+k,str(v)])
 p=subprocess.run(cmd,capture_output=True,text=True,timeout=40)
 a=json.loads(p.stdout.strip().splitlines()[-1])
 if p.returncode or a.get('status')!='RECEIPT_WRITTEN':raise RuntimeError(op+' failed '+p.stdout[-800:])
 return a,nonce
def card_name(n):return 'SM01_AUTOTEST_'+n+'.png'
def capture(run_folder):
 _,n=call('save_derived_card')
 card=SB/'UserData'/'chara'/'female'/card_name(n)
 a,_=call('verify_derived_card',{'file-name':card.name})
 if 'PASS_NATIVE_CARD_RELOAD' not in a['data']:raise RuntimeError('Native card reload failed')
 module=importlib.util.spec_from_file_location('native_intake',HERE/'intake_native_kk_card.py')
 imp=importlib.util.module_from_spec(module);module.loader.exec_module(imp)
 return imp.ingest(card,run_folder)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',required=True);a=ap.parse_args()
 output=pathlib.Path(a.output).resolve()
 if output.exists():raise FileExistsError(output)
 output.mkdir(parents=True)
 _,ins=call('inspect_appearance')
 baseline=capture(output/'baseline')
 options={'group':'character','slot':0,'file-name':'o_body_a','parent':'cf_m_body'}
 src,_=call('export_surface_texture',options)
 source=pathlib.Path(next(x.split('\t',1)[1] for x in src['data'].splitlines() if x.startswith('source_png\t')))
 sourcehash=h(source)
 texture='AutoCosmic_'+uuid.uuid4().hex[:14]+'.png'
 produced=CH/'surface_edits'/texture
 make=subprocess.run([sys.executable,str(HERE/'cosmic_skin_texture.py'),'--source',str(source),'--output',str(produced)],capture_output=True,text=True,timeout=70)
 if make.returncode:raise RuntimeError(make.stderr[-800:])
 applied,_=call('apply_surface_texture',{**options,'recipe-file':texture})
 if 'visible_texture_readback\tTrue' not in applied['data']:raise RuntimeError('Live material not read back')
 checked,_=call('export_surface_texture',options)
 observed=pathlib.Path(next(x.split('\t',1)[1] for x in checked['data'].splitlines() if x.startswith('source_png\t')))
 from PIL import Image,ImageChops
 with Image.open(observed) as ob,Image.open(produced) as expected:
  if ob.size!=expected.size or ImageChops.difference(ob.convert('RGBA'),expected.convert('RGBA')).getbbox():
   raise RuntimeError('Texture pixels mismatch after applying native')
 result=capture(output/'variant')
 target=pathlib.Path(result['source_path'])
 textures=MASTER/'VALIDATION'/('AutoCosmicMaterial_'+uuid.uuid4().hex[:12])
 exp=subprocess.run([sys.executable,str(MASTER/'AUTOMATION'/'export_card_materials_for_unreal.py'),'--card',str(target),'--out',str(textures)],capture_output=True,text=True,timeout=45)
 if exp.returncode:raise RuntimeError('Card texture extraction failed '+exp.stderr[-500:])
 manifest=json.loads((textures/'MATERIAL_EXPORT_MANIFEST.json').read_text())
 if not manifest['textures']:raise RuntimeError('No exported native textures')
 final={'schema':'NW_COSMIC_NATIVE_JOB_V1','status':'PASS_NATIVE_CHARACTER_DERIVATIVE',
 'baseline':baseline['copy_path'],'variant':result['copy_path'],
 'variant_card_reload':True,'source_texture_unchanged':h(source)==sourcehash,
 'native_material_pixel_match':True,'extracted_textures':len(manifest['textures']),
 'ue_texture_manifest':str(textures/'MATERIAL_EXPORT_MANIFEST.json'),
 'visual_likeness_accepted':False,'unreal_runtime_accepted':False}
 (output/'job_receipt.json').write_text(json.dumps(final,indent=2))
 print('NW_NATIVE_COSMIC_JOB='+json.dumps(final))
if __name__=='__main__':main()
