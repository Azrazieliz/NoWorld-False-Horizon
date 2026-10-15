"""Editable per-material asymmetric color gradients on original image textures.
Cross-object world anchoring via shared Empty, local per-object mixing.
Self contained Blender 4.5.10; tests on a local private KK glTF-derived .blend
without modifying source or redistributing private content.
"""
import bpy,os,json,time
from mathutils import Vector
OUT=r"D:\UnrealProjects\NW_Blender_Private_Review_20261015"
started=time.perf_counter()
bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT,"NW_KK_ChikaHaruno_LitReview_NONCANONICAL.blend"))
scene=bpy.context.scene
def mk_group():
 g=bpy.data.node_groups.new("NW_AsymMultiObject_Gradient_v1","ShaderNodeTree")
 def sock(name,typ,default=None,is_output=False):
  s=g.interface.new_socket(name=name,in_out=("OUTPUT" if is_output else "INPUT"),socket_type=typ)
  if default is not None:s.default_value=default
  return s
 sock("Coordinate","NodeSocketVector",(0.5,0.5,0.5))
 sock("Base Texture","NodeSocketColor",(1,1,1,1))
 sock("Bottom Tint","NodeSocketColor",(.38,.38,.8,1))
 sock("Top Tint","NodeSocketColor",(1,.82,.65,1))
 sock("Tint Strength","NodeSocketFloat",.4)
 sock("Gradient Axis","NodeSocketVector",(0,0,1))
 sock("Gradient Offset","NodeSocketFloat",0)
 sock("Asymmetry","NodeSocketFloat",.3)
 sock("Noise Scale","NodeSocketFloat",5)
 sock("Noise Strength","NodeSocketFloat",.08)
 sock("Color","NodeSocketColor",is_output=True)
 sock("Gradient Mask","NodeSocketFloat",is_output=True)
 ns=g.nodes;links=g.links
 ni=ns.new("NodeGroupInput");ni.location=(-1150,150)
 no=ns.new("NodeGroupOutput");no.location=(1350,100)
 dot=ns.new("ShaderNodeVectorMath");dot.operation="DOT_PRODUCT";dot.location=(-850,450)
 links.new(ni.outputs["Coordinate"],dot.inputs[0]);links.new(ni.outputs["Gradient Axis"],dot.inputs[1])
 sep=ns.new("ShaderNodeSeparateXYZ");sep.location=(-850,150)
 links.new(ni.outputs["Coordinate"],sep.inputs["Vector"])
 xcenter=ns.new("ShaderNodeMath");xcenter.operation="SUBTRACT";xcenter.inputs[1].default_value=.5;xcenter.location=(-620,130)
 links.new(sep.outputs["X"],xcenter.inputs[0])
 xgain=ns.new("ShaderNodeMath");xgain.operation="MULTIPLY";xgain.location=(-410,130)
 links.new(xcenter.outputs[0],xgain.inputs[0]);links.new(ni.outputs["Asymmetry"],xgain.inputs[1])
 noise=ns.new("ShaderNodeTexNoise");noise.location=(-850,-220)
 links.new(ni.outputs["Coordinate"],noise.inputs["Vector"])
 links.new(ni.outputs["Noise Scale"],noise.inputs["Scale"])
 noisecenter=ns.new("ShaderNodeMath");noisecenter.operation="SUBTRACT";noisecenter.inputs[1].default_value=.5;noisecenter.location=(-620,-150)
 links.new(noise.outputs["Fac"],noisecenter.inputs[0])
 noisegain=ns.new("ShaderNodeMath");noisegain.operation="MULTIPLY";noisegain.location=(-400,-150)
 links.new(noisecenter.outputs[0],noisegain.inputs[0]);links.new(ni.outputs["Noise Strength"],noisegain.inputs[1])
 add1=ns.new("ShaderNodeMath");add1.operation="ADD";add1.location=(-150,250)
 links.new(dot.outputs["Value"],add1.inputs[0]);links.new(xgain.outputs[0],add1.inputs[1])
 add2=ns.new("ShaderNodeMath");add2.operation="ADD";add2.location=(75,230)
 links.new(add1.outputs[0],add2.inputs[0]);links.new(noisegain.outputs[0],add2.inputs[1])
 add3=ns.new("ShaderNodeMath");add3.operation="ADD";add3.location=(280,210)
 links.new(add2.outputs[0],add3.inputs[0]);links.new(ni.outputs["Gradient Offset"],add3.inputs[1])
 ramp=ns.new("ShaderNodeMapRange");ramp.location=(500,200);ramp.clamp=True;ramp.interpolation_type="SMOOTHSTEP"
 links.new(add3.outputs[0],ramp.inputs["Value"])
 tint=ns.new("ShaderNodeMixRGB");tint.blend_type="MIX";tint.location=(720,90)
 links.new(ramp.outputs["Result"],tint.inputs["Fac"])
 links.new(ni.outputs["Bottom Tint"],tint.inputs["Color1"])
 links.new(ni.outputs["Top Tint"],tint.inputs["Color2"])
 mixed=ns.new("ShaderNodeMixRGB");mixed.blend_type="MULTIPLY";mixed.location=(1030,120)
 links.new(ni.outputs["Tint Strength"],mixed.inputs["Fac"])
 links.new(ni.outputs["Base Texture"],mixed.inputs["Color1"])
 links.new(tint.outputs["Color"],mixed.inputs["Color2"])
 links.new(mixed.outputs["Color"],no.inputs["Color"])
 links.new(ramp.outputs["Result"],no.inputs["Gradient Mask"])
 return g
