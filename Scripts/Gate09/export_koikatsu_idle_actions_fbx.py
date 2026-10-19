"""Export native Koikatsu-derived Luminous skeletal Actions as Unreal FBX interchange."""
import bpy,pathlib,hashlib,json
root=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
source=root/'G09_AuthenticIdleAndBow_Source_Review_v4.blend'
dest=root/'UnrealInterchange_20261019'
if dest.exists():raise FileExistsError('immutable interchange package already exists')
dest.mkdir()
bpy.ops.wm.open_mainfile(filepath=str(source),load_ui=False)
rig=next(x for x in bpy.data.objects if x.type=='ARMATURE' and len(x.data.bones)==975)
scene=bpy.context.scene;scene.render.fps=30
names={'f_tachi_00_02':'Idle_01_Thoughtful','f_tachi_00_09':'Idle_02_Greeting',
'f_tachi_00_19_L':'Idle_03_Expressive','f_tachi_00_18_L':'Emote_Bow_Source'}
results=[]
for source_id,target_name in names.items():
 action=bpy.data.actions.get('G09_REAL_KK_'+source_id)
 if action is None:raise RuntimeError('Native standing Action missing '+source_id)
 rig.animation_data.action=action;rig.animation_data.action_slot=action.slots[0]
 scene.frame_start=int(action.frame_range[0]);scene.frame_end=int(action.frame_range[1])
 scene.frame_set(scene.frame_start)
 bpy.ops.object.select_all(action='DESELECT')
 rig.select_set(True);bpy.context.view_layer.objects.active=rig
 f=dest/('G09_LuminousV3_'+target_name+'.fbx')
 bpy.ops.export_scene.fbx(filepath=str(f),use_selection=True,object_types={'ARMATURE'},
    add_leaf_bones=False,axis_forward='-Z',axis_up='Y',
    bake_anim=True,bake_anim_use_all_bones=True,
    bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_simplify_factor=0.0)
 if not f.is_file() or f.stat().st_size<100000:raise RuntimeError('no real FBX export')
 results.append({'source':source_id,'motion_role':target_name,'file':f.name,
  'sha256':hashlib.sha256(f.read_bytes()).hexdigest(),'bytes':f.stat().st_size,
  'fps':30,'frames':[scene.frame_start,scene.frame_end],
  'compatible_target':'Luminous V3 CLOTHED / same 975-bone imported GLB skeleton only',
  'unreal_native_import':'NOT_TESTED','art_acceptance':'SOURCE_REVIEW_NOT_APPROVED'})
 print('G09_UNREAL_INTERCHANGE',f.name,f.stat().st_size,flush=True)
receipt={'schema':'GATE09_SOURCE_MOTION_3_IDLES_1_BOW_INTERCHANGE_V1',
 'source_blend_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
 'files':results,'source_character_unchanged':True,'production_approval':False,
 'missing':['Unreal native import and skeleton retarget','Foot lock and root axis validation',
 'facial expression curves','hair and cloth secondary motion','full-color artistic review']}
(dest/'INTERCHANGE_MANIFEST.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf8')
print('G09_INTERCHANGE_DONE',len(results))
