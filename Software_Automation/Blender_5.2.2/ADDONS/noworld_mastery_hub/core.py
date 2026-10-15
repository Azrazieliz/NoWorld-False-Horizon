"""NoWorld production automation core. Blender 5.2.2 LTS.
No automatic writes to Gate worktrees. Art files are derivatives, never overwritten.
"""
import bpy, json, pathlib, hashlib, datetime, re, os
from mathutils import Vector

CORE_VERSION="1.0.0"
def sha256(path):
    h=hashlib.sha256()
    with open(path,"rb") as fh:
        for chunk in iter(lambda:fh.read(2**20),b""):h.update(chunk)
    return h.hexdigest()

def _validate_path(path, kind="output"):
    p=pathlib.Path(bpy.path.abspath(str(path))).resolve()
    if kind=="source":
        if not p.is_file():raise FileNotFoundError("Missing source: "+str(p))
    else:
        if not p.parent.is_dir():p.parent.mkdir(parents=True,exist_ok=True)
        if p.exists():raise FileExistsError("Refusing to overwrite existing output: "+str(p))
    return p

def _report(path, body):
    dst=_validate_path(path)
    body={"tool":"NoWorld Blender 5.2.2 Production Automation",
          "core_version":CORE_VERSION,
          "timestamp_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
          **body}
    dst.write_text(json.dumps(body,indent=2,default=str),encoding="utf-8")
    return str(dst)

def audit(path=None):
    """Read-only QA: source geometry, materials, UV, textures, armatures, slots and keys."""
    meshes=[o for o in bpy.data.objects if o.type=="MESH"]
    rigs=[o for o in bpy.data.objects if o.type=="ARMATURE"]
    counts={}
    for o in bpy.data.objects:counts[o.type]=counts.get(o.type,0)+1
    rows=[]
    for m in meshes:
        uv=[u.name for u in m.data.uv_layers]
        rows.append({"name":m.name,"vertices":len(m.data.vertices),
            "polygons":len(m.data.polygons),"uv_layers":uv,
            "materials":[x.name for x in m.data.materials],
            "morph_channels":max(len(m.data.shape_keys.key_blocks)-1,0) if m.data.shape_keys else 0,
            "armature_modifier":[q.object.name if q.object else None for q in m.modifiers if q.type=="ARMATURE"]})
    missing=[]
    for im in bpy.data.images:
        if im.source!="FILE" or im.packed_file:continue
        try:
            resolved=pathlib.Path(bpy.path.abspath(im.filepath,library=im.library))
            if not resolved.is_file():missing.append(im.name)
        except Exception:missing.append(im.name)
    bones=[]
    for rig in rigs:
        anim=rig.animation_data
        bones.append({"rig":rig.name,"bone_count":len(rig.data.bones),
            "deformed_meshes":len([o for o in meshes if any(mod.type=="ARMATURE" and mod.object==rig for mod in o.modifiers)]),
            "action":anim.action.name if anim and anim.action else None,
            "action_slot":anim.action_slot.identifier if anim and anim.action_slot else None,
            "constraints":sum(len(p.constraints) for p in rig.pose.bones)})
    outcome={"source_blend":bpy.data.filepath or None,
       "blender_version":bpy.app.version_string,"object_counts":counts,
       "meshes":rows,"rigs":bones,
       "uv_missing":[r["name"] for r in rows if r["polygons"] and not r["uv_layers"]],
       "material_missing":[r["name"] for r in rows if not r["materials"]],
       "missing_images":missing,"total_morph_channels":sum(r["morph_channels"] for r in rows),
       "action_data_blocks":len(bpy.data.actions),
       "grease_pencil_objects":counts.get("GREASEPENCIL",0),
       "artistic_quality_certified":False,"unreal_fidelity_certified":False}
    if path:_report(path,outcome)
    return outcome

