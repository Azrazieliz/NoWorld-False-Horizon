"""Single Maker session, genuine loaded Dancer set matching gold/white veil reference.
Disposable QA, no canonical changes. Native operations batched in one Python session.
"""
import json,time,pathlib,sys,uuid,io,contextlib,importlib.util
M=pathlib.Path(r'D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Unified_Master')
sys.path.insert(0,str(M))
import kk_one_session as one
o=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST06_WHITE_GOLD_DANCER')
o.mkdir(parents=True,exist_ok=True)
s=importlib.util.spec_from_file_location('native',str(one.CLIENT));cli=importlib.util.module_from_spec(s);s.loader.exec_module(cli)
records=[];start=time.perf_counter()
def do(op,optional=False,**kw):
 nonce='DNC_'+uuid.uuid4().hex[:14];data={'op':op,'nonce':nonce}
 for k,v in kw.items():data[one.ARG_NAMES.get(k,k)]=v
 buf=io.StringIO()
 with contextlib.redirect_stdout(buf):ret=cli.submit(data,9)
 raw=buf.getvalue().strip().splitlines()
 try:answer=json.loads(raw[-1])
 except Exception as er:answer={'error':str(er),'raw':buf.getvalue()[-350:]}
 okay=ret==0 and answer.get('status')=='RECEIPT_WRITTEN'
 records.append({'op':op,'args':kw,'nonce':nonce,'ok':okay,'elapsed_seconds':round(time.perf_counter()-start,3)})
 print('OP',op,'OK' if okay else 'FAIL',kw.get('slot',kw.get('index','')),round(time.perf_counter()-start,2),flush=True)
 if not okay and not optional:raise RuntimeError(op+' '+str(answer)[:650])
 return answer,nonce
def morphs():
 f=[.5]*52;b=[.5]*44
 for i,v in {0:.54,3:.53,4:.51,8:.51,9:.47,14:.44,15:.41,16:.47,19:.53,20:.54,25:.57,26:.54,27:.48,28:.48,30:.50,31:.54,33:.46,34:.53,35:.49,41:.49,42:.51,44:.46,45:.50,46:.51}.items():f[i]=v
 for i,v in {0:.68,1:.52,2:.94,3:.67,4:.48,5:.47,6:.46,7:.52,8:.77,10:.60,17:.45,18:.46,19:.45,20:.46,21:.45,22:.49,23:.49,24:.44,25:.39,26:.48,27:.53,28:.50,29:.68,30:.53,31:.47,32:.45,33:.47,35:.44,38:.76,39:.71,40:.69,41:.56,42:.51,43:.50}.items():b[i]=v
 recipe='T06_morph_'+uuid.uuid4().hex[:16]+'.tsv'
 p=one.RUNTIME/'recipes'/recipe;p.parent.mkdir(exist_ok=True)
 p.write_text(''.join(f'{g}\t{i}\t{v:.6g}\n' for g,values in [('face',f),('body',b)] for i,v in enumerate(values)))
 return recipe
do('apply_morph_batch',recipe_file=morphs())
do('set_face_makeup',optional=True)
do('set_iris_asset',effective_id=6)
do('set_eye_iris',value=.86,v2=.50,v3=.14)
for i,item in enumerate([19,36,2,0]):
 do('set_hair',index=i,effective_id=item)
 do('set_hair_palette',index=i,value=.93,v2=.66,v3=.33)
# No default school uniform layering.
do('set_clothes_top',guid='[Ω-G16] HS2-KK Set',category=105,original_id=14110)
do('set_clothes',index=1,guid='[Ω-G16] HS2-KK Set',category=106,original_id=14112)
for idx in [2,3,4,5,6,7,8]:
 do('set_clothes',optional=True,index=idx,effective_id=0)
for slot,g,cat,idx,parent in [
 (0,'[Ω-G16] HS2-KK Set',123,14124,'a_n_headtop'),
 (1,'[Ω-G16] HS2-KK Set',122,14126,'a_n_headtop'),
 (2,'[Ω-G16] HS2-KK Set',126,14128,'a_n_waist'),
 (3,'kks.backport.clothes',128,2,'a_n_hand_L'),
 (4,'kks.backport.clothes',128,3,'a_n_hand_R')]:
 do('apply_accessory',optional=True,slot=slot,guid=g,category=cat,original_id=idx,parent=parent)
ans,nonce=do('render_views')
(o/'FIRST_STAGE_NONCE.txt').write_text(nonce)
(o/'T06_FIRST_STAGE_RECEIPT.json').write_text(json.dumps({'elapsed_sec':round(time.perf_counter()-start,2),'ops':records,'render_nonce':nonce,'character_fidelity_accepted':False},indent=2))
print('T06_DANCER_NATIVE_FIRST_RENDER',nonce,'elapsed',round(time.perf_counter()-start,2),flush=True)
