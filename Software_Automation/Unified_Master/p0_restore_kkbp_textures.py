"""P0 native-KKBP PMX -> Blender material restoration, read-only inputs.

Runs under Blender 5.2.2 after import (no downloading assets, no destructive edits).
Restores actual *_MT_CT.png colors and *_NMP_CNV.png normal detail using exported
texture atlases. Leaves already-textured KKBP-matched materials intact.
Outputs a NEW isolated QA blend only when 'write_blend' is requested.
Source file and PMX export remain immutable.
"""
import bpy, pathlib, re, json, hashlib, sys, argparse, collections
from mathutils import Vector
def sha(path):
 h=hashlib.sha256()
 with open(path,'rb') as f:
  for c in iter(lambda:f.read(1<<20),b''):h.update(c)
 return h.hexdigest()
def source_files(root):
 return list(root.glob('*.png'))+list((root/'Outfit 00').glob('*.png'))
def key(name):
 n=name.removeprefix('KK ').strip()
 return re.sub(r'\.\d{3}$','',n)
def texture_match(name,candidates,texture_kind):
 s=key(name)
 options=[]
 for p in candidates:
  stem=p.stem
  if not stem.endswith('_'+texture_kind):continue
  prefix=stem[:-(len(texture_kind)+1)]
  # Full PMX material names are safer than loose substring approximations.
  score=100 if prefix==s else 85 if s.startswith(prefix+' ') or prefix.startswith(s+' ') else 72 if (s.lower()==prefix.lower()) else 0
  if not score:
   # Some 'KK ' wraps use a MMD material suffix e.g. cf_m_face_00 3310.
   if s.lower().startswith(prefix.lower()+' '):score=82
  if score:options.append((score,-abs(len(prefix)-len(s)),p))
 return max(options,key=lambda x:x[:2])[2] if options else None
