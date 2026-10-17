"""Direct correction of actual witch Koikatsu model after choosing native visual assets."""
import json,pathlib,shutil
import kk_native_visual_search as s
import kk_one_session as one
OUT=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\WITCH_20261018_NATIVE_BUILD')
submit=s.client_factory();one.profile_valid();one.game_guards(stage=False)
rows=[]
def cmd(op,args,optional=True):
 try:
  n,sec,r=s.invoke(submit,{'op':op,'args':args},timeout=13)
  x={'op':op,'args':args,'success':True,'sec':sec,'receipt':r.get('data','')[-700:]}
 except Exception as exc:
  x={'op':op,'args':args,'success':False,'error':str(exc)[:500]}
  if not optional:raise
 rows.append(x)
 print('NATIVE',op,'OK' if x['success'] else 'FAIL',str(args)[:140],flush=True)
 return x
def cap(view):
 n,_,r=s.invoke(submit,{'op':'render_camera_view','args':{'group':view}},timeout=16)
 src=one.RUNTIME/'camera_bank'/('SM01_CAM_'+n+'_'+view+'.png')
 target=OUT/('REFINED_WITCH_'+view+'.png')
 if target.exists():raise FileExistsError(target)
 shutil.copy2(src,target)
 print('REFINED_NATIVE_RENDER',target,flush=True)
 return str(target)
def recolor(kind,slot,renderer,material,col,properties=('_Color','_Color2','_Color3','_ShadowColor')):
 prof=cmd('inspect_surface_shader_profile',{'group':kind,'slot':slot,
   'file_name':renderer,'parent':material})
 if not prof['success']:return
 import re
 found=set(re.findall(r'color_property\t([^\t\r\n]+)',prof['receipt']))
 for prop in properties:
  if prop not in found:continue
  r=cmd('set_surface_color',{'group':kind,'slot':slot,'file_name':renderer,
    'parent':material,'guid':prop,'value':col[0],'v2':col[1],'v3':col[2],'v4':1})
  if r['success'] and prop!='_ShadowColor':break
def main():
 # Original character's gown is long, asymmetric and gilded: closest actual mod geometry.
 cmd('set_clothes_top',{'guid':'com.nep.ningguangstuff','category':105,'original_id':19511508},False)
 # Material introspection of each actual loaded cloth renderer, not guessed.
 a=cmd('inspect_surface_targets',{'group':'clothing','slot':0})
 if a['success']:
  for line in a['receipt'].splitlines():
   if not line.startswith('target\t'):continue
   parts=line.split('\t');name,mat=parts[1],parts[2].split(' (Instance)')[0]
   if 'gold' in mat.lower() or 'gold' in name.lower() or 'metal' in mat.lower():
    color=(.82,.62,.38)
   else:
    color=(.22,.12,.30)
   recolor('clothing',0,name,mat,color)
 # Restore exact visually-selected dark/cyan extensions, not unrelated blonde.
 cmd('apply_accessory',{'slot':4,'guid':'kushinahairpack.mod','category':122,
    'original_id':221100,'parent':'a_n_headtop'})
 cmd('set_accessory_vector',{'slot':4,'group':'scale','value':1.0,'v2':1.92,'v3':1.0})
 recolor('accessory',4,'Hair Base','main',(.05,.32,.50),
    ('_Color','_Color2','_Color3'))
 # Preserve Mona ornate witch hat but fit to scalp rather than floating.
 cmd('set_accessory_vector',{'slot':0,'group':'pos','value':0,'v2':-.48,'v3':0})
 cmd('set_accessory_vector',{'slot':0,'group':'scale','value':1.78,'v2':1.44,'v3':1.62})
 # Old optical white goggles were unacceptable: use small oval native frames and lower.
 cmd('apply_accessory_native',{'slot':1,'effective_id':3,'category':123,'parent':'a_n_head'})
 cmd('set_accessory_vector',{'slot':1,'group':'pos','value':0,'v2':-.33,'v3':0})
 q=cmd('inspect_surface_targets',{'group':'accessory','slot':1})
 if q['success']:
  for l in q['receipt'].splitlines():
   if not l.startswith('target\t'):continue
   t=l.split('\t');recolor('accessory',1,t[1],t[2].split(' (Instance)')[0],(.64,.44,.28))
 # Tail main geometry choice: pose2 drapes behind instead of protruding rigidly.
 cmd('apply_accessory',{'slot':2,'guid':'Zeta.GenericAcc','category':126,
    'original_id':11,'parent':'a_n_waist'})
 cmd('set_accessory_vector',{'slot':2,'group':'rot','value':0,'v2':0,'v3':45})
 cmd('set_accessory_vector',{'slot':2,'group':'scale','value':1.15,'v2':1.19,'v3':1.15})
 t=cmd('inspect_surface_targets',{'group':'accessory','slot':2})
 if t['success']:
  for l in t['receipt'].splitlines():
   if not l.startswith('target\t'):continue
   q=l.split('\t')
   shade=(.15,.19,.34) if 'Scale' in q[1] or q[1]=='Tail' else (.25,.21,.35)
   recolor('accessory',2,q[1],q[2].split(' (Instance)')[0],shade)
 # Open held book belongs to character's left hand (viewer's right).
 cmd('apply_accessory',{'slot':3,'guid':'GaryuX.rinwellmod','category':129,
    'original_id':6551432,'parent':'a_n_hand_L'})
 cmd('set_accessory_vector',{'slot':3,'group':'rot','value':0,'v2':-75,'v3':-5})
 cmd('set_accessory_vector',{'slot':3,'group':'scale','value':.86,'v2':.86,'v3':.86})
 images=[cap('front'),cap('face')]
 report=OUT/'REFINED_WITCH_NATIVE_EVENTS.json'
 if report.exists():raise FileExistsError(report)
 report.write_text(json.dumps({'events':rows,'images':images,'card_saved':False},indent=2))
if __name__=='__main__':main()
