"""Measure Koikatsu native-prefab / Blender bone rest alignment, no edits."""
import bpy,numpy as np,json,pathlib,hashlib
from mathutils import Vector
native=json.loads(pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016\G09_Unity_Native_Prefab_RestBasis_20261019.json').read_text(encoding='utf8'))['joints']
src=r'D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011\BLENDER_FRESH\CLOTHED\LUMINOUS_V3_CLOTHED_skinned.glb'
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.gltf(filepath=src)
rig=next(o for o in bpy.data.objects if o.type=='ARMATURE' and len(o.data.bones)==975)
alias={'cf_j_hips':'Center','cf_j_waist01':'Hips','cf_j_spine01':'Spine','cf_j_spine02':'Chest',
'cf_j_spine03':'Upper Chest','cf_j_neck':'Neck','cf_j_head':'Head','cf_j_arm00_L':'Left arm',
'cf_j_arm00_R':'Right arm','cf_j_forearm01_L':'Left elbow','cf_j_forearm01_R':'Right elbow',
'cf_j_hand_L':'Left wrist','cf_j_hand_R':'Right wrist','cf_j_thigh00_L':'Left leg',
'cf_j_thigh00_R':'Right leg','cf_j_leg01_L':'Left knee','cf_j_leg01_R':'Right knee',
'cf_j_foot_L':'Left ankle','cf_j_foot_R':'Right ankle'}
pts=[]
for path,info in native.items():
 name=alias.get(info['native_source_leaf_name'])
 if not name or name not in rig.data.bones:continue
 a=np.array(info['relative_prefab_world_rest'])[:3,3]
 b=np.array(rig.data.bones[name].matrix_local.translation)
 pts.append((name,a,b))
X=np.array([a for _,a,b in pts]);Y=np.array([b for _,a,b in pts])
xc=X.mean(axis=0);yc=Y.mean(axis=0)
cov=(X-xc).T@(Y-yc);u,s,vt=np.linalg.svd(cov);C=vt.T@u.T
fit=(X-xc)@C.T
scale=float(np.sum(fit*(Y-yc))/np.sum(fit*fit))
error=float(np.sqrt(np.mean(np.sum((fit*scale-(Y-yc))**2,axis=1))))
print('G09_NATIVE_COORD_FIT',{'pairs':len(pts),'source_to_blender_basis':C.tolist(),
  'axis_det':float(np.linalg.det(C)),'scale':scale,'rms_m':error},flush=True)