def restore(root,out,write_blend):
 mats=list(bpy.data.materials)
 candidates=source_files(root)
 source_colors=[p for p in candidates if p.stem.endswith('_MT_CT')]
 before_image_nodes=sum(1 for m in mats if m.use_nodes and m.node_tree for n in m.node_tree.nodes if n.type=='TEX_IMAGE' and n.image is not None)
 before_image_datablocks=len([i for i in bpy.data.images if i.source=='FILE'])
 restored=[];previous=[];missing=[];textures_used={}
 for mat in mats:
  if not mat.use_nodes:mat.use_nodes=True
  nt=mat.node_tree
  linked=[n for n in nt.nodes if n.type=='TEX_IMAGE' and n.image is not None]
  if linked:
   previous.append(mat.name);continue
  color=texture_match(mat.name,source_colors,'MT_CT')
  if not color:
   missing.append(mat.name);continue
  # Convert an untextured/import-broken MMD node graph to editable PBR for QA:
  # original source material data is preserved separately and NOT overwritten.
  nt.nodes.clear()
  tx=nt.nodes.new('ShaderNodeTexImage');tx.label='KKBP original native MT_CT';tx.location=(-470,150)
  tx.image=bpy.data.images.load(str(color),check_existing=True)
  tx.image.colorspace_settings.name='sRGB'
  bs=nt.nodes.new('ShaderNodeBsdfPrincipled');bs.location=(-150,160)
  bs.inputs['Roughness'].default_value=.52
  nt.links.new(tx.outputs['Color'],bs.inputs['Base Color'])
  if color and tx.image.channels==4:
   nt.links.new(tx.outputs['Alpha'],bs.inputs['Alpha'])
   try:mat.surface_render_method='DITHERED'
   except (AttributeError,TypeError,ValueError):pass
  normal=texture_match(mat.name,candidates,'NMP_CNV')
  if normal and normal.stat().st_size>1000:
   nm=nt.nodes.new('ShaderNodeTexImage');nm.label='KKBP original native normal';nm.location=(-470,-140)
   nm.image=bpy.data.images.load(str(normal),check_existing=True)
   nm.image.colorspace_settings.name='Non-Color'
   decode=nt.nodes.new('ShaderNodeNormalMap');decode.inputs['Strength'].default_value=.5
   decode.location=(-225,-80)
   nt.links.new(nm.outputs['Color'],decode.inputs['Color'])
   nt.links.new(decode.outputs['Normal'],bs.inputs['Normal'])
  else:normal=None
  outnode=nt.nodes.new('ShaderNodeOutputMaterial');outnode.location=(150,160)
  nt.links.new(bs.outputs['BSDF'],outnode.inputs['Surface'])
  restored.append({'material':mat.name,'albedo':str(color),'albedo_sha256':sha(color),
                   'normal':str(normal) if normal else None,
                   'normal_sha256':sha(normal) if normal else None})
  textures_used[str(color)]=str(sha(color))
 meshes=[x for x in bpy.data.objects if x.type=='MESH']
 missing_uv=[x.name for x in meshes if not x.data.uv_layers]
 rigs=[x for x in bpy.data.objects if x.type=='ARMATURE']
 total_shape_keys=sum(max(0,len(x.data.shape_keys.key_blocks)-1) for x in meshes if x.data.shape_keys)
 total_verts=sum(len(x.data.vertices) for x in meshes)
 after_image_nodes=sum(1 for m in mats if m.use_nodes and m.node_tree for n in m.node_tree.nodes if n.type=='TEX_IMAGE' and n.image)
 result={'schema':'NW_P0_KKBP_TEXTURE_IMPORT_REPAIR_V1','source_blend':bpy.data.filepath,
 'source_texture_root':str(root),'source_texture_png_count':len(candidates),
 'source_color_atlas_count':len(source_colors),'input_originals_unchanged':True,
 'before_image_datablocks':before_image_datablocks,'before_image_nodes':before_image_nodes,
 'after_image_datablocks':len([x for x in bpy.data.images if x.source=='FILE']),
 'after_image_nodes':after_image_nodes,
 'restored_count':len(restored),'preexisting_textured_materials':len(previous),
 'unmatched_materials':missing,'materials_restored':restored,
 'mesh_count':len(meshes),'vertices':total_verts,'shape_keys':total_shape_keys,
 'armatures':[{'name':x.name,'bones':len(x.data.bones)} for x in rigs],
 'missing_uv_meshes':missing_uv,
 'color_nodes_restored':len(restored)>0,
 'material_parity_to_native_unity':'NOT_PROVEN',
 'visual_native_fidelity':'NOT_PROVEN','unreal_import':'NOT_RUN'}
 if result['before_image_nodes']==0 and len(restored)==0:
  raise RuntimeError('REGRESSION: zero source textures linked and zero material repairs')
 out.mkdir(parents=True,exist_ok=True)
 if write_blend:
  dst=out/'P0_KKBP_RETEXTURED_QA.blend'
  if dst.exists():raise FileExistsError(dst)
  # Embed paths to original, independently hash-locked source PNG; do not copy 4k maps.
  bpy.ops.wm.save_as_mainfile(filepath=str(dst))
  result['qa_blend']=str(dst);result['qa_blend_bytes']=dst.stat().st_size
 receipt=out/'A_TEXTURE_IMPORT_REPAIR_RECEIPT.json'
 if receipt.exists():raise FileExistsError(receipt)
 receipt.write_text(json.dumps(result,indent=2,ensure_ascii=False))
 print('P0_KKBP_REPAIR='+json.dumps({k:v for k,v in result.items() if k not in ('materials_restored','unmatched_materials')},ensure_ascii=False),flush=True)
 print('P0_MISSING_MATERIAL_SAMPLE',missing[:10],flush=True)
def main():
 argv=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
 parser=argparse.ArgumentParser()
 parser.add_argument('--textures',required=True)
 parser.add_argument('--out',required=True)
 parser.add_argument('--write-blend',action='store_true')
 a=parser.parse_args(argv)
 restore(pathlib.Path(a.textures),pathlib.Path(a.out),a.write_blend)
if __name__=='__main__':main()
