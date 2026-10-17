"""Generate a lean 19-step reusable KK native identity base recipe."""
import pathlib,json
root=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST05_CRYSTAL_SERAPH')
root.mkdir(parents=True,exist_ok=True)
face=[.5]*52
body=[.5]*44
for i,v in {0:.46,3:.42,4:.41,9:.42,14:.28,15:.26,16:.35,19:.42,20:.58,25:.48,26:.48,27:.51,30:.53,31:.60,33:.37,34:.63,35:.49,44:.46,45:.54}.items():face[i]=v
for i,v in {0:.66,1:.48,2:.86,3:.57,4:.48,5:.47,8:.68,10:.57,17:.44,18:.46,20:.48,24:.46,25:.42,29:.73,30:.52,31:.47,32:.48,33:.46,35:.46,38:.71,39:.71,40:.67,41:.58,42:.51,43:.48}.items():body[i]=v
morph=[{'group':g,'index':i,'value':v} for g,vals in [('face',face),('body',body)] for i,v in enumerate(vals)]
def op(name,**args):return {'op':name,'args':args}
recipe=[op('apply_morph_batch',morphs=morph),
        op('set_face_makeup'),
        op('set_iris_asset',effective_id=6),
        op('set_eye_iris',value=.75,v2=.58,v3=.26)]
for ix,id in enumerate([19,36,2,0]):
 recipe.append(op('set_hair',index=ix,effective_id=id))
 recipe.append(op('set_hair_palette',index=ix,value=.31,v2=.16,v3=.25))
for ix in [1,2,3,4,5,6,7,8]:recipe.append(op('set_clothes',index=ix,effective_id=0))
recipe.append(op('render_views'))
(root/'TEST05_KOIKATSU_BASE_RECIPE.json').write_text(json.dumps(recipe,indent=2))
print('TEST05_KK_BASE_STEPS',len(recipe))
