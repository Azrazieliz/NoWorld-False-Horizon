bl_info = {
    "name": "NoWorld Mastery Hub",
    "author": "NoWorld software mastery automation",
    "version": (0, 5, 0),
    "blender": (5, 2, 0),
    "location": "3D View > Sidebar (N) > NoWorld",
    "description": "Read-only software mastery workflows, native diagnostics and exact control routes",
    "category": "Development",
}

import bpy
import json
import os
import tempfile
import datetime
import pathlib
import webbrowser
from bpy.props import EnumProperty, StringProperty, BoolProperty
from bpy.types import Operator, Panel, PropertyGroup
from . import gate_ops


_ROOT = pathlib.Path(__file__).resolve().parent
with (_ROOT / "WORKFLOWS.json").open("r", encoding="utf-8") as _f:
    _REGISTRY = json.load(_f)
ROWS = _REGISTRY["entries"]
WORKFLOWS = {x["id"]: x for x in ROWS}
DOMAINS = sorted({x["domain"] for x in ROWS})


def enum_domains(self, context):
    return [("ALL", "All Domains", "Every workflow")] + [(x, x, x) for x in DOMAINS]


def enum_workflows(self, context):
    current = getattr(self, "domain", "ALL")
    found = [x for x in ROWS if current == "ALL" or current == x["domain"]]
    return [(x["id"], x["name"], x["status"] + " | " + x["domain"]) for x in found] or [
        (ROWS[0]["id"], ROWS[0]["name"], ROWS[0]["domain"])]


def _get(context):
    pid = context.window_manager.nw_mastery.workflow
    return WORKFLOWS.get(pid, ROWS[0])


class NW_MasterSettings(PropertyGroup):
    domain: EnumProperty(name="Domain", description="Select specialized software domain",
                         items=enum_domains)
    workflow: EnumProperty(name="Workflow", description="Specific actionable workflow",
                           items=enum_workflows)
    last_audit: StringProperty(name="Last Audit", default="Not run")
    last_audit_path: StringProperty(default="")
    show_method: BoolProperty(name="Show source logic", default=True)


class NW_OT_copy_workflow(Operator):
    bl_idname = "nw_mastery.copy_workflow"
    bl_label = "Copy Exact Workflow"
    bl_description = "Copy UI route, native automation API, validation state and official source; no asset changes"
    def execute(self, context):
        a = _get(context)
        text = (
            "NoWorld Blender 5.2.2: " + a["name"] + "\n"
            "DOMAIN: " + a["domain"] + "\n"
            "STATUS: " + a["status"] + " (not equivalent to artistic acceptance)\n"
            "GUI: " + a["ui"] + "\n"
            "METHOD: " + a["method"] + "\n"
            "AUTOMATION: " + a["api"] + "\n"
            "SOURCE: " + a["url"] + "\n"
        )
        context.window_manager.clipboard = text
        self.report({'INFO'}, "Copied exact UI and API workflow, no changes to assets")
        return {'FINISHED'}


class NW_OT_open_reference(Operator):
    bl_idname = "nw_mastery.open_reference"
    bl_label = "Open Workflow Reference"
    bl_description = "Open the documented exact source website"
    def execute(self, context):
        a = _get(context)
        webbrowser.open(a["url"])
        self.report({'INFO'}, "Opened workflow documentation: " + a["id"])
        return {'FINISHED'}


class NW_OT_open_handbook(Operator):
    bl_idname = "nw_mastery.open_handbook"
    bl_label = "Open Master Playbook"
    bl_description = "Open a detailed local software mastery book; no modification of scene"
    def execute(self, context):
        path = _ROOT / "MASTERY_PLAYBOOK.html"
        if not path.is_file():
            self.report({'ERROR'}, "Local master playbook not found")
            return {'CANCELLED'}
        webbrowser.open(path.as_uri())
        self.report({'INFO'}, "Opened local master playbook")
        return {'FINISHED'}