def _set_group_socket(g, name, typ, default=None,output=False):
    s=g.interface.new_socket(name=name,in_out="OUTPUT" if output else "INPUT",socket_type=typ)
    if default is not None:s.default_value=default
    return s

def ensure_gradient_group():
    """Create unique reusable asymmetric gradient function with local or shared object coords."""
    group=bpy.data.node_groups.get("NW_Gate_Gradient_1")
    if group:return group
    group=bpy.data.node_groups.new("NW_Gate_Gradient_1","ShaderNodeTree")
    for name,typ,default in [
      ("Position","NodeSocketVector",(.5,.5,.5)),("Base Texture","NodeSocketColor",(1,1,1,1)),
      ("Shadow Tint","NodeSocketColor",(.23,.29,.42,1)),("Highlight Tint","NodeSocketColor",(.88,.78,.70,1)),
      ("Strength","NodeSocketFloat",.45),("Axis","NodeSocketVector",(0,0,1)),
      ("Offset","NodeSocketFloat",0),("Asymmetry","NodeSocketFloat",.25),
      ("Noise Scale","NodeSocketFloat",9),("Noise Strength","NodeSocketFloat",.06)]:
        _set_group_socket(group,name,typ,default)
    _set_group_socket(group,"Color","NodeSocketColor",output=True)
    _set_group_socket(group,"Mask","NodeSocketFloat",output=True)
    n=group.nodes;l=group.links
    ip=n.new("NodeGroupInput");op=n.new("NodeGroupOutput")
    dot=n.new("ShaderNodeVectorMath");dot.operation="DOT_PRODUCT"
    l.new(ip.outputs["Position"],dot.inputs[0]);l.new(ip.outputs["Axis"],dot.inputs[1])
    sep=n.new("ShaderNodeSeparateXYZ");l.new(ip.outputs["Position"],sep.inputs["Vector"])
    cent=n.new("ShaderNodeMath");cent.operation="SUBTRACT";cent.inputs[1].default_value=.5
    l.new(sep.outputs["X"],cent.inputs[0])
    asym=n.new("ShaderNodeMath");asym.operation="MULTIPLY"
    l.new(cent.outputs[0],asym.inputs[0]);l.new(ip.outputs["Asymmetry"],asym.inputs[1])
    noise=n.new("ShaderNodeTexNoise")
    l.new(ip.outputs["Position"],noise.inputs["Vector"])
    l.new(ip.outputs["Noise Scale"],noise.inputs["Scale"])
    ncent=n.new("ShaderNodeMath");ncent.operation="SUBTRACT";ncent.inputs[1].default_value=.5
    l.new(noise.outputs["Fac"],ncent.inputs[0])
    ngain=n.new("ShaderNodeMath");ngain.operation="MULTIPLY"
    l.new(ncent.outputs[0],ngain.inputs[0]);l.new(ip.outputs["Noise Strength"],ngain.inputs[1])
    acc=n.new("ShaderNodeMath");acc.operation="ADD"
    l.new(dot.outputs["Value"],acc.inputs[0]);l.new(asym.outputs[0],acc.inputs[1])
    acc2=n.new("ShaderNodeMath");acc2.operation="ADD"
    l.new(acc.outputs[0],acc2.inputs[0]);l.new(ngain.outputs[0],acc2.inputs[1])
    offset=n.new("ShaderNodeMath");offset.operation="ADD"
    l.new(acc2.outputs[0],offset.inputs[0]);l.new(ip.outputs["Offset"],offset.inputs[1])
    mask=n.new("ShaderNodeMapRange");mask.clamp=True;mask.interpolation_type="SMOOTHSTEP"
    l.new(offset.outputs[0],mask.inputs["Value"])
    tint=n.new("ShaderNodeMixRGB");tint.blend_type="MIX"
    l.new(mask.outputs["Result"],tint.inputs["Fac"])
    l.new(ip.outputs["Shadow Tint"],tint.inputs["Color1"])
    l.new(ip.outputs["Highlight Tint"],tint.inputs["Color2"])
    colored=n.new("ShaderNodeMixRGB");colored.blend_type="MULTIPLY"
    l.new(ip.outputs["Strength"],colored.inputs["Fac"])
    l.new(ip.outputs["Base Texture"],colored.inputs["Color1"])
    l.new(tint.outputs["Color"],colored.inputs["Color2"])
    l.new(colored.outputs["Color"],op.inputs["Color"])
    l.new(mask.outputs["Result"],op.inputs["Mask"])
    group.use_fake_user=True
    return group

