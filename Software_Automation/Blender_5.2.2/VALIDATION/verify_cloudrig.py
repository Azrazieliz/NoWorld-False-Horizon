"""Read-only installed Blender Studio CloudRig 2.2.x extension and Blender 5.2.2 inspection."""
import bpy,json,os,addon_utils,importlib,traceback
base=os.path.dirname(__file__)
r={"blender":bpy.app.version_string,"cloudrig_found":False,"enabled":False,"path":None}
module="bl_ext.blender_org.cloudrig"
try:
 mod=importlib.import_module(module)
 r["cloudrig_found"]=True
 r["enabled"]=bool(addon_utils.check(module)[1])
 r["path"]=mod.__file__.replace(os.environ.get("APPDATA","__MISSING__"),"%APPDATA%")
 import pathlib
 p=pathlib.Path(mod.__file__).resolve().parent/"blender_manifest.toml"
 r["manifest_exists"]=p.is_file()
 if p.is_file():
  try:
   import tomllib
   mf=tomllib.loads(p.read_text(encoding="utf-8"))
   r["version"]=mf.get("version")
   r["license"]=mf.get("license")
   r["blender_version_min"]=mf.get("blender_version_min")
  except Exception as e:r["manifest_error"]=repr(e)
 r["operators"]=[x.identifier for x in bpy.ops.armature.bl_rna.functions] if False else []
 r["registered_addon_keys"]=[x.module for x in bpy.context.preferences.addons if "cloud" in x.module.lower()]
except Exception:r["error"]=traceback.format_exc()
r["pass"]=bool(r["cloudrig_found"] and r["enabled"] and r["path"] and r.get("version"))
p=os.path.join(base,"cloudrig_522_inventory.json")
with open(p,"w",encoding="utf-8") as f:json.dump(r,f,indent=2)
print("NW_CLOUDRIG_INSTALL="+json.dumps(r))
if not r["pass"]:raise SystemExit(2)