def audit_scene(context):
    scene = context.scene
    meshes = [x for x in bpy.data.objects if x.type == "MESH"]
    lights = [x for x in bpy.data.objects if x.type == "LIGHT"]
    rigs = [x for x in bpy.data.objects if x.type == "ARMATURE"]
    anim_objects = [x for x in bpy.data.objects if x.animation_data and x.animation_data.action]
    assets = [x for x in bpy.data.objects if getattr(x, "asset_data", None)]
    gp = [x for x in bpy.data.objects if x.type == "GREASEPENCIL"]
    image_issues = []
    for i in bpy.data.images:
        # Packed images are valid; generated images do not require external paths.
        if i.source == "FILE" and not i.packed_file and i.filepath:
            try:
                resolved = bpy.path.abspath(i.filepath, library=i.library)
                if not pathlib.Path(resolved).exists():
                    image_issues.append(i.name)
            except Exception:
                image_issues.append(i.name)

    uv_missing = [m.name for m in meshes if not m.data.uv_layers and len(m.data.polygons)]
    no_mats = [m.name for m in meshes if len(m.data.materials) == 0]
    no_uv_action = [m.name for m in meshes if not m.data.uv_layers and any(
        mod.type == "NODES" for mod in m.modifiers)]

    # An Action being present is not evidence of actual skeletal motion.
    anim_rigs = []
    for r in rigs:
        ad = r.animation_data
        action = ad.action if ad else None
        anim_rigs.append({
            "bones": len(r.data.bones),
            "assigned_action": bool(action),
            "valid_action_slot": bool(ad and ad.action_slot),
            "meshes_deformed": len([m for m in meshes if any(
                mod.type == "ARMATURE" and mod.object == r for mod in m.modifiers)]),
            "constraints": sum(len(pb.constraints) for pb in r.pose.bones)
        })

    counts = {}
    for ob in bpy.data.objects:
        counts[ob.type] = counts.get(ob.type, 0) + 1
    summary = {
        "generated_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "blender_version": bpy.app.version_string,
        "audit_mode": "READ_ONLY; no objects/materials/animations created or changed",
        "professional_demo_acceptance": 0,
        "scene_saved": bool(bpy.data.filepath),
        "scene_object_count": len(bpy.data.objects),
        "object_types": counts,
        "meshes": len(meshes),
        "total_mesh_vertices": sum(len(m.data.vertices) for m in meshes),
        "total_mesh_faces": sum(len(m.data.polygons) for m in meshes),
        "uv_missing_count": len(uv_missing),
        "uv_missing_names": uv_missing[:100],
        "missing_material_count": len(no_mats),
        "missing_material_names": no_mats[:100],
        "geometry_nodes_modifiers": sum(sum(mod.type == "NODES" for mod in m.modifiers) for m in meshes),
        "geo_node_uv_missing": no_uv_action[:100],
        "shape_key_channels": sum(max(len(m.data.shape_keys.key_blocks) - 1, 0) for m in meshes if m.data.shape_keys),
        "rigs": anim_rigs,
        "armatures": len(rigs),
        "assigned_animation_actions": len(anim_objects),
        "action_data_blocks": len(bpy.data.actions),
        "grease_pencil_objects": len(gp),
        "lights": len(lights),
        "assets_marked": len(assets),
        "missing_external_images": image_issues[:100],
        "missing_external_images_count": len(image_issues),
        "warning": "Static scene checks cannot certify actual character performance, source likeness, Unreal parity, or high-end visual quality."
    }
    return summary


class NW_OT_audit_scene(Operator):
    bl_idname = "nw_mastery.audit_scene"
    bl_label = "Run Read-only Scene Audit"
    bl_description = "Audit scene assets, UVs, materials, rigs, shape keys, action slots, Grease Pencil and broken image paths without modifying data"
    def execute(self, context):
        try:
            report = audit_scene(context)
            loc = pathlib.Path(tempfile.gettempdir()) / "NoWorldMastery" / "runtime_audits"
            loc.mkdir(parents=True, exist_ok=True)
            path = loc / "latest_scene_audit.json"
            path.write_text(json.dumps(report, indent=2), encoding="utf-8")
            settings = context.window_manager.nw_mastery
            settings.last_audit_path = str(path)
            settings.last_audit = "Objects {} | mesh {} | rigs {} | UV absent {}".format(
                report["scene_object_count"], report["meshes"], report["armatures"], report["uv_missing_count"])
            self.report({'INFO'}, "Read-only audit saved to local temp, {} meshes".format(report["meshes"]))
            return {'FINISHED'}
        except Exception as exc:
            self.report({'ERROR'}, "Audit error: {}".format(exc))
            return {'CANCELLED'}


class NW_OT_copy_audit_path(Operator):
    bl_idname = "nw_mastery.copy_audit_path"
    bl_label = "Copy Audit JSON Path"
    bl_description = "Copy report filepath without accessing or modifying project content"
    def execute(self, context):
        settings = context.window_manager.nw_mastery
        if not settings.last_audit_path:
            self.report({'ERROR'}, "Run an audit first")
            return {'CANCELLED'}
        context.window_manager.clipboard = settings.last_audit_path
        self.report({'INFO'}, "Copied report path")
        return {'FINISHED'}


class NW_OT_workspace(Operator):
    bl_idname = "nw_mastery.switch_workspace"
    bl_label = "Open Blender Workspace"
    bl_description = "Jump to the chosen native Blender workspace, no geometry changes"
    name: StringProperty()
    def execute(self, context):
        ws = bpy.data.workspaces.get(self.name)
        if not ws:
            self.report({'WARNING'}, "Workspace '{}' not in this file".format(self.name))
            return {'CANCELLED'}
        context.window.workspace = ws
        return {'FINISHED'}


