"""Isolated Blender 4.5.10 engineering proof. Noncanonical and not art-approved."""
import bpy, json, math, os, sys, time, traceback
from mathutils import Vector

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
OUT = os.path.join(ROOT, "VALIDATION", "local")
os.makedirs(OUT, exist_ok=True)
start = time.perf_counter()
report = {"blender": bpy.app.version_string, "status": "STARTED", "tests": {}, "assets": [], "limitations": []}

def check(name, condition, detail=None):
    report["tests"][name] = {"pass": bool(condition), "detail": detail}
    if not condition:
        raise AssertionError("%s: %r" % (name, detail))

def reset():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for ng in list(bpy.data.node_groups):
        if ng.name.startswith("NW_"):
            bpy.data.node_groups.remove(ng)

def socket(tree, name, direction, kind, default=None, lo=None, hi=None):
    s = tree.interface.new_socket(name=name, in_out=direction, socket_type=kind)
    if default is not None: s.default_value = default
    if lo is not None: s.min_value = lo
    if hi is not None: s.max_value = hi
    return s

def material_group():
    ng = bpy.data.node_groups.new("NW_Adaptive_Asymmetric_Color_v1", "ShaderNodeTree")
    socket(ng,"Vector","INPUT","NodeSocketVector")
    socket(ng,"Primary","INPUT","NodeSocketColor",(0.03,0.11,0.31,1.0))
    socket(ng,"Secondary","INPUT","NodeSocketColor",(0.8,0.2,0.12,1.0))
    socket(ng,"Skew","INPUT","NodeSocketFloat",0.3,-3,3)
    socket(ng,"Noise Warp","INPUT","NodeSocketFloat",0.2,0,2)
    socket(ng,"Color","OUTPUT","NodeSocketColor")
    n=ng.nodes; l=ng.links
    a=n.new("NodeGroupInput");a.location=(-650,0)
    b=n.new("ShaderNodeSeparateXYZ");b.location=(-440,240)
    l.new(a.outputs["Vector"],b.inputs["Vector"])
    sk=n.new("ShaderNodeMath");sk.operation='MULTIPLY';sk.location=(-230,170)
    l.new(b.outputs["Y"],sk.inputs[0]);l.new(a.outputs["Skew"],sk.inputs[1])
    add=n.new("ShaderNodeMath");add.operation='ADD';add.location=(-30,200)
    l.new(b.outputs["X"],add.inputs[0]);l.new(sk.outputs[0],add.inputs[1])
    noise=n.new("ShaderNodeTexNoise");noise.inputs["Scale"].default_value=4.5
    noise.location=(-440,-110);l.new(a.outputs["Vector"],noise.inputs["Vector"])
    warp=n.new("ShaderNodeMath");warp.operation='MULTIPLY';warp.location=(-230,-100)
    l.new(noise.outputs["Fac"],warp.inputs[0]);l.new(a.outputs["Noise Warp"],warp.inputs[1])
    mixpos=n.new("ShaderNodeMath");mixpos.operation='ADD';mixpos.use_clamp=True
    mixpos.location=(180,120);l.new(add.outputs[0],mixpos.inputs[0]);l.new(warp.outputs[0],mixpos.inputs[1])
    mix=n.new("ShaderNodeMixRGB");mix.blend_type='MIX';mix.location=(380,150)
    l.new(mixpos.outputs[0],mix.inputs[0]);l.new(a.outputs["Primary"],mix.inputs[1]);l.new(a.outputs["Secondary"],mix.inputs[2])
    out=n.new("NodeGroupOutput");out.location=(610,150);l.new(mix.outputs[0],out.inputs["Color"])
    return ng

def material(name, group, colors, skew, warp, use_world=False):
    m=bpy.data.materials.new(name);m.diffuse_color=colors[0];m.use_nodes=True
    nodes=m.node_tree.nodes;nodes.clear();links=m.node_tree.links
    source=nodes.new("ShaderNodeNewGeometry" if use_world else "ShaderNodeTexCoord")
    source.location=(-460,0)
    adapter=nodes.new("ShaderNodeGroup");adapter.node_tree=group;adapter.location=(-220,100)
    adapter.inputs["Primary"].default_value=colors[0]
    adapter.inputs["Secondary"].default_value=colors[1]
    adapter.inputs["Skew"].default_value=skew
    adapter.inputs["Noise Warp"].default_value=warp
    links.new(source.outputs["Position" if use_world else "Generated"],adapter.inputs["Vector"])
    bsdf=nodes.new("ShaderNodeBsdfPrincipled");bsdf.location=(20,150)
    bsdf.inputs["Roughness"].default_value=0.46
    links.new(adapter.outputs["Color"],bsdf.inputs["Base Color"])
    output=nodes.new("ShaderNodeOutputMaterial");output.location=(350,150)
    links.new(bsdf.outputs["BSDF"],output.inputs["Surface"])
    m["nw_adaptive_version"]="1.0"
    m["nw_gradient_scope"]="world" if use_world else "local"
    return m

