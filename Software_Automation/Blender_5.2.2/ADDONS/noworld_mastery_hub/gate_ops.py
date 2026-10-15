"""Native Blender 5.2 sidebar actions for Gate 01/12 immediate reuse.
Non-overwriting exports to a separate review directory; errors visible in Blender reports.
"""
import bpy,pathlib,tempfile,json,datetime
from bpy.types import Operator,PropertyGroup
from bpy.props import StringProperty,EnumProperty,FloatProperty,BoolProperty
from . import core
class NW_GateSettings(PropertyGroup):
    output_dir:StringProperty(name="Output Folder",subtype='DIR_PATH',default="")
    asset_id:StringProperty(name="Output Name",default="NW_Asset")
    rig_name:StringProperty(name="Rig (exact name)",description="Armature object name for animation/skeletal export")
    export_mode:EnumProperty(name="Export",items=[
      ("STATIC","Static FBX","Selected meshes, or all meshes if nothing selected"),
      ("SKELETAL","Skeletal FBX","Armature with deformed meshes"),
      ("ANIMATION","Animation-only FBX","Armature-only baked Action+Slot")])
    gradient_strength:FloatProperty(name="Gradient Strength",min=0,max=1,default=.45)
    shared_anchor:BoolProperty(name="Shared Gradient",description="Anchor material gradient across objects",default=False)
    look_preset:EnumProperty(name="Automated Look",items=[("Moonlit","Moonlit","Cool-warm hair and skirt gradients"),("Autumn","Autumn","Warm hair and contrasting clothing gradients")])

def outpath(context,ext):
    a=context.window_manager.nw_gate
    directory=pathlib.Path(bpy.path.abspath(a.output_dir)).resolve() if a.output_dir else pathlib.Path(tempfile.gettempdir())/"NoWorldMastery"/"GateExports"
    directory.mkdir(parents=True,exist_ok=True)
    name="".join(c if c.isalnum() or c in "-_" else "_" for c in a.asset_id) or "NW_Asset"
    stamp=datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%d_%H%M%S_%f")
    return directory/(name+"_"+stamp+"."+ext)

class NW_OT_gate_export(Operator):
    bl_idname="nw_gate.export"
    bl_label="Export Gate FBX"
    bl_description="Write a new FBX derivative without overwriting existing source"
    def execute(self,context):
        a=context.window_manager.nw_gate
        try:
            path=outpath(context,"fbx")
            selected=[o.name for o in context.selected_objects if o.type=="MESH"]
            kind=a.export_mode
            res=core.export_fbx(path,rig_name=a.rig_name if kind!="STATIC" else None,
                action_only=kind=="ANIMATION",mesh_names=selected if kind=="STATIC" else None)
            ev={"export":res,"scene":bpy.data.filepath or "UNSAVED","art_acceptance":False,
                "UE_import_done_by_UI":False}
            report=path.with_suffix(".json")
            report.write_text(json.dumps(ev,indent=2),encoding="utf-8")
            context.window_manager.clipboard=str(path)
            self.report({'INFO'},"Exported "+str(path.name)+" | path copied")
            return {'FINISHED'}
        except Exception as e:
            self.report({'ERROR'},"Export failed: "+str(e))
            return {'CANCELLED'}
class NW_OT_gate_gradient(Operator):
    bl_idname="nw_gate.apply_gradient"
    bl_label="Apply Editable Gradient"
    bl_description="Copies material, retains original texture link and makes adjustable gradient group"
    def execute(self,context):
        a=context.window_manager.nw_gate
        try:
            o=context.active_object
            if not o or o.type!="MESH":raise ValueError("Select a mesh")
            r=core.apply_gradient(o,strength=a.gradient_strength,world=a.shared_anchor)
            self.report({'INFO'},"New editable derived material: "+r["material"])
            return {'FINISHED'}
        except Exception as e:
            self.report({'ERROR'},"Gradient failed: "+str(e))
            return {'CANCELLED'}
class NW_OT_gate_save_derivative(Operator):
    bl_idname="nw_gate.save_derivative"
    bl_label="Save New Editable Blend"
    bl_description="Save a new .blend copy to isolated output folder, never overwrite source"
    def execute(self,context):
        try:
            p=outpath(context,"blend")
            result=core.derivative_save(p)
            context.window_manager.clipboard=result["file"]
            self.report({'INFO'},"Saved editable derivative, path copied")
            return {'FINISHED'}
        except Exception as e:
            self.report({'ERROR'},"Save derivative failed: "+str(e))
            return {'CANCELLED'}
