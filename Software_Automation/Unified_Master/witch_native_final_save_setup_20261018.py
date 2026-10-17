"""Final appearance correction, actual native character save follows separately."""
import pathlib,shutil,json,uuid
from PIL import Image
import kk_native_visual_search as s
import kk_one_session as one
OUT=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\WITCH_20261018_NATIVE_BUILD')
submit=s.client_factory();one.game_guards(stage=False)
events=[]
def edit(op,args):
 try:
  _,sec,r=s.invoke(submit,{'op':op,'args':args},timeout=15)
  x={'op':op,'args':args,'ok':True,'sec':sec}
 except Exception as exc:x={'op':op,'args':args,'ok':False,'error':str(exc)[:400]}
 events.append(x)
 print('VISUAL_FINALIZATION',op,'OK' if x['ok'] else x['error'][:130],flush=True)
 return x['ok']
def capture(view):
 n,_,_=s.invoke(submit,{'op':'render_camera_view','args':{'group':view}},timeout=17)
 src=one.RUNTIME/'camera_bank'/('SM01_CAM_'+n+'_'+view+'.png')
 out=OUT/('FINAL_CANDIDATE_'+view+'.png')
 if out.exists():raise FileExistsError(out)
 shutil.copy2(src,out)
 print('FINAL_NATIVE_CANDIDATE',out,flush=True)
def main():
 # Hide white accessory-only hair and inappropriate white bow from source-specific preset.
 edit('apply_accessory_native',{'slot':4,'effective_id':0,'category':122,'parent':'a_n_headtop'})
 # Make wide-brim witch hat TOUCH the scalp (previously floated).
 edit('set_accessory_vector',{'slot':0,'group':'pos','value':0,'v2':-8.2,'v3':0})
 # MaterialEditor persistent transparent lens avoids full-white opaque cartoon goggles.
 transparent=Image.new('RGBA',(128,128),(255,255,255,0))
 surface=one.RUNTIME/'surface_edits'/('witch_optical_transparent_'+uuid.uuid4().hex[:7]+'.png')
 surface.parent.mkdir(parents=True,exist_ok=True)
 transparent.save(surface)
 edit('apply_surface_texture',{'group':'accessory','slot':1,
    'file_name':'o_glasses_typeF2_lens','parent':'acs_m_lens','recipe_file':surface.name})
 for view in ('front','face'):
  capture(view)
 rec=OUT/'FINAL_CORRECTIONS.json'
 if rec.exists():raise FileExistsError(rec)
 rec.write_text(json.dumps(events,indent=2))
if __name__=='__main__':main()
