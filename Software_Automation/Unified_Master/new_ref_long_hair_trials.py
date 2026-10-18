"""Native 3-wig comparison; inspect real renders before choosing, do not save."""
import pathlib,uuid,io,contextlib,json,importlib.util
R=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST06_WHITE_GOLD_DANCER')
cli_path=r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME\AUTOMATION\sm01_sandbox_client.py'
s=importlib.util.spec_from_file_location('native',cli_path);cli=importlib.util.module_from_spec(s);s.loader.exec_module(cli)
def op(kind,**args):
 nonce='DNC2_'+uuid.uuid4().hex[:14];buf=io.StringIO()
 with contextlib.redirect_stdout(buf):rc=cli.submit({'op':kind,'nonce':nonce,**args},9)
 try:r=json.loads(buf.getvalue().strip().splitlines()[-1])
 except Exception as e:raise RuntimeError(buf.getvalue()[-250:])
 if rc or r.get('status')!='RECEIPT_WRITTEN':print('FAILED',kind,args,str(r)[:350],flush=True)
 return nonce,rc
for name,guid,cat,item in [
 ('TAKANE_LONG','lrdj.starlit_season',122,5),
 ('ANNE_LONG','anne001',122,84210),
 ('MINAMI_LONG','lrdj.minami_ssr_outfits',122,2),
 ('IORI_LONG','lrdj.starlit_season',122,8)]:
 op('apply_accessory',accessorySlot=5,guid=guid,category=cat,originalId=item,parentBone='a_n_headtop')
 op('set_accessory_vector',accessorySlot=5,group='pos',value=0,value2=0,value3=-.03)
 nonce,rc=op('render_views')
 (R/(name+'_nonce.txt')).write_text(nonce)
 print('TRIAL',name,nonce,flush=True)
