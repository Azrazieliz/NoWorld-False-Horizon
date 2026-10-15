"""One-time autonomous installation: install NoWorld Mastery Hub into Blender 5.2 user add-ons.
Only addon files and Blender 5.2 user preferences are changed. Never production assets.
"""
import bpy,pathlib,shutil,json,addon_utils,datetime,os
root=pathlib.Path(__file__).resolve().parents[1]
src=root/"ADDONS"/"noworld_mastery_hub"
target=pathlib.Path(bpy.utils.user_resource("SCRIPTS",path="addons",create=True))/"noworld_mastery_hub"
if target.exists():
 if not (target/"__init__.py").is_file():raise RuntimeError("Unrecognized existing addon directory; stop")
 shutil.rmtree(target)
shutil.copytree(src,target,ignore=shutil.ignore_patterns("__pycache__","*.pyc","*.blend1"))
addon_utils.modules_refresh()
addon_utils.enable("noworld_mastery_hub",default_set=True,persistent=True)
assert addon_utils.check("noworld_mastery_hub")[1]
assert hasattr(bpy.context.window_manager,"nw_gate")
bpy.ops.wm.save_userpref()
report={"blender":bpy.app.version_string,"addon":str(target),"enabled":True,
 "persistent_userpref":True,"workflows":len(__import__("noworld_mastery_hub").ROWS),
 "production_gate_modified":False,
 "installed_utc":datetime.datetime.now(datetime.timezone.utc).isoformat()}
p=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015\addon_installed.json")
p.write_text(json.dumps(report,indent=2),encoding="utf-8")
print("NOWORLD_HUB_INSTALLED="+json.dumps(report))