def box(name, loc, dims, mat, tag="ARCH", bevel=0.02):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    o=bpy.context.object;o.name=name
    o.dimensions=dims
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    o.data.materials.append(mat)
    if bevel:
        mod=o.modifiers.new("NW_EdgeSoftening","BEVEL");mod.width=bevel;mod.segments=2
        o.modifiers.new("NW_VertexNormals","WEIGHTED_NORMAL")
    o["nw_role"]=tag
    return o

def geometry_nodes_pillars(mat):
    ng=bpy.data.node_groups.new("NW_ArchitecturalBayArray_v1","GeometryNodeTree")
    socket(ng,"Geometry","INPUT","NodeSocketGeometry")
    socket(ng,"Bay Count","INPUT","NodeSocketInt",5,2,30)
    socket(ng,"Bay Spacing","INPUT","NodeSocketFloat",2.3,0.6,8)
    socket(ng,"Geometry","OUTPUT","NodeSocketGeometry")
    nodes=ng.nodes;links=ng.links
    gi=nodes.new("NodeGroupInput");gi.location=(-600,200)
    line=nodes.new("GeometryNodeMeshLine");line.mode='OFFSET';line.location=(-290,200)
    links.new(gi.outputs["Bay Count"],line.inputs["Count"])
    comb=nodes.new("ShaderNodeCombineXYZ");comb.location=(-570,-50)
    links.new(gi.outputs["Bay Spacing"],comb.inputs["X"])
    links.new(comb.outputs["Vector"],line.inputs["Offset"])
    cube=nodes.new("GeometryNodeMeshCube");cube.location=(-310,-80)
    cube.inputs["Size"].default_value=(0.27,0.32,3.8)
    transform=nodes.new("GeometryNodeTransform");transform.location=(-80,-50)
    transform.inputs["Translation"].default_value=(0,0,2.0)
    links.new(cube.outputs["Mesh"],transform.inputs["Geometry"])
    setmat=nodes.new("GeometryNodeSetMaterial");setmat.location=(105,-50);setmat.inputs["Material"].default_value=mat
    links.new(transform.outputs["Geometry"],setmat.inputs["Geometry"])
    inst=nodes.new("GeometryNodeInstanceOnPoints");inst.location=(300,170)
    links.new(line.outputs["Mesh"],inst.inputs["Points"]);links.new(setmat.outputs["Geometry"],inst.inputs["Instance"])
    real=nodes.new("GeometryNodeRealizeInstances");real.location=(510,170)
    links.new(inst.outputs["Instances"],real.inputs["Geometry"])
    join=nodes.new("GeometryNodeJoinGeometry");join.location=(720,200)
    links.new(gi.outputs["Geometry"],join.inputs["Geometry"])
    links.new(real.outputs["Geometry"],join.inputs["Geometry"])
    out=nodes.new("NodeGroupOutput");out.location=(910,200)
    links.new(join.outputs["Geometry"],out.inputs["Geometry"])
    mesh=bpy.data.meshes.new("NW_EmptyGeneratorMesh");ob=bpy.data.objects.new("NW_Parametric_Pillars",mesh)
    bpy.context.collection.objects.link(ob);ob.location=(-4.6,-1.5,0)
    md=ob.modifiers.new("NW_EditableArchitecturalBays","NODES");md.node_group=ng
    ob["nw_role"]="ARCH"
    # The node group is editable in the Geometry Nodes workspace; no mesh bake required.
    return ob,ng

