"""Reference Test 03: native Koikatsu single-run construction, no intermediate save.
Native outfit persistence and reloaded-card content verified separately.
"""
import subprocess,sys,uuid,json,time,pathlib
R=pathlib.Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME')
O=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261016\TEST03_ORANGE_BRAID')
CLI=R/'AUTOMATION'/'sm01_sandbox_client.py'
t0=time.perf_counter();calls=[]
def native(op,optional=False,**kwargs):
  nonce='t03_'+uuid.uuid4().hex[:14]
  args=[sys.executable,str(CLI),'--op',op,'--nonce',nonce,'--timeout','11']
  for k,v in kwargs.items():args+=['--'+k.replace('_','-'),str(v)]
  p=subprocess.run(args,capture_output=True,text=True,encoding='utf8',errors='replace',timeout=19)
  try:r=json.loads(p.stdout.strip().splitlines()[-1])
  except Exception:raise RuntimeError('INVALID RECEIPT '+op+' '+p.stdout[-800:])
  if p.returncode or r['status']!='RECEIPT_WRITTEN':
    if optional:
      print('OPTIONAL_FAILED',op,str(r)[-400:],flush=True);return None
    raise RuntimeError('FAILED '+op+' '+str(r)[-900:])
  calls.append(op);print('OK',op,'%.1fs'%(time.perf_counter()-t0),flush=True)
  return r,nonce
def rgb(group,slot,rgb):
  result=native('inspect_surface_targets',optional=True,group=group,slot=slot)
  if result is None:return
  scanned=set()
  for line in result[0]['data'].splitlines():
    f=line.split('\t')
    if len(f)<4 or f[0]!='target':continue
    key=(f[1],f[2])
    if key in scanned:continue
    scanned.add(key)
    native('set_surface_color',optional=True,group=group,slot=slot,file_name=f[1],parent=f[2],value=rgb[0],v2=rgb[1],v3=rgb[2],v4=1)
def run():
  O.mkdir(parents=True,exist_ok=True)
  face=[.5]*52;body=[.5]*44
  # Named anatomy anchors from native ChaCustom slider catalog.
  for i,v in {0:.44,3:.48,4:.45,8:.39,9:.39,14:.32,15:.31,16:.49,19:.43,20:.53,25:.58,26:.61,27:.57,28:.59,29:.48,30:.48,31:.47,33:.39,34:.59,35:.62,41:.55,42:.56,44:.62,45:.52,46:.62,47:.43}.items():face[i]=v
  for i,v in {0:.67,1:.48,2:.88,3:.58,4:.53,5:.51,6:.48,7:.54,8:.73,10:.65,17:.43,18:.49,19:.51,20:.56,21:.48,22:.52,23:.52,24:.45,25:.38,26:.42,27:.40,28:.42,29:.81,30:.57,31:.49,32:.50,33:.43,35:.41,38:.86,39:.79,40:.78,41:.65,42:.59,43:.55}.items():body[i]=v
  O.joinpath('native_morph_96.json').write_text(json.dumps([{'group':k,'index':i,'value':v} for k,xs in [('face',face),('body',body)] for i,v in enumerate(xs)]))
  native('apply_morph_batch',morphs=str(O/'native_morph_96.json'))
  native('set_face_makeup')
  native('set_iris_asset',effective_id=6)
  native('set_eye_iris',value=.16,v2=.57,v3=.98)
  # Native hairstyle: layered messy front, long flowing rear, no Chika default retained.
  for slot,item in [(0,19),(1,36),(2,2),(3,0)]:
    native('set_hair',index=slot,effective_id=item)
    native('set_hair_palette',index=slot,value=.98,v2=.38,v3=.052)
  # Clear school-uniform and underlayers as actual asset IDs.
  for part in [1,2,3,4,5,7,8]:native('set_clothes',index=part,effective_id=0)
  native('set_clothes_top',guid='Gugi.ShirtDress',category=105,original_id=4)
  rgb('clothing',0,[1.,.985,.985])
  native('set_clothes',index=5,guid='com.Quokka.thighhighs',category=110,original_id=93)
  rgb('clothing',5,[.97,.97,1.])
  # Large orange thick braid, headside anchor; adjust native axes rather than changing library.
  native('apply_accessory',guid='codevein.AccessoryHair',category=122,original_id=670020,slot=0,parent='a_n_headside')
  rgb('accessory',0,[.97,.40,.055])
  native('set_accessory_vector',slot=0,group='scale',value=.95,v2=1.20,v3=.95)
  native('set_accessory_vector',slot=0,group='pos',value=-11,v2=-4,v3=8)
  native('set_accessory_vector',slot=0,group='rot',value=0,v2=345,v3=8)
  # Earring pair: native accessory metadata, parent to left/right ear.
  native('apply_accessory',guid='conq.accear.earrings1',category=123,original_id=12,slot=1,parent='a_n_ear_L',optional=True)
  native('apply_accessory',guid='conq.accear.earrings1',category=123,original_id=13,slot=2,parent='a_n_ear_R',optional=True)
  native('render_views')
  (O/'stage_receipt.json').write_text(json.dumps({'status':'NATIVE_TEST03_STAGE_RENDERED','elapsed':round(time.perf_counter()-t0,2),'calls':calls,'no_card_saved_yet':True},indent=2))
  print('TEST03_READY_FOR_VISUAL_INSPECTION',round(time.perf_counter()-t0,2),flush=True)
if __name__=='__main__':run()