class NW_OT_gate_auto_look(Operator):
    bl_idname="nw_gate.auto_look"
    bl_label="Automate Hair and Outfit Look"
    bl_description="Automatically classify and style every hair and skirt material; preserves source texture links"
    def execute(self,context):
        cfg=context.window_manager.nw_gate
        try:
            r=core.apply_look_preset(cfg.look_preset,world=cfg.shared_anchor)
            path=outpath(context,"blend")
            saved=core.derivative_save(path)
            path.with_suffix(".json").write_text(json.dumps({"look":r,"derivative":saved,"source_preserved":True},indent=2),encoding="utf-8")
            self.report({"INFO"},"Styled {} material slots, saved derivative".format(r["applied_material_slots"]))
            return {"FINISHED"}
        except Exception as e:
            self.report({"ERROR"},str(e))
            return {"CANCELLED"}
class NW_OT_gate_send_kk(Operator):
    bl_idname="nw_gate.send_kk"
    bl_label="Send OBJ Directly to Modded Koikatsu Studio"
    bl_description="Export selected mesh assets into the installed KK ObjImport bridge and enqueue automatic import"
    def execute(self,context):
        try:
            game=pathlib.Path(r"D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014")
            plugin=game/"BepInEx"/"plugins"/"NoWorldBlenderBridge"/"NoWorld.BlenderBridge.dll"
            if not plugin.is_file():
                raise RuntimeError("Optional Studio OBJ bridge not installed in isolated SM01 sandbox; native Maker skinned import remains available")
            root=game/"BepInEx"/"NoWorldBridge"
            assets=root/"assets";inbox=root/"inbox";events=root/"events"
            for p in (assets,inbox,events):p.mkdir(parents=True,exist_ok=True)
            selected=[o.name for o in context.selected_objects if o.type=="MESH"]
            if not selected:raise RuntimeError("No selected meshes; refusing export of entire scene")
            cfg=context.window_manager.nw_gate
            clean="".join(c if c.isalnum() or c in "-_" else "_" for c in cfg.asset_id)[:40]
            job="NW_GUI_"+clean+"_"+datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%d%H%M%S%f")
            destination=assets/(job+".obj")
            res=core.export_koikatsu_obj(destination,mesh_names=selected)
            ready=inbox/(job+".ready");tmp=inbox/(job+".pending")
            tmp.write_text(str(destination)+"\n"+res["sha256"]+"\n",encoding="utf-8")
            tmp.replace(ready)
            result={"status":"QUEUED_FOR_KK","job_id":job,"obj":res,"source_blend":bpy.data.filepath,
                "in_game_quality_certified":False}
            (assets/(job+".submission.json")).write_text(json.dumps(result,indent=2),encoding="utf-8")
            self.report({"INFO"},"Queued {} mesh group(s) for KK Studio".format(len(selected)))
            return {"FINISHED"}
        except Exception as e:
            self.report({"ERROR"},str(e))
            return {"CANCELLED"}
GATE_CLASSES=(NW_GateSettings,NW_OT_gate_export,NW_OT_gate_gradient,NW_OT_gate_save_derivative,NW_OT_gate_auto_look,NW_OT_gate_send_kk)
def draw_ui(layout,context):
    cfg=context.window_manager.nw_gate
    box=layout.box()
    box.label(text="GATE 01/12 : DO REAL WORK",icon="EXPORT")
    box.prop(cfg,"output_dir")
    box.prop(cfg,"asset_id")
    box.prop(cfg,"export_mode")
    if cfg.export_mode!="STATIC":box.prop(cfg,"rig_name")
    box.operator("nw_gate.export",text="Export Native FBX + Evidence",icon="EXPORT")
    box.separator()
    box.prop(cfg,"gradient_strength")
    box.prop(cfg,"shared_anchor")
    box.operator("nw_gate.apply_gradient",text="Create Non-Destructive Gradient",icon="MATERIAL")
    box.prop(cfg,"look_preset")
    box.operator("nw_gate.auto_look",text="AUTO: Restyle All Hair + Skirt",icon="MATERIAL")
    box.operator("nw_gate.save_derivative",text="Save Separate Editable Source",icon="FILE_BLEND")
    box.operator("nw_gate.send_kk",text="AUTO: Send Mesh to Modded KK Studio",icon="EXPORT")
    box.label(text="No source overwrite; UE QA still required",icon="INFO")
