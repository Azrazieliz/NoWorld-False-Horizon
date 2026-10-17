"""Offline CPU image score regression; no game and no character modification."""
import tempfile,pathlib,json
from PIL import Image,ImageDraw
import kk_visual_fit as kk
results={}
def test(name,func):
 try:func();results[name]="PASS"
 except Exception as e:results[name]="FAIL "+str(e)
def check(c,m):
 if not c:raise AssertionError(m)
def error_for(a,b,roi=None):
 try:kk.evaluate(a,b,reference_roi=roi)
 except Exception as e:return str(e)
 return ""
def make(p,robe=(80,25,43),hair=(230,168,85),face_offset=0,
         wider=False,horns=False,bg="white",size=(512,768)):
 im=Image.new('RGB',size,bg);d=ImageDraw.Draw(im);s=size[0]/512
 def q(p):return tuple(round(v*s) for v in p)
 d.polygon([q((184,225)),q((330 if wider else 299,222)),
            q((365 if wider else 355,702)),q((105,702))],fill=robe)
 d.ellipse(q((135,110,340,340)),fill=hair)
 d.ellipse(q((171,157,320,312)),fill=(239,201,177))
 d.polygon([q((137,175)),q((184,65)),q((328,120)),q((285,239)),
            q((240,187)),q((190,234))],fill=hair)
 for x in (208,279):
  d.ellipse(q((x+face_offset,226,x+22+face_offset,236)),fill=(83,41,47))
  d.ellipse(q((x+7+face_offset,225,x+17+face_offset,234)),fill=(185,129,44))
 if horns:
  d.polygon([q((154,170)),q((132,9)),q((189,98))],fill=(45,42,45))
  d.polygon([q((298,170)),q((335,5)),q((334,162))],fill=(45,42,45))
 d.line([q((165,420)),q((265,650))],fill=(203,131,60),width=max(1,round(8*s)))
 im.save(p)
with tempfile.TemporaryDirectory() as td:
 r=pathlib.Path(td);ref=r/'ref.png';same=r/'same.png';color=r/'color.png'
 wider=r/'wider.png';horns=r/'horns.png';face=r/'face.png';dark=r/'dark.png';tiny=r/'tiny.png'
 make(ref);make(same);make(color,robe=(24,114,171));make(wider,wider=True)
 make(horns,horns=True);make(face,face_offset=19);make(dark,bg='navy');make(tiny,size=(116,206))
 scores={}
 for name,p in [('same',same),('color',color),('wider',wider),('horns',horns),('face',face)]:
  scores[name]=kk.evaluate(ref,p)['score_0_to_100']
 test('self_similarity_above_99',lambda:check(scores['same']>99,'Identity score wrong'))
 test('wrong_robe_penalty',lambda:check(scores['same']>scores['color']+4,'Color failure missed'))
 test('wrong_outline_penalty',lambda:check(scores['same']>scores['wider']+.3,'Geometry failure missed'))
 test('horn_structure_penalty',lambda:check(scores['same']>scores['horns']+.3,'Headpiece failure missed'))
 test('face_geometry_penalty',lambda:check(scores['same']>scores['face']+.1,'Facial change missed'))
 test('reject_maker_background',lambda:check('Uncontrolled background' in error_for(ref,dark),'Background not rejected'))
 test('low_resolution_warning',lambda:check(
  'REFERENCE_IS_LOW_RESOLUTION_NO_FIDELITY_ACCEPTANCE' in kk.evaluate(tiny,same)['warnings'],'Low resolution accepted'))
 test('no_visual_approval_from_metric',lambda:check(kk.evaluate(ref,same)['visual_identity_accepted'] is False,'Auto acceptance'))
 test('structural_edges_detect_horns',lambda:check(
   kk.evaluate(ref,horns)['components']['edge_structure']<kk.evaluate(ref,same)['components']['edge_structure'],'Missing structural check'))
 test('invalid_roi_fails_closed',lambda:check('Bad image region' in error_for(ref,same,(-1,0,30,30)),'Bad ROI not rejected'))
 test('same_scene_measured_delta_zero',lambda:check(
   kk.measure_visible_change(ref,same)['changed_subject_fraction']==0,'Same image changed'))
 test('major_robe_material_change_visible',lambda:check(
   kk.measure_visible_change(ref,color)['changed_subject_fraction']>.05,'Major clothing color change missed'))
 test('known_noise_floor_rejects_non_improvement',lambda:check(
   kk.measure_visible_change(ref,color,noise_floor=.90)['measurably_changed'] is False,
   'Near-maximal noise not rejected'))
 print('KK_VISUAL_REGRESSIONS='+json.dumps({'passed':sum(v=='PASS' for v in results.values()),
 'total':len(results),'tests':results,'scores':scores,'native_game_edited':False}))
 if any(v!='PASS' for v in results.values()):raise SystemExit(2)