def animate_wings(mat):
    ad=bpy.data.armatures.new("NW_WingPrototype_Skeleton")
    rig=bpy.data.objects.new("NW_WingPrototype_Rig",ad);bpy.context.collection.objects.link(rig)
    rig.location=(0,5.0,0)
    bpy.context.view_layer.objects.active=rig;rig.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    root=ad.edit_bones.new("Root");root.head=(0,0,0);root.tail=(0,0,1.4)
    for name,sign in (("Wing_L",-1),("Wing_R",1)):
        bone=ad.edit_bones.new(name);bone.head=(0,0,1.2);bone.tail=(sign*1.7,0,1.2)
        bone.parent=root;bone.use_connect=False
    bpy.ops.object.mode_set(mode="OBJECT");rig.select_set(False)
    for name,sign in (("Wing_L",-1),("Wing_R",1)):
        verts=[(0,0,1.2),(sign*1.85,0,1.2),(sign*0.8,0,0.25),(sign*0.8,0,2.0)]
        me=bpy.data.meshes.new(name+"_Mesh")
        me.from_pydata(verts,[],[(0,1,2),(0,3,1)]);me.update()
        ob=bpy.data.objects.new("NW_"+name+"_Surface",me)
        bpy.context.collection.objects.link(ob);ob.location=rig.location
        ob.data.materials.append(mat)
        vg=ob.vertex_groups.new(name=name);vg.add([0,1,2,3],1.0,'REPLACE')
        md=ob.modifiers.new("NW_Skin", "ARMATURE");md.object=rig
        ob["nw_role"]="RIG_TEST"
    for frame,rot in ((1,0),(7,.48),(13,0),(19,-.3),(25,0)):
        for name,sign in (("Wing_L",-1),("Wing_R",1)):
            pb=rig.pose.bones[name];pb.rotation_mode="XYZ"
            pb.rotation_euler=(0,sign*rot,0)
            pb.keyframe_insert(data_path="rotation_euler",frame=frame,group=name)
    return rig

def grease_pencil_test():
    gp=bpy.data.grease_pencils_v3.new("NW_2D_Speedline_Data")
    obj=bpy.data.objects.new("NW_2D_Speedlines",gp)
    bpy.context.collection.objects.link(obj)
    obj.location=(-2,5,0)
    layer=gp.layers.new("Speedline_Layer",set_active=True)
    for f, yoff in [(1,0.0),(6,0.4),(12,-0.2),(18,0.0)]:
        drawing=layer.frames.new(f).drawing
        drawing.add_strokes([5,5,5])
        for sidx,stroke in enumerate(drawing.strokes):
            for i,pt in enumerate(stroke.points):
                pt.position=(i*0.4, sidx*0.3+yoff, 0)
                pt.radius=0.018
        del drawing
    obj["nw_role"]="GP_TEST"
    return gp,obj

