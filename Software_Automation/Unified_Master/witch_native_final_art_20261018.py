"""Final visual corrective pass on one true Koikatsu character; target > receipts."""
import pathlib,shutil,json
import kk_native_visual_search as s
import kk_one_session as one
OUT=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\WITCH_20261018_NATIVE_BUILD')
submit=s.client_factory();one.game_guards(stage=False);one.profile_valid()
log=[]
def do(op,args):
 try:
  _,elapsed,r=s.invoke(submit,{'op':op,'args':args},timeout=14)
  result={'op':op,'args':args,'ok':True,'seconds':elapsed}
 except Exception as exc:
  result={'op':op,'args':args,'ok':False,'error':str(exc)[:450]}
 log.append(result)
 if not result['ok']:print('CORRECTION_FAILED',op,str(args)[:150],result['error'][:150],flush=True)
 return result['ok']
def color(kind,slot,name,mat,prop,rgba):
 r,g,b,a=rgba
 return do('set_surface_color',{'group':kind,'slot':slot,'file_name':name,
  'parent':mat,'guid':prop,'value':r,'v2':g,'v3':b,'v4':a})
def screenshot(view,tag):
 n,_,_=s.invoke(submit,{'op':'render_camera_view','args':{'group':view}},timeout=16)
 img=one.RUNTIME/'camera_bank'/('SM01_CAM_'+n+'_'+view+'.png')
 dst=OUT/(tag+'_'+view+'.png')
 if dst.exists():raise FileExistsError(dst)
 shutil.copy2(img,dst)
 print('REAL_KOIKATSU_IMAGE',dst,flush=True)
 return str(dst)
def main():
 # Visually chosen fitted dark blue low-cut witch bodice and purple asymmetrical skirt.
 do('set_clothes_top',{'guid':'Kraitx.Yelan.Mod','category':105,'original_id':130732})
 # Eliminate severe hair volume artifact: original accessory scale restored.
 do('apply_accessory',{'slot':4,'guid':'kushinahairpack.mod','category':122,
   'original_id':221100,'parent':'a_n_headtop'})
 do('set_accessory_vector',{'slot':4,'group':'scale','value':1,'v2':1,'v3':1})
 for prop in ('_Color','_Color2','_Color3'):
  color('accessory',4,'Hair Base','main',prop,(.055,.32,.45,1))
 color('accessory',4,'+ Hair Clip','Clip','_Color',(.06,.08,.19,1))
 # Scalp contact: native hair accessory / hat addMove uses small position units.
 do('set_accessory_vector',{'slot':0,'group':'pos','value':0,'v2':-2.1,'v3':0})
 do('set_accessory_vector',{'slot':0,'group':'scale','value':1.68,'v2':1.36,'v3':1.48})
 # Glasses: avoid white blank lense stripe above eyebrows.
 do('apply_accessory_native',{'slot':1,'category':123,'effective_id':3,'parent':'a_n_head'})
 do('set_accessory_vector',{'slot':1,'group':'pos','value':0,'v2':-2.35,'v3':0})
 for prop in ('_Color','_Color3'):
  color('accessory',1,'o_glasses_typeF2_00','acs_m_megane04',prop,(.55,.40,.30,1))
 color('accessory',1,'o_glasses_typeF2_lens','acs_m_lens','_Color',(1,1,1,.12))
 # Dark scaled serpent appendage, lateral silhouette visible at all full body views.
 do('apply_accessory',{'slot':2,'guid':'Zeta.GenericAcc','category':126,
    'original_id':10,'parent':'a_n_waist'})
 do('set_accessory_vector',{'slot':2,'group':'rot','value':0,'v2':0,'v3':53})
 do('set_accessory_vector',{'slot':2,'group':'scale','value':1.13,'v2':1.15,'v3':1.13})
 for name,material,rgb in [('Tail','Tail',(.19,.21,.39)),
    ('Under-Scales','Under-Scales',(.15,.22,.34)),
    ('Fur-Tip','Fur-Tip',(.20,.23,.37)),
    ('Fur-Top','Fur-Top',(.18,.19,.35))]:
  for prop in ('_Color','_Color2','_Color3'):
   color('accessory',2,name,material,prop,(*rgb,1))
 # The additional burgundy sleeve/cape belongs to the game reference silhouette.
 do('apply_accessory',{'slot':7,'guid':'xm007.Nia','category':125,
     'original_id':25090,'parent':'a_n_back'})
 do('set_accessory_vector',{'slot':7,'group':'scale','value':1.15,'v2':1.20,'v3':1.1})
 photos=[screenshot('front','FINAL_ART_TRIAL'),screenshot('face','FINAL_ART_TRIAL')]
 out=OUT/'FINAL_ART_TRIAL_NATIVE_EVENTS.json'
 if out.exists():raise FileExistsError(out)
 out.write_text(json.dumps({'operations':log,'images':photos},indent=2))
if __name__=='__main__':main()
