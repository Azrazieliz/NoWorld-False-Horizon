"""Reopen persisted 4.5.10 scene; mutate GN and shader settings; import exported FBX."""
import bpy, json, os, time, traceback
ROOT=os.path.normpath(os.path.join(os.path.dirname(__file__),'..'))
OUT=os.path.join(ROOT,'VALIDATION','local')
RESULT={"blender":bpy.app.version_string,"checks":{},"status":"STARTED"}
def chk(name,cond,details=None):
    RESULT["checks"][name]={"pass":bool(cond),"details":details}
    if not cond:raise AssertionError(name+":"+str(details))
def vertices(obj):
    dep=bpy.context.evaluated_depsgraph_get()
    ev=obj.evaluated_get(dep);me=ev.to_mesh();n=len(me.vertices);ev.to_mesh_clear()
    return n
try:
    t=time.perf_counter()
    f=os.path.join(OUT,'NW_EngineeringTests.blend')
    bpy.ops.wm.open_mainfile(filepath=f)
    gn=bpy.data.objects["NW_Parametric_Pillars"]
    group=gn.modifiers["NW_EditableArchitecturalBays"].node_group
    it=next(x for x in group.interface.items_tree if x.name=="Bay Count")
    n5=vertices(gn)
    mod=gn.modifiers[0]
    mod[it.identifier]=7
    gn.update_tag();bpy.context.view_layer.update()
    n7=vertices(gn)
    chk("gn_count_change_without_rebuild",n5==40 and n7==56,{"five":n5,"seven":n7})
    mod[it.identifier]=5
    acc=bpy.data.materials["NW_Asymmetric_Copper"]
    node=next(n for n in acc.node_tree.nodes if n.type=='GROUP')
    original=float(node.inputs['Skew'].default_value)
    node.inputs['Skew'].default_value=1.7
    chk("shader_parameter_edit",abs(node.inputs['Skew'].default_value-1.7)<.001,{"before":original,"after":node.inputs['Skew'].default_value})
    node.inputs['Skew'].default_value=original
    rig=bpy.data.objects["NW_WingPrototype_Rig"]
    slot=rig.animation_data.action_slot
    chk("animation_slot_persisted",slot is not None,{"slot":slot.identifier if slot else None})
    bpy.context.scene.frame_set(7)
    r=float(rig.pose.bones['Wing_R'].rotation_euler.y)
    chk("animation_key_persisted",abs(r-.48)<.0001,r)
    gp=bpy.data.objects["NW_2D_Speedlines"].data
    chk("grease_pencil_persisted",len(gp.layers[0].frames)==4,len(gp.layers[0].frames))
    img=bpy.data.images.load(os.path.join(OUT,'NW_EngineeringPreview.png'),check_existing=False)
    chk("preview_pixels",img.size[0]==320 and img.size[1]==240,list(img.size))
    pre_arch=len([o for o in bpy.data.objects if o.get('nw_role')=='ARCH'])
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    imp=bpy.ops.import_scene.fbx(filepath=os.path.join(OUT,'NW_Architecture_Test.fbx'))
    mesh_objects=[o for o in bpy.data.objects if o.type=='MESH']
    chk("fbx_reimport_blender",len(mesh_objects)>=pre_arch,{"original_arch_count":pre_arch,"imported_mesh_count":len(mesh_objects),"operator":str(imp)})
    chk("fbx_nonempty_geometry",sum(len(o.data.polygons) for o in mesh_objects)>20,sum(len(o.data.polygons) for o in mesh_objects))
    RESULT["duration_seconds"]=round(time.perf_counter()-t,3)
    RESULT["status"]="PASS"
except Exception as exc:
    RESULT["status"]="FAIL";RESULT["traceback"]=traceback.format_exc()
finally:
    with open(os.path.join(OUT,'roundtrip_results.json'),'w') as f:json.dump(RESULT,f,indent=2)
    print("NOWORLD_ROUNDTRIP="+json.dumps(RESULT))
if RESULT["status"]!="PASS":raise SystemExit(2)