g=mk_group()
global_anchor=bpy.data.objects.new("NW_Shared_GradientAnchor",None)
scene.collection.objects.link(global_anchor)
global_anchor.empty_display_type="SPHERE"
global_anchor.empty_display_size=.055
# Common origin allows style gradients to flow across meshes without individual UV edits.
global_anchor.location=(0,0,0)
styles={
 "hair":{"find":("hair_f_","hair_b_"),"bottom":(.54,.36,.85,1),"top":(1,.93,.73,1),"mix":.65,"asym":.45,"world":0,"axis":(.16,0,.84)},
 "skirt":{"find":("bot_skirt",),"bottom":(.33,.58,1,1),"top":(1,.87,.92,1),"mix":.38,"asym":.62,"world":0,"axis":(.10,0,.98)}
}
used=[];already={}
for ob in [o for o in bpy.data.objects if o.type=="MESH"]:
 for idx,slot in enumerate(ob.material_slots):
  m=slot.material
  if not m:continue
  stylename=next((k for k,style in styles.items() if any(pattern in m.name.lower() for pattern in style["find"])),None)
  if not stylename:continue
  key=(m.as_pointer(),stylename)
  if key in already:
   slot.material=already[key];continue
  st=styles[stylename]
  copy=m.copy();copy.name="NW_Gradient_"+stylename+"_"+m.name
  copy.use_nodes=True
  nodes=copy.node_tree.nodes;links=copy.node_tree.links
  principled=next((n for n in nodes if n.type=="BSDF_PRINCIPLED" and "Base Color" in n.inputs),None)
  if not principled:continue
  original=principled.inputs["Base Color"]
  previous=next((l.from_socket for l in original.links),None)
  if previous:links.remove(original.links[0])
  tc=nodes.new("ShaderNodeTexCoord");tc.location=(-1000,-680);tc.object=global_anchor
  coord=nodes.new("ShaderNodeMixRGB");coord.blend_type="MIX";coord.location=(-740,-660)
  coord.label="Local (0) / Global world anchor (1)"
  coord.inputs["Fac"].default_value=st["world"]
  links.new(tc.outputs["Generated"],coord.inputs["Color1"])
  links.new(tc.outputs["Object"],coord.inputs["Color2"])
  grp=nodes.new("ShaderNodeGroup");grp.node_tree=g;grp.location=(-360,-600)
  grp.label="EDIT HERE: independent asymmetric gradient"
  links.new(coord.outputs["Color"],grp.inputs["Coordinate"])
  if previous:links.new(previous,grp.inputs["Base Texture"])
  else:grp.inputs["Base Texture"].default_value=tuple(original.default_value)
  grp.inputs["Bottom Tint"].default_value=st["bottom"]
  grp.inputs["Top Tint"].default_value=st["top"]
  grp.inputs["Tint Strength"].default_value=st["mix"]
  grp.inputs["Gradient Axis"].default_value=st["axis"]
  grp.inputs["Asymmetry"].default_value=st["asym"]
  grp.inputs["Noise Scale"].default_value=8
  grp.inputs["Noise Strength"].default_value=.08
  links.new(grp.outputs["Color"],original)
  copy["nw_editable_gradient"]=True
  copy["nw_coordinate_local_global_mix"]="Set the node MIX Fac to 0 local or 1 global anchor"
  already[key]=copy;slot.material=copy
  used.append({"mesh":ob.name,"original":m.name,"material":copy.name,"style":stylename,"base_image_link_preserved":bool(previous)})
assert len(used)>0,"No hair or clothing material matched; cannot validate"
# Save editable noncanonical local style variant, preserving source material links.
scene.render.engine="CYCLES";scene.cycles.samples=10;scene.cycles.device="CPU"
scene.render.threads_mode="FIXED";scene.render.threads=2
scene.render.resolution_x=500;scene.render.resolution_y=720
scene.render.image_settings.file_format="PNG"
style_scene=os.path.join(OUT,"NW_KK_ChikaHaruno_AdaptiveMaterials_PRIVATE.blend")
preview=os.path.join(OUT,"NW_KK_ChikaHaruno_AdaptiveMaterials_PRIVATE.png")
bpy.ops.wm.save_as_mainfile(filepath=style_scene,compress=True)
scene.render.filepath=preview;bpy.ops.render.render(write_still=True)
result={"version":bpy.app.version_string,"status":"TECHNICALLY_EXECUTED",
 "node_group":g.name,"styled_materials":used,"material_count":len(used),
 "preserves_images":all(x["base_image_link_preserved"] for x in used),
 "editable_src":style_scene,"render":preview,
 "seconds":round(time.perf_counter()-started,3),
 "source_materials_changed":False,"in_game_Koikatsu_editing":False,
 "unreal_material_equivalence":False,"professional_art_acceptance":False}
with open(os.path.join(OUT,"adaptive_material_results.json"),"w",encoding="utf-8") as f:json.dump(result,f,indent=2)
print("NW_ADAPTIVE_MATERIALS="+json.dumps(result))
