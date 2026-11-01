"""NoWorld 5-star actual UnrealEditor-Cmd Python worker.

Never stand-in geometry or copy a generic dummy. Imports only source-authentic
GLB parts or user-supplied artwork; creates a real native skeletal appearance
UAsset through OGNativeContentFactoryEditorLibrary (authoring-only C++ API).
Then atomically records source-to-runtime bindings. No Registry/ownership grant.
Unreal must enable its existing Python and source-format import plugins.
"""
import hashlib
import json
import os
import pathlib
import re
import traceback
import unreal

repo=pathlib.Path(__file__).resolve().parents[2]
job_path=pathlib.Path(os.environ["NOWORLD_FACTORY_JOB"]).resolve(strict=True)
receipt=pathlib.Path(os.environ["NOWORLD_FACTORY_RECEIPT"]).resolve()
stage=os.environ["NOWORLD_FACTORY_STAGE"]
job=json.loads(job_path.read_text("utf-8-sig"))
if job.get("schema")!="noworld.factory.job.v1":
    raise ValueError("Unapproved content job schema")
if stage not in ("character","artwork"):
    raise ValueError("Invalid native UE authoring stage")
job_id=job["job_id"]
if not re.fullmatch(r"[a-z0-9][a-z0-9_.-]{2,80}",job_id):
    raise ValueError("Invalid job id")
project=pathlib.Path(unreal.Paths.project_dir()).resolve()
content_root=pathlib.Path(unreal.Paths.project_content_dir()).resolve()
index=content_root/"NoWorld/ContentFactory/Characters.json"
def sha(path):
    with pathlib.Path(path).open("rb") as f:
        return hashlib.file_digest(f,"sha256").hexdigest()
def write_json(path,obj):
    path.parent.mkdir(parents=True,exist_ok=True)
    temp=path.with_suffix(path.suffix+".new")
    temp.write_text(json.dumps(obj,ensure_ascii=False,indent=2)+"\n","utf-8")
    os.replace(temp,path)
def need_file(filename,expected=None):
    path=pathlib.Path(filename).resolve(strict=True)
    if not path.is_file() or (expected and sha(path)!=expected):
        raise ValueError("Authentic original source absent/hash mismatch: "+str(path))
    return path
def normalized(value):
    name=re.sub(r"[^a-z0-9_]+","_",value.lower()).strip("_")
    if not name or len(name)>150:
        raise ValueError("Invalid authored content ID")
    return name
