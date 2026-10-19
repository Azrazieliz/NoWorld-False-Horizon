"""Turn the 287-editable-component Sovereign Eclipse Blender source into 4-6 material-preserving UE static mesh groups.
Do not overwrite approved source. Source remains fully component-editable.
"""
import bpy,pathlib,hashlib,json,collections
P=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
source=P/'G09_SovereignEclipse_Architectural_SOURCE_v6.blend'
out=P/'G09_SovereignEclipse_Architectural_UE_GROUPED_v6.glb'
if out.exists():raise FileExistsError(out)
bpy.ops.wm.open_mainfile(filepath=str(source),load_ui=False)
meshes=[o for o in bpy.data.objects if o.type=='MESH']
assert len(meshes)==287
groups=collections.defaultdict(list)
for o in meshes:
 m=o.data.materials[0].name if o.data.materials else ''
 if 'SovereignBoundary' in m or o.name.startswith('Boundary_'):cat='Boundary'
 elif 'SovereignNight' in m:cat='Night'
 elif 'SovereignDay' in m:cat='Day'
 elif 'SovereignSteel' in m:cat='Steel'
 else:cat='Other'
 groups[cat].append(o)
joined=[]
for cat in sorted(groups):
 bpy.ops.object.select_all(action='DESELECT')
 for o in groups[cat]:o.select_set(True)
 bpy.context.view_layer.objects.active=groups[cat][0]
 bpy.ops.object.join()
 m=groups[cat][0];m.name='FX_SovereignEclipse_'+cat+'_EditableGroup'
 joined.append(m)
 print('UE_STATIC_MESH_GROUP',cat,len(groups[cat]),len(m.data.vertices),flush=True)
assert 4<=len(joined)<=6
bpy.ops.object.select_all(action='DESELECT')
for m in joined:m.select_set(True)
bpy.context.view_layer.objects.active=joined[0]
bpy.ops.export_scene.gltf(filepath=str(out),export_format='GLB',use_selection=True,
 export_yup=True,export_apply=False)
assert out.stat().st_size>200000
print('UE_GROUPED_SOVEREIGN_GLTF',out,len(joined),out.stat().st_size,
 hashlib.sha256(out.read_bytes()).hexdigest(),flush=True)