class NW_OT_installed_tools(Operator):
    bl_idname = "nw_mastery.installed_tools"
    bl_label = "Inspect Installed Automators"
    bl_description = "Check installed addon status and Blender version without modifying data"
    def execute(self, context):
        import addon_utils
        tools = ["bl_ext.blender_org.cloudrig", "bl_ext.blender_org.retarget", "rigify"]
        details = {}
        for tool in tools:
            try:
                details[tool] = {"installed_and_enabled": bool(addon_utils.check(tool)[1])}
            except Exception as exc:
                details[tool] = {"installed_and_enabled": False, "error": str(exc)}
        path = pathlib.Path(tempfile.gettempdir()) / "NoWorldMastery" / "installed_tools.json"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps({
            "blender_version": bpy.app.version_string, "addons": details}, indent=2), encoding="utf-8")
        context.window_manager.nw_mastery.last_audit = "CloudRig: {} | Retarget: {}".format(
            details[tools[0]]["installed_and_enabled"], details[tools[1]]["installed_and_enabled"])
        context.window_manager.nw_mastery.last_audit_path = str(path)
        self.report({'INFO'}, "Installed tools checked without scene modification")
        return {'FINISHED'}


class NW_PT_hub(Panel):
    bl_idname = "NW_PT_mastery_hub"
    bl_label = "NoWorld | Software Mastery"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "NoWorld"

    def draw(self, context):
        layout = self.layout
        settings = context.window_manager.nw_mastery
        col = layout.column(align=True)
        col.label(text="Blender {}".format(bpy.app.version_string), icon="BLENDER")
        col.label(text="PRODUCTION ART: NOT ACCEPTED", icon="ERROR")
        col.separator()
        col.label(text="Exact controls, no asset generation")
        col.prop(settings, "domain")
        col.prop(settings, "workflow")
        wf = _get(context)
        box = layout.box()
        box.label(text=wf["domain"] + " | " + wf["status"], icon="INFO")
        for field in ("ui", "method"):
            parts = wf[field].split(" > ") if field == "ui" else [wf[field]]
            text = " > ".join(parts)
            # Blender UI labels don't auto-wrap: split intelligently for legible sidebar.
            max_width = 45
            for j in range(0, min(len(text), 320), max_width):
                box.label(text=text[j:j+max_width])
        if settings.show_method:
            box.separator()
            box.label(text="API route:")
            api = wf["api"]
            for j in range(0, min(len(api), 240), 45):
                box.label(text=api[j:j+45])
        row = layout.row(align=True)
        row.operator("nw_mastery.copy_workflow", text="Copy How-To", icon="COPYDOWN")
        row.operator("nw_mastery.open_reference", text="Source", icon="URL")
        layout.prop(settings, "show_method")
        layout.separator()
        col = layout.column(align=True)
        col.label(text="Immediate automation controls", icon="PREFERENCES")
        col.operator("nw_mastery.audit_scene", text="One-click Read-only Audit", icon="CHECKMARK")
        col.operator("nw_mastery.installed_tools", text="Installed Tools Report", icon="CONSOLE")
        col.operator("nw_mastery.copy_audit_path", text="Copy Report Path", icon="COPYDOWN")
        if settings.last_audit != "Not run":
            for j in range(0, len(settings.last_audit), 38):
                col.label(text=settings.last_audit[j:j+38])
        layout.separator()
        layout.operator("nw_mastery.open_handbook", text="Open Complete Workflow Book", icon="TEXT")
        grid = layout.grid_flow(columns=2, align=True)
        for value, label in (
            ("Animation", "Animation"), ("Geometry Nodes", "Geometry"),
            ("Shading", "Materials"), ("Sculpting", "Sculpt"),
            ("2D Animation", "2D"), ("UV Editing", "UV")):
            op = grid.operator("nw_mastery.switch_workspace", text=label)
            op.name = value
        gate_ops.draw_ui(layout,context)
        layout.label(text="Sources protected; derivative workflows", icon="LOCKED")


CLASSES = (NW_MasterSettings,NW_OT_copy_workflow,NW_OT_open_reference,
           NW_OT_open_handbook,NW_OT_audit_scene,NW_OT_copy_audit_path,
           NW_OT_workspace,NW_OT_installed_tools,NW_PT_hub) + gate_ops.GATE_CLASSES


def register():
    for c in CLASSES: bpy.utils.register_class(c)
    bpy.types.WindowManager.nw_mastery = bpy.props.PointerProperty(type=NW_MasterSettings)
    bpy.types.WindowManager.nw_gate = bpy.props.PointerProperty(type=gate_ops.NW_GateSettings)


def unregister():
    del bpy.types.WindowManager.nw_gate
    del bpy.types.WindowManager.nw_mastery
    for c in reversed(CLASSES): bpy.utils.unregister_class(c)


if __name__ == "__main__":
    register()
