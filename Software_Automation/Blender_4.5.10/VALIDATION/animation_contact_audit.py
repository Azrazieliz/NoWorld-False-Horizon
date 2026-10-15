"""Read-only motion evaluator for contact, grounding and continuity on any Blender armature.
The metrics assess data; they do not replace animator quality review.
"""
import bpy,math,json,os
from mathutils import Vector
ROOT=os.path.normpath(os.path.join(os.path.dirname(__file__),".."))
OUT=os.path.join(ROOT,"VALIDATION","phase2")
bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT,"NW_ArticulatedMount.blend"))
rig=bpy.data.objects["NW_Skyrunner_Mount_Rig"]
legs=[x for x in rig.pose.bones if x.name.endswith("_Lower")]
frames=range(1,49)
all_metrics=[]
for pb in legs:
 coords=[]
 for frame in frames:
  bpy.context.scene.frame_set(frame)
  # PoseBone.tail is in armature/object space and respects evaluated pose
  pos=rig.matrix_world @ rig.pose.bones[pb.name].tail
  coords.append((pos.x,pos.y,pos.z))
 zmin=min(p[2] for p in coords)
 threshold=zmin+.08
 grounded=[i for i,p in enumerate(coords) if p[2]<=threshold]
 step_delta=[(Vector(coords[i+1])-Vector(coords[i])).length for i in range(len(coords)-1)]
 contact_deltas=[math.dist(coords[i][:2],coords[i+1][:2]) for i in grounded if i+1<len(coords) and i+1 in grounded]
 # Vertical foot floor heuristic is relative to lowest observed pose.
 all_metrics.append({"bone":pb.name,"zmin":round(zmin,4),"zmax":round(max(p[2] for p in coords),4),
   "root_motion_mode":"IN_PLACE",
   "grounded_samples":len(grounded),"consecutive_grounded_steps":len(contact_deltas),
   "max_grounded_xy_translation_per_frame":round(max(contact_deltas,default=0),5),
   "mean_grounded_xy_translation_per_frame":round(sum(contact_deltas)/len(contact_deltas),5) if contact_deltas else None,
   "max_total_step_translation":round(max(step_delta),5)})
results={"blender":bpy.app.version_string,"frames":48,"fps":24,
 "method":"relative lowest point foot-contact heuristic; manually evaluate actual soles, collision, slip and physical plausibility before approval",
 "legs":all_metrics,"contact_quality_approved":False,
 "motion_pipeline_status":"MEASURED_NOT_REPAIRED",
 "source_modified":False}
with open(os.path.join(OUT,"mount_contact_metrics.json"),"w") as f:json.dump(results,f,indent=2)
print("NOWORLD_CONTACT="+json.dumps(results))
