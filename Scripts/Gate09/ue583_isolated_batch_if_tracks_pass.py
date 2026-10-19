"""Gate09 isolated UE5.8.3 post-import track diagnostic and conditional batch.
No authoritative Gate12 write or root/export pipeline replacement.
"""
import unreal,os,hashlib,json,traceback,time
ROOT=r"D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019"
SOURCES=r"D:\UnrealProjects\Gate09_Protected_Production_20261016\UnrealInterchange_20261019"
MAN=os.path.join(SOURCES,"INTERCHANGE_MANIFEST.json")
TARGET="/Game/Experimental/LuminousValentineV3/CLOTHED/Parts/Mesh000Prim000/Mesh_00_Prim_00/SkeletalMeshes/Mesh_00_Prim_00_Skeleton.Mesh_00_Prim_00_Skeleton"
FIRST="/Game/Gate09_QA_20261019/TestSingle/G09_LuminousV3_Idle_01_Thoughtful"
REPORT=os.path.join(ROOT,"Saved","Gate09QA","unreal_02_track_verified_batch.json")
r={"status":"STARTED","engine":str(unreal.SystemLibrary.get_engine_version()),"project":str(unreal.Paths.get_project_file_path()),
"initial_import":FIRST,"production_modified":False,"records":[],"artistic_validation":False}
def save():
 os.makedirs(os.path.dirname(REPORT),exist_ok=True)
 with open(REPORT,"w",encoding="utf8") as f:json.dump(r,f,indent=2,default=str)
def inspect(a):
 if not isinstance(a,unreal.AnimSequence):raise ValueError("Not an AnimSequence")
 mod=a.get_editor_property("controller").get_model_interface()
 names=[str(n) for n in mod.get_bone_track_names()]
 skeleton=a.get_editor_property("skeleton")
 refs={str(n) for n in skeleton.get_reference_pose().get_bone_names()}
 row={"asset":a.get_path_name(),"duration_s":float(a.get_play_length()),"skeleton":skeleton.get_path_name(),
      "bones":len(refs),"num_tracks":len(names),"sample_tracks":names[:38],
      "mapped_onto_target_count":len(refs & set(names)),"unmapped_track_sample":sorted(set(names)-refs)[:32]}
 try:row["model_frame_count"]=int(mod.get_number_of_frames())
 except Exception as e:row["frame_error"]=str(e)
 try:row["frame_rate"]=str(mod.get_frame_rate())
 except Exception as e:row["rate_error"]=str(e)
 try:
  tracks=mod.get_bone_animation_tracks()
  if len(tracks):
   row["track_struct_methods"]=[x for x in dir(tracks[0]) if "key" in x or "data" in x or "name" in x][:20]
   row["track_first_repr"]=str(tracks[0])[:500]
 except Exception as e:row["track_struct_error"]=str(e)
 return row
save()
try:
 assert "Gate09_Unreal583_Animation_QA_20261019" in r["project"]
 expected=json.loads(open(MAN,"r",encoding="utf8").read())["files"]
 assert len(expected)==4
 seq=unreal.load_asset(FIRST)
 first=inspect(seq);r["initial_diagnosis"]=first;save()
 if first["num_tracks"]<20:raise RuntimeError("FAILED_IMPORT_NO_ANIM_TRACKS")
 if first["mapped_onto_target_count"]<20:raise RuntimeError("FAILED_SKELETON_TRACK_NAME_MISMATCH")
 if first.get("model_frame_count",0)<60:raise RuntimeError("FAILED_FBX_ANIMATION_SAMPLING")
 if not 4.6<=first["duration_s"]<=5.2:raise RuntimeError("FAILED_FIRST_CLIP_TIMING")
 r["status"]="FIRST_UNREAL_NATIVE_ANIMATION_DATA_PASS";save()
 target=unreal.load_asset(TARGET)
 assert isinstance(target,unreal.Skeleton)
 for row in expected:
  f=os.path.join(SOURCES,row["file"])
  sha=hashlib.sha256(open(f,"rb").read()).hexdigest()
  assert sha==row["sha256"],"fbx hash changed"
  if row["source"]=="f_tachi_00_02":
   r["records"].append({**first,"filename":row["file"],"sha256":sha})
   save();continue
  task=unreal.AssetImportTask();task.filename=f
  task.destination_path="/Game/Gate09_QA_20261019/NativeKoikatsu"
  task.automated=True;task.save=True;task.replace_existing=False
  ui=unreal.FbxImportUI();ui.import_mesh=False;ui.import_as_skeletal=True
  ui.import_animations=True;ui.automated_import_should_detect_type=False
  ui.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION
  ui.skeleton=target
  task.options=ui
  unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
  paths=[str(x) for x in task.imported_object_paths]
  if len(paths)!=1:raise RuntimeError("FBX import did not produce exactly 1 asset: "+str(paths))
  a=unreal.load_asset(paths[0])
  rec=inspect(a)
  if rec["skeleton"]!=TARGET or rec["mapped_onto_target_count"]<20 or rec.get("model_frame_count",0)<80:raise RuntimeError("Unreal import failed source track guard: "+str(rec))
  rec["filename"]=row["file"];rec["sha256"]=sha
  r["records"].append(rec);save()
  unreal.log("GATE09_ANIM_IMPORT_ACCEPTED="+row["file"]+" "+str(rec["num_tracks"]))
 r["status"]="PASS_ALL_FOUR_NATIVE_UNREAL_SEQUENCES_TRACKS_TIMING_SKELETON"
except Exception:
 r["status"]="FAIL_BATCH_REQUIRES_CORRECTION";r["error"]=traceback.format_exc()
finally:
 save();unreal.log("GATE09_UNREAL_BATCH="+r["status"])
