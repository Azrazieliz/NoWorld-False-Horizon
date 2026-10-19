"""Gate09 isolated UE 5.8.3 true imported facial morph playback proof.
Use native unchanged Luminous Body mesh; non-explicit blink/smile source morph.
No adult motion scene, no Gate12 production mutation.
"""
import unreal,os,json,traceback,math,time,re
ROOT=r"D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019"
REPORT=os.path.join(ROOT,"Saved","Gate09QA","unreal_04_authentic_source_face_morphs.json")
MESH="/Game/Experimental/LuminousValentineV3/CLOTHED/Parts/Mesh003Prim000/Mesh_03_Prim_00/SkeletalMeshes/Mesh_03_Prim_00.Mesh_03_Prim_00"
OUT="/Game/Gate09_QA_20261019/AuthenticFaceSource"
r={"status":"STARTED","engine":str(unreal.SystemLibrary.get_engine_version()),"mesh":MESH,
   "source_gate12_modified":False,"adult_scene_rendered":False,"technical_only":True}
def save():
 os.makedirs(os.path.dirname(REPORT),exist_ok=True)
 with open(REPORT,"w",encoding="utf8") as f:json.dump(r,f,indent=2,default=str)
save()
try:
 assert "Gate09_Unreal583_Animation_QA_20261019" in str(unreal.Paths.get_project_file_path())
 mesh=unreal.load_asset(MESH)
 assert isinstance(mesh,unreal.SkeletalMesh),"Original body mesh missing"
 sk=mesh.get_editor_property("skeleton")
 r["skeleton"]=sk.get_path_name()
 r["skeleton_bones"]=len(sk.get_reference_pose().get_bone_names())
 morphs=[str(x.get_name()) for x in mesh.get_editor_property("morph_targets")]
 r["native_morph_target_count"]=len(morphs)
 r["example_morphs"]=morphs[:30]
 def norm(s):return re.sub("[^a-z0-9]","",s.casefold())
 names={norm(x):x for x in morphs}
 wanted=["Eye Closed","Mouth Distorted Smile, Closed"]
 chosen=[names.get(norm(x)) for x in wanted]
 r["requested"]=wanted;r["exact_morph_matches"]=chosen
 if any(x is None for x in chosen):
  r["status"]="FACE_MORPH_SOURCE_MISSING_IN_UNREAL";save()
  raise RuntimeError("Actual source morphs missing in imported native UE part")
 label="AN_G09_Luminous_NativeBlinkSmile_SourceProof"
 uri=OUT+"/"+label
 if unreal.EditorAssetLibrary.does_asset_exist(uri):raise FileExistsError(uri)
 factory=unreal.AnimSequenceFactory()
 factory.set_editor_property("target_skeleton",sk)
 seq=unreal.AssetToolsHelpers.get_asset_tools().create_asset(label,OUT,unreal.AnimSequence,factory)
 assert isinstance(seq,unreal.AnimSequence),"native anim asset creation failed"
 ctl=seq.get_editor_property("controller")
 ctl.set_frame_rate(unreal.FrameRate(numerator=30,denominator=1),False)
 ctl.set_number_of_frames(unreal.FrameNumber(value=60),False)
 values={}
 for morph in chosen:
  ident=unreal.AnimationCurveIdentifier()
  ident.set_curve_identifier(unreal.Name(morph),unreal.RawCurveTrackTypes.RCT_FLOAT)
  if not ctl.add_curve(ident,curve_flags=4,should_transact=False):
   raise RuntimeError("Could not create source morph curve "+morph)
  keys=[]
  for frame in range(61):
   t=frame/30.
   if morph==chosen[0]:
    # Exact native eyelid shape target; 0.35s blink, 1.30s blink.
    v=max(max(0,1-abs(t-0.35)/0.16),max(0,1-abs(t-1.30)/0.16))
   else:
    # Original native mouth smile target: ease in, hold, ease out.
    if t<0.45:v=0.0
    elif t<0.95:
     x=(t-.45)/.50;v=0.80*(x*x*(3-2*x))
    elif t<1.5:v=0.80
    else:
     x=min(1,(t-1.5)/.5);v=0.80*(1-(x*x*(3-2*x)))
   keys.append(unreal.RichCurveKey(time=t,value=v))
  if not ctl.set_curve_keys(ident,keys,False):
   raise RuntimeError("Could not key original morph "+morph)
  sk.set_curve_meta_data_morph_target(unreal.Name(morph),True)
  values[morph]={"samples":len(keys),"minimum":min(float(k.value) for k in keys),
                 "maximum":max(float(k.value) for k in keys)}
 if not unreal.EditorAssetLibrary.save_loaded_asset(sk):raise RuntimeError("Could not save isolated skeleton morph metadata")
 if not unreal.EditorAssetLibrary.save_loaded_asset(seq):raise RuntimeError("Could not persist UE morph sequence")
 r["asset"]=seq.get_path_name()
 r["frame_count"]=ctl.get_model_interface().get_number_of_frames()
 r["duration"]=seq.get_play_length()
 r["curves"]=values
 r["status"]="PASS_NATIVE_UNREAL_ORIGINAL_LUMINOUS_FACE_CURVES_SOURCE_PROOF"
except Exception:
 r["exception"]=traceback.format_exc()
 if r["status"]=="STARTED":r["status"]="FAILED_AUTHENTIC_FACE_SOURCE_TEST"
finally:
 r["finished_utc"]=time.strftime("%Y-%m-%dT%H:%M:%SZ",time.gmtime())
 save();unreal.log("G09_UE_NATIVE_FACE_QA="+r["status"])