def apply_gradient(obj,slot=0,world=False,strength=.45,axis=(0,0,1),asym=.25,
                   shadow=(.23,.29,.42,1),highlight=(.88,.78,.70,1)):
    """Copies slot's material so original source is untouched; preserves incoming image link."""
    if obj.type!="MESH":raise TypeError("Select a mesh")
    if slot>=len(obj.material_slots) or not obj.material_slots[slot].material:
        raise ValueError("Mesh has no material in slot "+str(slot))
    original=obj.material_slots[slot].material
    if original.get("nw_gradient_derived"):
        raise ValueError("This material already has a NoWorld gradient. Adjust node inputs instead.")
    m=original.copy();m.name="NW_Gradient_"+original.name;m.use_nodes=True
    principled=next((x for x in m.node_tree.nodes if x.type=="BSDF_PRINCIPLED"),None)
    if not principled:raise ValueError("Material has no Principled BSDF: "+m.name)
    socket=principled.inputs["Base Color"]
    linked=socket.links[0].from_socket if socket.is_linked else None
    base=tuple(socket.default_value)
    if linked:m.node_tree.links.remove(socket.links[0])
    coord=m.node_tree.nodes.new("ShaderNodeTexCoord")
    if world:
        anchor=bpy.data.objects.get("NW_SharedGradientAnchor")
        if not anchor:
            anchor=bpy.data.objects.new("NW_SharedGradientAnchor",None)
            bpy.context.scene.collection.objects.link(anchor)
        coord.object=anchor
    g=m.node_tree.nodes.new("ShaderNodeGroup")
    g.label="EDIT: asymmetric gradient + original texture";g.node_tree=ensure_gradient_group()
    m.node_tree.links.new(coord.outputs["Object" if world else "Generated"],g.inputs["Position"])
    if linked:m.node_tree.links.new(linked,g.inputs["Base Texture"])
    else:g.inputs["Base Texture"].default_value=base
    for k,v in [("Strength",strength),("Axis",axis),("Asymmetry",asym),
                ("Shadow Tint",shadow),("Highlight Tint",highlight)]:
        g.inputs[k].default_value=v
    m.node_tree.links.new(g.outputs["Color"],socket)
    m["nw_gradient_derived"]=True
    m["nw_source_material"]=original.name
    obj.material_slots[slot].material=m
    return {"object":obj.name,"material":m.name,"source":original.name,
           "source_base_texture_link_preserved":bool(linked),
           "mode":"shared_world_anchor" if world else "per_object_generated","strength":strength}

