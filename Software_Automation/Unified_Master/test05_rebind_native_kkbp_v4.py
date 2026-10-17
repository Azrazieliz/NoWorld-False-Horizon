"""Test 5 genuine native card KKBP PMX rebind to authored four-wing Blender scene.
Does not edit source PMX, source card or older .blend versions. Rig/morph count checked.
"""
import bpy,sys,pathlib,json,hashlib
ROOT=pathlib.Path(r'D:\UnrealProjects\NW_Blender_Private_Review_20261017\TEST05_CRYSTAL_SERAPH')
SRC=pathlib.Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014\Export_PMX\20261017174837_ChikaHaruno')
body=SRC/'model.pmx'
if not body.is_file() or body.stat().st_size<1_000_000:raise FileNotFoundError(body)
for o in bpy.data.objects:
 if o.name=='TEST05 | Real KKBP Skinned Character Base':
  o.hide_render=True;o.hide_viewport=True
tools=pathlib.Path(r'D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\TOOLS')
sys.path.insert(0,str(tools/'blender_mmd_tools_noWorld'))
sys.path.insert(0,str(tools/'mmd_blender_python_deps'))
import mmd_tools
try:mmd_tools.register()
except Exception as e:print('MMD_TOOLS_REGISTER_NOTE',e)
before=set(bpy.data.objects.keys())
r=bpy.ops.mmd_tools.import_model(filepath=str(body),scale=.08,
    types={'MESH','ARMATURE','MORPHS','DISPLAY'},clean_model=False,remove_doubles=False)
objects=[x for x in bpy.data.objects if x.name not in before]
original_body=[x for x in objects if x.type=='MESH']
print('NATIVE_TEST05_IMPORTED',len(original_body),'total',len(objects),flush=True)
if not original_body:raise RuntimeError('Current Test05 native body geometry import missing')
mesh=max(original_body,key=lambda x:len(x.data.vertices))
for o in objects:
 if o.type=='ARMATURE':o.scale=(1,1,1)
 if o.type=='MESH':
  o.scale=(12.45,12.45,12.45)
  o.name='T05_Actual_20261017_KKBP_'+o.name
tex_count=0
missing=[]
for m in mesh.data.materials:
 if not m:continue
 n=m.name
 # PMX tools may append numeric suffixes; recover long parent texture prefix by startswith.
 lookup=n
 source=None
 for suffix in ('_MT_CT.png','_light.png'):
  folder=SRC if suffix=='_MT_CT.png' else SRC/'pre_light'
  prefix=n+suffix if suffix=='_MT_CT.png' else n+suffix
  path=folder/prefix
  if path.is_file():source=path;break
 if source is None and n.startswith('cf_m_'):
  folder=SRC/'pre_light'
  possibilities=list(folder.glob(n+'*_light.png'))
  if possibilities:source=possibilities[0]
 if source is None:
  missing.append(n);continue
 # Assign native exported color atlas on original UVs, not generic flat colors.
 m.use_nodes=True;nt=m.node_tree;nt.nodes.clear()
 t=nt.nodes.new('ShaderNodeTexImage');t.label='Authentic Test05 Koikatsu exported texture'
 t.image=bpy.data.images.load(str(source),check_existing=True);t.image.colorspace_settings.name='sRGB'
 t.location=(-360,90)
 p=nt.nodes.new('ShaderNodeBsdfPrincipled');p.location=(-50,100)
 p.inputs['Metallic'].default_value=.0;p.inputs['Roughness'].default_value=.66
 p.inputs['Subsurface Weight'].default_value=.10 if ('body' in n or 'face' in n) else .0
 nt.links.new(t.outputs['Color'],p.inputs['Base Color'])
 nt.links.new(t.outputs['Alpha'],p.inputs['Alpha'])
 if 'eyeline' in n or 'mayuge' in n:
  try:m.surface_render_method='DITHERED'
  except Exception:pass
 out=nt.nodes.new('ShaderNodeOutputMaterial')
 out.location=(200,100);nt.links.new(p.outputs['BSDF'],out.inputs['Surface'])
 tex_count+=1
from mathutils import Vector
evalob=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
verts=[evalob.matrix_world@v.co for v in evalob.to_mesh().vertices]
height=max(v.z for v in verts)-min(v.z for v in verts)
width=max(v.x for v in verts)-min(v.x for v in verts)
evalob.to_mesh_clear()
print('ACTUAL_TEST05_BODY_HEIGHT',height,'WIDTH',width,'TEXTURE_CONNECTED',tex_count,flush=True)
if not 1<height<2.5:raise RuntimeError('Source native body incorrectly transformed again')
blend=ROOT/'TEST05_CrystallineDragonSeraph_Editable_v4_NativeKKBP.blend'
if blend.exists():raise FileExistsError(blend)
scene=bpy.context.scene
scene.render.resolution_percentage=65
scene.render.filepath=str(ROOT/'TEST05_ActualNativeBody_ArtPass_v4.png')
bpy.ops.wm.save_as_mainfile(filepath=str(blend))
receipt={'schema':'NW_TEST05_NATIVE_QA_REBIND_V1',
 'native_source_pmx':str(body),'sha256':hashlib.sha256(body.read_bytes()).hexdigest(),
 'native_src_card':'SM01_AUTOTEST_T05_CRYSTAL_SERAPH_NATIVE_20261017.png',
 'native_20261017_body_mesh':mesh.name,'native_vertex_count':len(mesh.data.vertices),
 'native_original_shape_keys':len(mesh.data.shape_keys.key_blocks) if mesh.data.shape_keys else 0,
 'actual_body_eval_height_m':height,'actual_body_eval_width_m':width,
 'connected_original_kkbp_images':tex_count,'unmatched_native_materials':missing,
 'output_blend':str(blend),'renderer_accepted':False,
 'fidelity_accepted':False,'canonical_content_modified':False}
(ROOT/'TEST05_REAL_NATIVE_REBIND_RECEIPT.json').write_text(json.dumps(receipt,indent=2))
print('TEST05_NATIVE_SCENE_SAVED',blend,flush=True)
bpy.ops.render.render(write_still=True)
print('TEST05_NATIVE_BODY_RENDERED',scene.render.filepath,flush=True)
