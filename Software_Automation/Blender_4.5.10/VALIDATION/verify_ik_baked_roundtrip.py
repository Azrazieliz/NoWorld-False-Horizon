"""Reopen baked deform-only source, import armature-only FBX, compare evaluated foot world coordinates."""
import bpy,json,os,math,time
ROOT=os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
OUT=os.path.join(ROOT,"VALIDATION","phase2")
start=time.perf_counter()
src=os.path.join(OUT,"NW_ArticulatedMount_IK_Baked.blend")
bpy.ops.wm.open_mainfile(filepath=src)
samples=[1,8,12,16,24,28,36,40,48]
def positions(rig):
 out={}
 for f in samples:
  bpy.context.scene.frame_set(f);bpy.context.view_layer.update()
  out[f]=[float(x) for x in rig.matrix_world @ rig.pose.bones["FrontL_Lower"].tail]
 return out
original=positions(bpy.data.objects["NW_Skyrunner_Mount_Rig"])
bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=os.path.join(OUT,"NW_ContactIK_Baked_AnimOnly.fbx"))
rig=next(o for o in bpy.data.objects if o.type=="ARMATURE")
if not (rig.animation_data and rig.animation_data.action):
 cand=next(a for a in bpy.data.actions if "Skyrunner_Mount_Rig" in a.name)
 rig.animation_data_create();rig.animation_data.action=cand
 if cand.slots:rig.animation_data.action_slot=cand.slots[0]
imported=positions(rig)
diffs={str(f):round(math.dist(original[f],imported[f]),6) for f in samples}
r={"frames":samples,"skeleton_bones":len(rig.data.bones),
   "has_animation":bool(rig.animation_data and rig.animation_data.action),
   "distance_differences":diffs,"max_error":max(diffs.values()),
   "under_1cm":max(diffs.values())<.01,"seconds":round(time.perf_counter()-start,3)}
with open(os.path.join(OUT,"contact_ik_baked_roundtrip.json"),"w") as f:json.dump(r,f,indent=2)
print("NW_IK_BAKED_ROUNDTRIP="+json.dumps(r))