def export_fbx(path,rig_name=None,action_only=False,mesh_names=None):
    """Non-destructive select/export/recover. Never writes original blend or Gate package."""
    path=_validate_path(path)
    before=[x.name for x in bpy.context.selected_objects]
    active=bpy.context.view_layer.objects.active
    meshes=[o for o in bpy.data.objects if o.type=="MESH"]
    try:
        bpy.ops.object.mode_set(mode="OBJECT") if bpy.context.object and bpy.context.object.mode!="OBJECT" else None
        bpy.ops.object.select_all(action="DESELECT")
        if rig_name:
            rig=bpy.data.objects.get(rig_name)
            if not rig or rig.type!="ARMATURE":raise ValueError("Expected armature: "+rig_name)
            ani=rig.animation_data
            if action_only and not (ani and ani.action and ani.action_slot):
                raise ValueError("Animation-only export requires assigned Action and Blender 5 Action Slot")
            chosen=[rig]
            if not action_only:
                chosen += [m for m in meshes if any(q.type=="ARMATURE" and q.object==rig for q in m.modifiers)]
                if len(chosen)==1:raise ValueError("Skeletal mesh export requires deform meshes; choose action_only if appropriate")
        else:
            names=set(mesh_names or [])
            if names:
                absent=names-set(bpy.data.objects.keys())
                if absent:raise ValueError("Missing mesh names: "+str(sorted(absent)))
            chosen=[m for m in meshes if not names or m.name in names]
            if not chosen:raise ValueError("No mesh objects for static FBX")
        for o in chosen:o.select_set(True)
        bpy.context.view_layer.objects.active=chosen[0]
        bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,
            object_types={"ARMATURE"} if action_only else ({"MESH","ARMATURE"} if rig_name else {"MESH"}),
            add_leaf_bones=False,bake_anim=bool(rig_name),
            bake_anim_use_all_bones=True,bake_anim_use_all_actions=False,
            bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0)
        if not path.is_file() or path.stat().st_size<1000:raise IOError("FBX missing or unexpectedly short")
        return {"file":str(path),"sha256":sha256(path),"bytes":path.stat().st_size,
                "objects":[o.name for o in chosen],"kind":"animation" if action_only else ("skeletal" if rig_name else "static"),
                "action":rig.animation_data.action.name if rig_name and rig.animation_data and rig.animation_data.action else None,
                "action_slot":rig.animation_data.action_slot.identifier if rig_name and rig.animation_data and rig.animation_data.action_slot else None,
                "unreal_native_import_verified_by_export":False}
    finally:
        bpy.ops.object.select_all(action="DESELECT")
        for name in before:
            ob=bpy.data.objects.get(name)
            if ob:ob.select_set(True)
        if active and active.name in bpy.data.objects:bpy.context.view_layer.objects.active=active

def derivative_save(path):
    """New source file only, refuses overwrites."""
    p=_validate_path(path)
    bpy.ops.wm.save_as_mainfile(filepath=str(p),compress=True)
    return {"file":str(p),"sha256":sha256(p),"bytes":p.stat().st_size}

