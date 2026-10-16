"""Deterministic controlled facial/hair/full body renderer for Blender/GLB parity QA.
Never edits original source project. Portrait camera, fixed Kelvin-neutral studio.
"""
import bpy,sys,argparse,pathlib,json,math,hashlib
from mathutils import Vector
argv=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
ap=argparse.ArgumentParser()
ap.add_argument('--stage',required=True)
ap.add_argument('--out',required=True)
ap.add_argument('--glb')
ap.add_argument('--max-samples',type=int,default=16)
ap.add_argument('--light-factor',type=float,default=1.0)
ap.add_argument('--exposure',type=float,default=0.0)
ap.add_argument('--view-transform',default='Standard')
args=ap.parse_args(argv)
out=pathlib.Path(args.out);out.mkdir(parents=True,exist_ok=True)
if args.glb:
 bpy.ops.wm.read_factory_settings(use_empty=True)
 bpy.ops.import_scene.gltf(filepath=args.glb)
pose_helper_shapes={bone.custom_shape for rig in bpy.data.objects if rig.type=='ARMATURE' for bone in rig.pose.bones if bone.custom_shape}
for helper in pose_helper_shapes:helper.hide_render=True
meshes=[x for x in bpy.data.objects if x.type=='MESH' and x not in pose_helper_shapes and not x.hide_render]
if not meshes:raise RuntimeError('No character mesh')
print('P0_HELPER_EXCLUDED',[(o.name,len(o.data.vertices)) for o in pose_helper_shapes],flush=True)
coords=[]
for ob in meshes:
 for corner in ob.bound_box:coords.append(ob.matrix_world@Vector(corner))
bounds={'x':[min(v.x for v in coords),max(v.x for v in coords)],
        'y':[min(v.y for v in coords),max(v.y for v in coords)],
        'z':[min(v.z for v in coords),max(v.z for v in coords)]}
height=bounds['z'][1]-bounds['z'][0]
cx=(bounds['x'][0]+bounds['x'][1])/2
cy=(bounds['y'][0]+bounds['y'][1])/2
minz,maxz=bounds['z']
# Both imported native Blender and glTF are made Z-up by Blender, but object orientation may differ.
world=bpy.data.worlds.new('P0 Neutral Studio') if not bpy.data.worlds else bpy.data.worlds[0]
bpy.context.scene.world=world
world.use_nodes=True
bg=world.node_tree.nodes.get('Background')
if bg:bg.inputs['Color'].default_value=(.13,.13,.13,1);bg.inputs['Strength'].default_value=.55
for obj in list(bpy.data.objects):
 if obj.type in ('LIGHT','CAMERA'):bpy.data.objects.remove(obj,do_unlink=True)
def light(n,loc,watts,size):
 obj=bpy.data.objects.new(n,bpy.data.lights.new(n,'AREA'));bpy.context.collection.objects.link(obj)
 obj.location=loc;obj.data.energy=watts;obj.data.shape='DISK';obj.data.size=size
 obj.rotation_euler=(Vector((cx,cy,minz+.9*height))-obj.location).to_track_quat('-Z','Y').to_euler()
 return obj
light('P0 Front Studio',(cx-.5,cy-1.25*height,minz+1.65*height),210*args.light_factor,1.4)
light('P0 Fill',(cx+.5,cy-1.6*height,minz+.7*height),120*args.light_factor,1.8)
light('P0 Rim',(cx+.5,cy+.7*height,minz+1.2*height),230*args.light_factor,1.4)
cam=bpy.data.objects.new('P0 Camera',bpy.data.cameras.new('P0 Camera'))
bpy.context.collection.objects.link(cam)
scene=bpy.context.scene;scene.camera=cam
scene.render.engine='BLENDER_EEVEE'
scene.render.image_settings.file_format='PNG'
scene.render.image_settings.color_mode='RGBA'
scene.render.resolution_percentage=100
scene.render.film_transparent=False
scene.view_settings.view_transform=args.view_transform
scene.view_settings.view_settings_unused if False else None
scene.view_settings.exposure=args.exposure
scene.view_settings.look='AgX - Medium High Contrast' if args.view_transform=='AgX' else 'Medium High Contrast'
scene.render.resolution_x=650
scene.render.resolution_y=960
summary={'schema':'NW_P0_CONTROLLED_VIEWS_V1','stage':args.stage,'world_bounds_m':bounds,'measured_z_height_m':height,
         'vertices':sum(len(x.data.vertices) for x in meshes),
         'images_loaded':len([x for x in bpy.data.images if x.source=='FILE']),
         'armatures':[(x.name,len(x.data.bones)) for x in bpy.data.objects if x.type=='ARMATURE'],
         'excluded_pose_helpers':[(x.name,len(x.data.vertices)) for x in pose_helper_shapes],
         'light_parameters':{'front':210*args.light_factor,'fill':120*args.light_factor,'rim':230*args.light_factor},'exposure':args.exposure,'view_transform':args.view_transform,'shots':{},'native_visual_parity':'NOT_YET_COMPARED'}
shots={
 'full':{'at':(cx,cy,minz+height*.52),'ortho':height*1.23,'width':650,'height':960},
 'face':{'at':(cx,cy,minz+height*.88),'ortho':height*.31,'width':650,'height':650},
 'hair':{'at':(cx,cy,minz+height*.74),'ortho':height*.56,'width':650,'height':760},
}
for label,setup in shots.items():
 scene.render.resolution_x=setup['width'];scene.render.resolution_y=setup['height']
 aim=Vector(setup['at'])
 # near orthographic to avoid FOV altering perceived size
 cam.data.type='ORTHO';cam.data.ortho_scale=setup['ortho']
 cam.location=(cx,cy-height*2.0,aim.z)
 cam.rotation_euler=(aim-cam.location).to_track_quat('-Z','Y').to_euler()
 file=out/(args.stage+'_'+label+'.png')
 scene.render.filepath=str(file)
 bpy.ops.render.render(write_still=True)
 summary['shots'][label]={'path':str(file),'camera_position':list(cam.location),
                          'target':list(aim),'ortho_vertical_m':setup['ortho'],
                          'resolution':(setup['width'],setup['height'])}
 summary['shots'][label]['sha256']=hashlib.sha256(file.read_bytes()).hexdigest()
 print('P0_RENDER_SHOT',args.stage,label,file.stat().st_size,flush=True)
(out/(args.stage+'_VIEW_RECEIPT.json')).write_text(json.dumps(summary,indent=2,ensure_ascii=False))
print('P0_CONTROLLED_VIEWS='+json.dumps({k:v for k,v in summary.items() if k!='shots'},ensure_ascii=False),flush=True)
