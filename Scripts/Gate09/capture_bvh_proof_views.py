"""Render actual skinned animated Blender source at fixed diagnostic views; private QA only."""
import bpy,sys,pathlib,hashlib,json,math
from mathutils import Vector
root=pathlib.Path(sys.argv[sys.argv.index('--')+1])
source=root/'G09_Luminous_CMU144_20_Retarget_PROOF.blend'
receipt=root/'G09_Luminous_CMU144_20_Retarget_PROOF.json'
bpy.ops.wm.open_mainfile(filepath=str(source),load_ui=False)
scene=bpy.context.scene;scene.render.engine='BLENDER_WORKBENCH'
scene.render.resolution_x=540;scene.render.resolution_y=810;scene.render.resolution_percentage=100
scene.display.shading.light='STUDIO';scene.display.shading.color_type='MATERIAL'
scene.display.shading.show_cavity=True
scene.render.image_settings.file_format='PNG'
meshes=[o for o in bpy.data.objects if o.type=='MESH' and not o.hide_render]
if not meshes:raise RuntimeError('no meshes')
pts=[o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
lo=Vector([min(v[i] for v in pts) for i in range(3)])
hi=Vector([max(v[i] for v in pts) for i in range(3)])
center=(lo+hi)/2;h=max((hi-lo).length,1.5)
cam=bpy.data.objects.new('G09_ThreeAngleQA_Camera',bpy.data.cameras.new('G09_ThreeAngleQA'))
bpy.context.collection.objects.link(cam);scene.camera=cam
cam.data.type='ORTHO';cam.data.ortho_scale=max((hi-lo).z*1.3,1.8)
frame=(scene.frame_start+scene.frame_end)//2
shots=[]
for label,offset in [('front',(0,-1,0.12)),('side',(1,0,0.12)),('diagonal',(0.65,-0.85,0.24))]:
 p=root/('G09_CMU144_20_'+label+'_frame'+str(frame)+'.png')
 if p.exists():raise FileExistsError(p)
 cam.location=center+Vector(offset).normalized()*max(h*2.2,3.0)
 cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
 scene.frame_set(frame);scene.render.filepath=str(p)
 bpy.ops.render.render(write_still=True)
 shots.append({'name':p.name,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':p.stat().st_size,'frame':frame})
out=root/'G09_CMU144_20_Captures.json'
if out.exists():raise FileExistsError(out)
out.write_text(json.dumps({'source_blend_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'shots':shots,
 'render':'Blender Workbench studio material-color diagnostic; not native Koikatsu material parity',
 'creative_accepted':False},indent=2)+'\n',encoding='utf-8')
print('GATE09_VISUAL_CAPTURES='+json.dumps({'frame':frame,'count':len(shots),'files':[x['name'] for x in shots]}))