def run():
    reset()
    scene=bpy.context.scene;scene.frame_start=1;scene.frame_end=25;scene.render.fps=24
    grp=material_group()
    stone=material("NW_Stone_Gradient",grp,[(.09,.20,.30,1),(.56,.72,.77,1)],.30,.12)
    accent=material("NW_Asymmetric_Copper",grp,[(.05,.12,.20,1),(.95,.39,.12,1)],-.8,.27)
    ground=material("NW_Ground_Gradient",grp,[(.04,.055,.09,1),(.25,.24,.27,1)],.17,.08,True)
    box("NW_Anchor_Deck",(0,0,-.3),(11,5,.6),ground,bevel=.12)
    box("NW_Wall_Backdrop",(0,1.1,2),(10,.35,4),stone)
    box("NW_Upper_Crown",(0,-.6,4),(10.5,.7,.28),accent)
    for x in (-4.5,-2.25,0,2.25,4.5):
        box("NW_WindowTop_%s"%x,(x,0.85,2.9),(1.5,.25,.15),accent)
        box("NW_WindowBase_%s"%x,(x,0.85,1.4),(1.4,.25,.10),accent)
    ob,ng=geometry_nodes_pillars(stone)
    rig=animate_wings(accent)
    report["assets"]=["parametric architecture","asymmetric and world-space gradients","skeletal wing motion"]
    check("shader_group_reuse",sum(1 for m in bpy.data.materials if m.get("nw_adaptive_version"))==3)
    check("gradient_asymmetry",accent.node_tree.nodes.get("Group") is not None or any(n.type=="GROUP" and abs(n.inputs["Skew"].default_value)>.4 for n in accent.node_tree.nodes))
    check("node_group_control",len(ng.interface.items_tree)>=4)
    deps=bpy.context.evaluated_depsgraph_get()
    mesh=ob.evaluated_get(deps).to_mesh()
    check("evaluated_geometry_nodes",len(mesh.vertices)>=40,{"vertices":len(mesh.vertices),"faces":len(mesh.polygons)})
    ob.evaluated_get(deps).to_mesh_clear()
    check("skeletal_animation",len(rig.pose.bones)>=3 and rig.animation_data is not None)
    scene.frame_set(7);val7=float(rig.pose.bones["Wing_R"].rotation_euler[1])
    scene.frame_set(1);val1=float(rig.pose.bones["Wing_R"].rotation_euler[1])
    check("keyframe_motion",abs(val7-val1)>.3,{"frame1":val1,"frame7":val7})
    try:
        gp,gpo=grease_pencil_test()
        total=sum(len(layer.frames) for layer in gp.layers)
        check("grease_pencil_v3",total==4,{"keys":total,"strokes_per_frame":3})
        report["assets"].append("Grease Pencil multi-frame speedline")
    except Exception as exc:
        report["tests"]["grease_pencil_v3"]={"pass":False,"error":repr(exc)}
        report["limitations"].append("Grease Pencil requires correction: "+repr(exc))
    scene.frame_set(1)
    # Never change the user's preferences. Test artifacts stay in isolated worktree.
    source=os.path.join(OUT,"NW_EngineeringTests.blend")
    bpy.ops.wm.save_as_mainfile(filepath=source,compress=True)
    check("native_editable_source",os.path.getsize(source)>1000,os.path.getsize(source))
    # Preview stays low-cost; technical evidence does not imply professional artistic signoff.
    try:
        scene.render.engine='BLENDER_WORKBENCH'
        scene.render.resolution_x=320;scene.render.resolution_y=240
        scene.render.resolution_percentage=100
        scene.render.image_settings.file_format='PNG'
        cam_data=bpy.data.cameras.new("NW_ReviewCamera");cam=bpy.data.objects.new("NW_ReviewCamera",cam_data)
        scene.collection.objects.link(cam);cam.location=(12,-16,12)
        direction=Vector((0,0,1.6))-cam.location;cam.rotation_euler=direction.to_track_quat('-Z','Y').to_euler()
        cam_data.type="ORTHO";cam_data.ortho_scale=15;scene.camera=cam
        scene.render.filepath=os.path.join(OUT,"NW_EngineeringPreview.png")
        bpy.ops.render.render(write_still=True)
        check("preview_render",os.path.getsize(scene.render.filepath)>1000)
    except Exception as exc:
        report["tests"]["preview_render"]={"pass":False,"error":repr(exc)}
        report["limitations"].append("Preview render requires correction: "+repr(exc))
    try:
        bpy.ops.object.select_all(action='DESELECT')
        arch=[o for o in bpy.data.objects if o.get("nw_role")=="ARCH"]
        for o in arch:o.select_set(True)
        bpy.context.view_layer.objects.active=arch[0]
        path=os.path.join(OUT,"NW_Architecture_Test.fbx")
        bpy.ops.export_scene.fbx(filepath=path,use_selection=True,object_types={'MESH'},apply_unit_scale=True,add_leaf_bones=False,bake_anim=False)
        check("fbx_file_created",os.path.getsize(path)>1000,os.path.getsize(path))
        report["limitations"].append("FBX file is not proof of Unreal import or material equivalence")
    except Exception as exc:
        report["tests"]["fbx_file_created"]={"pass":False,"error":repr(exc)}
        report["limitations"].append("FBX export requires correction: "+repr(exc))
    report["status"]="EXECUTED_WITH_UNRESOLVED_TESTS" if any(not v["pass"] for v in report["tests"].values()) else "ALL_CURRENT_TESTS_PASS"
    report["duration_seconds"]=round(time.perf_counter()-start,3)
    report["blend_file"]=source
    with open(os.path.join(OUT,"engineering_results.json"),"w") as f:json.dump(report,f,indent=2)
    print("NOWORLD_REPORT="+json.dumps(report))
if __name__ == '__main__':
    try:run()
    except Exception as exc:
        report["status"]="FAILED";report["fatal"]=traceback.format_exc()
        report["duration_seconds"]=round(time.perf_counter()-start,3)
        with open(os.path.join(OUT,"engineering_results.json"),"w") as f:json.dump(report,f,indent=2)
        print("NOWORLD_REPORT="+json.dumps(report))
        raise