# Art-directed source-preserving look variant; no per-mesh rebuild, only named material inputs.
LOOK_PRESETS={
  "Moonlit": {
    "hair":{"shadow":(.16,.11,.86,1),"highlight":(.90,.85,.48,1),"strength":.90,"axis":(.42,.0,1.8),"asym":.62},
    "skirt":{"shadow":(.03,.48,.8,1),"highlight":(.92,.23,.7,1),"strength":.8,"axis":(.36,0,1.6),"asym":.7}
  },
  "Autumn": {
    "hair":{"shadow":(.23,.08,.04,1),"highlight":(.96,.65,.19,1),"strength":.9,"axis":(.40,0,2.0),"asym":.58},
    "skirt":{"shadow":(.14,.16,.31,1),"highlight":(.98,.55,.24,1),"strength":.8,"axis":(.3,0,1.7),"asym":.62}
  }
}
def apply_look_preset(name="Moonlit",world=False):
    """Automatically classify existing material slots on character rigs.
    Assigns precisely controlled asymmetric gradients on Hair/Skirt image materials.
    Preserves texture links, material source, all mesh vertices and all bones.
    """
    if name not in LOOK_PRESETS:raise ValueError("Unknown look preset "+str(name))
    results=[]
    candidates=[]
    extents={"hair":[],"skirt":[]}
    for ob in bpy.data.objects:
        if ob.type!="MESH":continue
        for index,slot in enumerate(ob.material_slots):
            m=slot.material
            if not m or m.get("nw_gradient_derived"):continue
            k=m.name.lower()
            role="hair" if "_hair_" in k else "skirt" if ("_skirt" in k or "skirt" in k) else None
            if role is None:continue
            candidates.append((ob,index,role))
            # Inspect faces assigned to THIS material, not the whole clothing mesh.
            # Normalize by each role's union to create a continuous world gradient
            # even when several meshes/slots cover one hairstyle or outfit.
            for face in ob.data.polygons:
                if face.material_index==index:
                    for vi in face.vertices:
                        world_co=ob.matrix_world @ ob.data.vertices[vi].co
                        extents[role].append((world_co.x,world_co.z))
    if not candidates:raise RuntimeError("No identifiable hair/skirt material slots with preserved source")
    transforms={}
    for role,points in extents.items():
        if not points:continue
        xmin=min(x for x,z in points);xmax=max(x for x,z in points)
        zmin=min(z for x,z in points);zmax=max(z for x,z in points)
        xs=max(xmax-xmin,.02);zs=max(zmax-zmin,.02)
        ax=.12/xs;az=.84/zs
        transforms[role]={"axis":(ax,0,az),
           "offset":-.12*xmin/xs-.84*zmin/zs+.02,
           "x_span":xs,"z_span":zs}
    for ob,index,role in candidates:
        opt=LOOK_PRESETS[name][role]
        tx=transforms.get(role,{})
        axis=tx.get("axis",opt["axis"]) if world else (.12,0,.84)
        row=apply_gradient(ob,slot=index,world=world,strength=opt["strength"],
                           axis=axis,asym=opt["asym"],
                           shadow=opt["shadow"],highlight=opt["highlight"])
        if world and tx:
            derived=ob.material_slots[index].material
            group=next(node for node in derived.node_tree.nodes if node.type=="GROUP" and
                       node.node_tree and node.node_tree.name=="NW_Gate_Gradient_1")
            group.inputs["Offset"].default_value=tx["offset"]
            group.inputs["Asymmetry"].default_value=opt["asym"]/max(tx["x_span"],.02)
            row["world_gradient_role_extent"]={"x":tx["x_span"],"z":tx["z_span"]}
            row["auto_gradient_offset"]=tx["offset"]
        row["role"]=role
        results.append(row)
    return {"preset":name,"applied_material_slots":len(results),
      "results":results,"meshes_rebuilt":0,"bones_rebuilt":0,
      "source_original_unchanged":True,"canonically_approved":False}