def register_actual(file,destination,source_sha,expected_class):
    file=need_file(file,source_sha)
    if not destination.startswith("/Game/NoWorld/ContentFactory/"):
        raise ValueError("Native asset destination outside factory scope")
    if unreal.EditorAssetLibrary.does_directory_exist(destination):
        old=unreal.EditorAssetLibrary.list_assets(destination,recursive=True,include_folder=False)
        if old:
            raise FileExistsError("Refuse overwrite earlier authored native asset: "+destination)
    task=unreal.AssetImportTask()
    task.set_editor_property("filename",str(file))
    task.set_editor_property("destination_path",destination)
    task.set_editor_property("automated",True)
    task.set_editor_property("replace_existing",False)
    task.set_editor_property("save",True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    assets=unreal.EditorAssetLibrary.list_assets(destination,recursive=True,include_folder=False)
    native=[]
    for asset_path in assets:
        obj=unreal.EditorAssetLibrary.load_asset(asset_path)
        if obj and obj.get_class().get_name()==expected_class:
            native.append(str(obj.get_path_name()))
    if not native: raise RuntimeError("Real native "+expected_class+" not imported: "+str(file))
    return native
def merge_index(row):
    current=json.loads(index.read_text("utf-8-sig")) if index.exists() else {
       "schema":"noworld.content_factory.index.v1","characters":[]}
    if current.get("schema")!="noworld.content_factory.index.v1" or not isinstance(current.get("characters"),list):
        raise ValueError("Existing content registry is not authoritative format")
    found=[c for c in current["characters"] if c["identity"]==row["identity"] and c["version"]==row["version"]]
    if found and found[0]!=row:
        raise FileExistsError("Refuse silently replace prior Identity/Version definition")
    if not found:current["characters"].append(row)
    current["characters"].sort(key=lambda c:(c["identity"],c["version"]))
    write_json(index,current)
status={"schema":"noworld.factory.unreal.v1","stage":stage,
        "job_id":job_id,"status":"STARTED","assets":[],
        "runtime_verified":False,"device_verified":False,"owner_accepted":False}
try:
    if stage=="character":
        work=pathlib.Path(os.environ["NOWORLD_FACTORY_RECEIPT"]).parent
        source=json.loads((work/"blender_receipt.json").read_text("utf-8-sig"))
        if source.get("status")!="SOURCE_RIG_MORPHS_SKIN_EXPORTED" or source.get("job_id")!=job_id:
            raise ValueError("Authentic Blender source receipt is missing")
        if not isinstance(job.get("native"),dict):
            raise ValueError("Native character requires authored binding metadata")
        native=job["native"]
        version=job["version"]
        if not (job["identity"].startswith("character:") and version.startswith("character:")):
            raise ValueError("Character identity and version must be explicit existing canon IDs")
        if not all(native.get(k) for k in ("rig","anatomy","wardrobe","wardrobe_state_field","shared_motion_library")):
            raise ValueError("Native rig/wardrobe/shared library cannot be inferred from visual appearance")
        suffix=normalized(version)
        parts=[]
        for part in source["parts"]:
            target="/Game/NoWorld/ContentFactory/Characters/"+suffix+"/"+normalized(job_id)+"/part_"+str(part["index"]).zfill(3)
            meshes=register_actual(part["path"],target,part["sha256"],"SkeletalMesh")
            # One actual skeletal output per authored source mesh; ambiguity
            # is not silently resolved by choosing a random unrelated part.
            if len(meshes)!=1:
                raise RuntimeError("Native importer returned ambiguous original skeletal parts: "+target)
            parts.extend(meshes)
            status["assets"].extend(meshes)
        pkg="/Game/NoWorld/ContentFactory/Appearances/DA_"+suffix
        if not hasattr(unreal,"OGNativeContentFactoryEditorLibrary"):
            raise RuntimeError("Native authoring bridge not compiled: OGNativeContentFactoryEditorLibrary")
        result=unreal.OGNativeContentFactoryEditorLibrary.author_native_appearance(
            job["identity"],version,pkg,native["shared_motion_library"],parts,
            native["wardrobe"],native["rig"],native["anatomy"],
            native["wardrobe_state_field"],native.get("capabilities",[]))
        objpath=pkg+"."+pkg.rsplit("/",1)[-1]
        if result is not True or not unreal.EditorAssetLibrary.does_asset_exist(objpath):
            raise RuntimeError("Exact source appearance DataAsset was not authored/saved")
        status["appearance"]=objpath
        status["status"]="NATIVE_ORIGINAL_CHARACTER_SOURCE_ASSET_REGISTERED"
        merge_index({"identity":job["identity"],"version":version,"appearance":objpath,
                     "source_card_sha256":source["original_card_sha256"],
                     "world_class":"/Script/OfflineGame.OGContentPlayableCharacter",
                     "artwork":None,"needs_original_registered_gameplay":True})
    elif stage=="artwork":
        art=job.get("artwork",[])
        if not art:raise ValueError("No approved original artwork supplied")
        rows=[]
        for i,item in enumerate(art):
            if not item.get("approval_evidence"):
                raise ValueError("Artwork requires actual approval receipt")
            need_file(repo/item["approval_evidence"])
            where="/Game/NoWorld/ContentFactory/Art/"+normalized(job_id)+"/art_"+str(i).zfill(2)
            images=register_actual(item["path"],where,item["sha256"],"Texture2D")
            if len(images)!=1:
                raise RuntimeError("Actual original artwork texture import was ambiguous")
            rows.append({"path":images[0],"source_sha256":item["sha256"],
                         "purpose":item.get("purpose","original")})
            status["assets"].extend(images)
        status["artwork"]=rows
        status["status"]="ORIGINAL_ARTWORK_UASSET_IMPORTED"
        if job.get("identity") and job.get("version") and index.is_file():
            current=json.loads(index.read_text("utf-8-sig"))
            found=[c for c in current.get("characters",[]) if
                   c["identity"]==job["identity"] and c["version"]==job["version"]]
            if len(found)==1:
                # No replacement of previously accepted artwork.
                if found[0].get("artwork") and found[0]["artwork"]!=rows[0]["path"]:
                    raise FileExistsError("Existing approved original artwork preserved")
                found[0]["artwork"]=rows[0]["path"]
                write_json(index,current)
    write_json(receipt,status)
    unreal.log("NOWORLD_FACTORY_NATIVE_SOURCE_READY "+str(receipt))
except Exception as e:
    status["status"]="BLOCKED"
    status["error"]=str(e)
    status["traceback"]=traceback.format_exc()[-4000:]
    write_json(receipt,status)
    unreal.log_error("NOWORLD_FACTORY_NATIVE_IMPORT_BLOCKED "+str(e))
    raise
