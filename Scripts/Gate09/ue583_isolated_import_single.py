"""Gate09 Unreal 5.8.3 isolated source animation import probe.
Copied native target skeleton; zero mutation of Gate12/production content.
Test ONE genuine animation before importing the other three.
"""
import unreal,os,json,traceback,time,hashlib
ROOT=r"D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019"
FBX=r"D:\UnrealProjects\Gate09_Protected_Production_20261016\UnrealInterchange_20261019\G09_LuminousV3_Idle_01_Thoughtful.fbx"
TARGET="/Game/Experimental/LuminousValentineV3/CLOTHED/Parts/Mesh000Prim000/Mesh_00_Prim_00/SkeletalMeshes/Mesh_00_Prim_00_Skeleton.Mesh_00_Prim_00_Skeleton"
ALTERNATE="/Game/Experimental/Gate12Koikatsu/FullPrimitives/Mesh03Prim00/Mesh_03_Prim_00/SkeletalMeshes/Mesh_03_Prim_00_Skeleton.Mesh_03_Prim_00_Skeleton"
REPORT=os.path.join(ROOT,"Saved","Gate09QA","unreal_01_single_clip_probe.json")
r={"status":"STARTED","engine":str(unreal.SystemLibrary.get_engine_version()),
   "project":str(unreal.Paths.get_project_file_path()),"source":FBX,"tested_at":time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime()),
   "production_unmodified":True}
def save():
 os.makedirs(os.path.dirname(REPORT),exist_ok=True)
 with open(REPORT,"w",encoding="utf8") as h:json.dump(r,h,indent=2,default=str)
save()
try:
 assert "Gate09_Unreal583_Animation_QA_20261019" in r["project"],"wrong Unreal project"
 assert os.path.exists(FBX)
 r["fbx_sha256"]=hashlib.sha256(open(FBX,"rb").read()).hexdigest()
 r["available_target_skeletons"]=[]
 for name,assetpath in (("luminous_clothed",TARGET),("gate12_fixture",ALTERNATE)):
  s=unreal.EditorAssetLibrary.load_asset(assetpath)
  row={"name":name,"asset":assetpath,"loaded":bool(s),"class":s.get_class().get_name() if s else None}
  if s:
   try:
    pose=s.get_reference_pose()
    bn=[str(n) for n in pose.get_bone_names()]
    row["bone_count"]=len(bn)
    row["sample_bones"]=bn[:32]
    row["has_luminous_names"]={x:x in bn for x in ["Hips","Center","Left arm","Right arm","spine_01","pelvis"]}
   except Exception as e:row["bone_error"]=str(e)
  r["available_target_skeletons"].append(row)
  save()
 skeleton=unreal.load_asset(TARGET)
 assert isinstance(skeleton,unreal.Skeleton),"original Luminous native skeleton missing"
 task=unreal.AssetImportTask()
 task.filename=FBX
 task.destination_path="/Game/Gate09_QA_20261019/TestSingle"
 task.automated=True;task.save=True;task.replace_existing=False
 ui=unreal.FbxImportUI()
 ui.import_mesh=False;ui.import_as_skeletal=True;ui.import_animations=True
 ui.automated_import_should_detect_type=False
 ui.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION
 ui.skeleton=skeleton
 task.options=ui
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
 r["paths"]=[str(x) for x in task.imported_object_paths]
 r["assets"]=[]
 for p in r["paths"]:
  obj=unreal.load_asset(p)
  row={"path":p,"class":obj.get_class().get_name() if obj else None}
  if isinstance(obj,unreal.AnimSequence):
   sk=obj.get_editor_property("skeleton")
   row["skeleton"]=sk.get_path_name() if sk else None
   try:row["duration_s"]=float(obj.get_play_length())
   except Exception as exc:row["duration_error"]=str(exc)
   try:row["frame_rate"]=str(obj.get_sampling_frame_rate())
   except Exception as exc:row["frame_rate_error"]=str(exc)
   try:
    tracks=unreal.AnimationBlueprintLibrary.get_bone_track_names(obj)
    row["bone_track_count"]=len(tracks)
    row["sample_tracks"]=[str(x) for x in tracks[:32]]
   except Exception as exc:row["tracks_error"]=str(exc)
   try:row["sample_keys"]=int(obj.get_number_of_sampled_keys())
   except Exception as exc:row["keys_error"]=str(exc)
  r["assets"].append(row);save()
 seqs=[v for v in r["assets"] if v["class"]=="AnimSequence"]
 if len(seqs)!=1:raise RuntimeError("No genuine Unreal AnimSequence from imported FBX")
 if seqs[0].get("skeleton")!=TARGET:raise RuntimeError("Unexpected target skeleton")
 if seqs[0].get("duration_s",0)<3:raise RuntimeError("Invalid Unreal animation duration")
 if seqs[0].get("bone_track_count",0)<20:raise RuntimeError("Missing animated skeleton tracks")
 r["status"]="PASS_REAL_UNREAL_ANIMSEQUENCE_SKELETON_TRACKS_ISOLATED"
except Exception:
 r["status"]="FAIL_SINGLE_IMPORT";r["exception"]=traceback.format_exc()
finally:
 save();unreal.log("GATE09_NATIVE_UNREAL_IMPORT_PROBE="+r["status"])