def export_koikatsu_obj(path,mesh_names=None,max_triangles=14000):
    """Unity/ObjImport-ready multimesh OBJ with auto segmentation and UV preservation.
    Does not modify or save source .blend; staged geometric derivatives only.
    MTL exports ordinary material image channels; complex procedural shaders require baking.
    """
    target=_validate_path(path)
    if target.suffix.lower()!=".obj":raise ValueError("KK ObjImport requires .obj")
    chosen=[o for o in bpy.data.objects if o.type=="MESH" and
        (mesh_names is None or o.name in mesh_names)]
    if not chosen:raise ValueError("No selected source mesh objects")
    missing=set(mesh_names or [])-{o.name for o in chosen}
    if missing:raise ValueError("Mesh names absent: "+str(sorted(missing)))
    if len(chosen)>150:raise ValueError("Explicitly select subset: refusing unintended 150+ source meshes")
    before=[o.name for o in bpy.context.selected_objects]
    active=bpy.context.view_layer.objects.active
    work=[]
    groups=[]
    try:
        if bpy.context.object and bpy.context.object.mode!="OBJECT":
            bpy.ops.object.mode_set(mode="OBJECT")
        dg=bpy.context.evaluated_depsgraph_get()
        for original in chosen:
            evaluated=original.evaluated_get(dg)
            mesh=evaluated.to_mesh(preserve_all_data_layers=True,depsgraph=dg)
            try:
                tris=list(mesh.loop_triangles)
                if not tris:continue
                uv=mesh.uv_layers.active
                material_names=[m.name if m else None for m in mesh.materials]
                for index in range(0,len(tris),max_triangles):
                    batch=tris[index:index+max_triangles]
                    vmap={};vertices=[];faces=[];faceuv=[];face_materials=[];face_smooth=[]
                    for tr in batch:
                        face=[];uv_triplet=[]
                        poly=mesh.polygons[tr.polygon_index]
                        for vi,loopi in zip(tr.vertices,tr.loops):
                            uvxy=tuple(round(float(x),7) for x in uv.data[loopi].uv) if uv else (0,0)
                            key=(int(vi),uvxy)
                            if key not in vmap:
                                vmap[key]=len(vertices);vertices.append(tuple(mesh.vertices[vi].co))
                            face.append(vmap[key]);uv_triplet.append(uvxy)
                        faces.append(face);faceuv.append(uv_triplet)
                        face_materials.append(int(poly.material_index));face_smooth.append(bool(poly.use_smooth))
                    assert len(vertices)<=65535 and len(faces)<=max_triangles
                    outmesh=bpy.data.meshes.new("NW_KK_"+original.name+"_"+str(index//max_triangles))
                    outmesh.from_pydata(vertices,[],faces);outmesh.update()
                    if uv:
                        outuv=outmesh.uv_layers.new(name=uv.name)
                        for j,poly in enumerate(outmesh.polygons):
                            for offset,li in enumerate(poly.loop_indices):
                                outuv.data[li].uv=faceuv[j][offset]
                    for mat in mesh.materials:
                        if mat:outmesh.materials.append(mat)
                    for j,poly in enumerate(outmesh.polygons):
                        if len(outmesh.materials):
                            poly.material_index=min(face_materials[j],len(outmesh.materials)-1)
                        poly.use_smooth=face_smooth[j]
                    outobj=bpy.data.objects.new("NW_KK_"+original.name+"_"+str(index//max_triangles),outmesh)
                    bpy.context.scene.collection.objects.link(outobj)
                    outobj.matrix_world=original.matrix_world.copy()
                    work.append(outobj)
                    groups.append({"original":original.name,"part":outobj.name,
                      "triangles":len(faces),"vertices":len(vertices),"uv_preserved":uv is not None,
                      "materials":material_names})
            finally:
                evaluated.to_mesh_clear()
        if not work:raise ValueError("No triangles available to export")
        bpy.ops.object.select_all(action="DESELECT")
        for o in work:o.select_set(True)
        bpy.context.view_layer.objects.active=work[0]
        bpy.ops.wm.obj_export(filepath=str(target),export_selected_objects=True,
            export_uv=True,export_normals=True,export_materials=True,
            export_triangulated_mesh=True,path_mode="COPY",
            apply_modifiers=False,forward_axis="NEGATIVE_Z",up_axis="Y")
        if not target.is_file() or target.stat().st_size<90:raise RuntimeError("OBJ export did not create geometry")
        mtl=target.with_suffix(".mtl")
        return {"obj":str(target),"sha256":sha256(target),"bytes":target.stat().st_size,
          "mtl":str(mtl) if mtl.exists() else None,"source_meshes":len(chosen),
          "generated_mesh_parts":len(work),"auto_segments":groups,
          "limits":{"max_triangles_per_object":max_triangles,"max_vertices_per_object":65535},
          "complex_shader_equivalence":False,
          "textures_and_masks_require_baking_for_game_fidelity":True,
          "original_source_untouched":True,
          "in_game_import_confirmed_by_export_alone":False}
    finally:
        for o in work:
            d=o.data
            bpy.data.objects.remove(o,do_unlink=True)
            if d.users==0:bpy.data.meshes.remove(d)
        bpy.ops.object.select_all(action="DESELECT")
        for name in before:
            ob=bpy.data.objects.get(name)
            if ob:ob.select_set(True)
        if active and active.name in bpy.data.objects:
            bpy.context.view_layer.objects.active=active
