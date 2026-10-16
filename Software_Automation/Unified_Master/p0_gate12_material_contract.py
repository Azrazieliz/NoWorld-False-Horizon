"""Source-driven Gate12 Unreal material contract, zero producer changes."""
import json,pathlib,hashlib,struct
OUT=pathlib.Path(r'D:\UnrealProjects\NW_SoftwareMaster_P0_20261016')
base=json.loads((OUT/'P0_NATIVE_GLB_EYELINE_ALPHA_AUDIT.json').read_text())
emb=json.loads((OUT/'P0_NATIVE_GLB_EYELINE_EMBEDDED_COMPARISON.json').read_text())
gltfs=json.loads((OUT/'A_LUMINOUS_GLB_STAGE_AUDIT.json').read_text())
rules=[]
for state in ('CLOTHED','UNDERWEAR','NUDE'):
 for m in base[state]['glb_eye_face_materials']:
  name=m['name'];orig=m['alpha_mode']
  if 'eyeline' not in name.lower():continue
  a=emb[state].get(name)
  if not a:raise RuntimeError('Missing embedded alpha for '+name)
  p=a['partial_alpha_fraction']
  if orig!='BLEND':raise RuntimeError('Original GLB eyeline did not declare BLEND')
  rules.append({'state':state,'material':name,'source_glb_alpha_mode':orig,'source_partial_alpha_fraction':p,
   'source_embedded_texture':a['embedded_image'],'source_embedded_texture_sha256':a['embedded_sha256'],
   'expected_unreal_blend_mode':'BLEND_Translucent',
   'opacity_source':'TextureSample.A' if p>0 else 'SourceAlphaZeroRespectVisibility',
   'alpha_gradient_must_survive':p>0,
   'must_not_use_unreal_opaque':True,
   'separate_material_slot_no_merging':True,
   'alpha_threshold_zero_does_not_replace_translucent_blending':True,
   'native_visual_validation_required':True})
contract={'schema':'NW_P0_GATE12_UNREAL_SOURCE_SEMANTICS_V1','status':'SOURCE_DRIVEN_NOT_NATIVE_UNREAL_ACCEPTED',
 'source_revision':'Archived Luminous Base V3; original hash-locked cards and GLBs',
 'source_glb_hashes':{s:gltfs[s]['sha256'] if 'sha256' in gltfs[s] else gltfs[s].get('file') for s in ('CLOTHED','UNDERWEAR','NUDE')},
 'material_contracts':rules,
 'global_material_requirements':[
  'Preserve original character material slots; no flattening of face/iris/upper-lower eyeline/hair into a single opaque material',
  'Retain source alpha gradients using the actual embedded GLB texture alpha. Upper eyeline must be translucent if its source mode is BLEND.',
  'Preserve eye color/albedo, eye highlight, normal-map normal space, hair two-sided alpha and texture UV/channel masks separately.',
  'Source alpha BLEND does not imply every channel is nonzero; do not reinterpret a fully transparent source as opaque.',
  'Match native Koikatsu shading with identical pose, camera, color management and light placement; record independent visual acceptance.',
  'Morph import must report effective name/index/nonzero delta on actual target UE component, not just a list of names.',
  'World dimensions, bounds and camera framing are separate tests; no camera changes can compensate missing shader opacity or morph deltas.'
 ],
 'production_default_changed':False,
 'unreal_native_execution':'NOT_RUN_IN_SOFTWARE_MASTERY',
 'gate12_own_native_material':True}
out=OUT/'P0_GATE12_MATERIAL_AND_EFFECTIVE_MORPH_CONTRACT.json'
out.write_text(json.dumps(contract,indent=2))
assert sum(x['alpha_gradient_must_survive'] for x in rules)==3
print('CONTRACT',out,'eyeline_cases',len(rules),'upper_with_gradient',sum(x['alpha_gradient_must_survive'] for x in rules))
