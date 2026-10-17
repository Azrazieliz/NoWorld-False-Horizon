"""Explicit native artistic fixes from reviewed front/face renders, no save yet."""
import pathlib,json,shutil
import kk_native_visual_search as s
import kk_one_session as one
OUT=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\WITCH_20261018_NATIVE_BUILD')
submit=s.client_factory();one.profile_valid();one.game_guards(stage=False)
events=[]
def do(op,args):
 try:
  n,t,r=s.invoke(submit,{'op':op,'args':args},timeout=15)
  rec={'op':op,'args':args,'ok':True,'seconds':t}
 except Exception as exc:rec={'op':op,'args':args,'ok':False,'error':str(exc)[:500]}
 events.append(rec)
 print('FINAL_TOUCH',op,'PASS' if rec['ok'] else 'REJECT',flush=True)
 return rec['ok']
def capture(label,view):
 n,_,_=s.invoke(submit,{'op':'render_camera_view','args':{'group':view}},timeout=16)
 f=one.RUNTIME/'camera_bank'/('SM01_CAM_'+n+'_'+view+'.png')
 dst=OUT/(label+'_'+view+'.png')
 if dst.exists():raise FileExistsError(dst)
 shutil.copy2(f,dst);print('FINAL_TOUCH_IMAGE',dst,flush=True)
def main():
 # These red plastic beams were visually worse than the reference's flowing sleeves.
 do('apply_accessory_native',{'slot':7,'effective_id':0,'category':125,'parent':'a_n_back'})
 # Lower huge witch brim to actual scalp; preview before acceptance.
 do('set_accessory_vector',{'slot':0,'group':'pos','value':0,'v2':-5.0,'v3':0})
 # Native lens alpha was ignored by toon_glasses; switch to cutout with full cutoff.
 do('set_surface_shader',{'group':'accessory','slot':1,
     'file_name':'o_glasses_typeF2_lens','parent':'acs_m_lens',
     'recipe_file':'Unlit/Transparent Cutout'})
 do('set_surface_float',{'group':'accessory','slot':1,'file_name':'o_glasses_typeF2_lens',
     'parent':'acs_m_lens','guid':'_Cutoff','value':1.0})
 # Frame must overlap iris, not hover at eyebrows.
 do('set_accessory_vector',{'slot':1,'group':'pos','value':0,'v2':-8.0,'v3':0})
 # Turn open book toward camera on left hand.
 do('set_accessory_vector',{'slot':3,'group':'rot','value':0,'v2':90,'v3':0})
 capture('CONTACT_REPAIRED','front');capture('CONTACT_REPAIRED','face')
 path=OUT/'CONTACT_REPAIRED_NATIVE_OPERATIONS.json'
 if path.exists():raise FileExistsError(path)
 path.write_text(json.dumps(events,indent=2))
if __name__=='__main__':main()
