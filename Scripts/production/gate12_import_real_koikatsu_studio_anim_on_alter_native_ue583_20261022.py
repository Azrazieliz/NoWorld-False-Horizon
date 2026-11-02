"""Author one actual UE AnimSequence from decoded installed Koikatsu Studio
AnimationClip. Native original user-approved Alter skeleton, 30fps source pose,
no reused Base rig, no fabricated animation or auto-gameplay enable.
Explicitly technical preview: retarget axis and translation not visually approved.
"""
import unreal,os,json,gzip,pathlib,hashlib,math,traceback
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
STATE=os.environ["G12_KK_SOURCE_STATE"]
CLIP=os.environ["G12_KK_SOURCE_CLIP"]
# Backward-compatible extension: other actual Koikatsu character skeletons
# reuse the ORIGINAL source motion but receive an independent native derivative.
FORM=os.environ.get("G12_KK_SOURCE_FORM","Alter")
assert FORM in ("Alter","Awakened","Berserker","Grand")
assert STATE in ("CLOTHED","UNDERWEAR","NUDE")
assert CLIP.replace("_","").isalnum()
INPUT=R/"Saved/Gate12Readiness/KoikatsuGenuineNativeStudioMotion"/(CLIP+".genuine_unity_kk_stream_native_deltas.json.gz")
if not INPUT.exists():
 INPUT=R/"SourceAssets/Gate12/Animations20261023/KoikatsuCalibratedNativeTracks"/(CLIP+".genuine_unity_kk_stream_native_deltas.json.gz")
TARGET_DIR=f"/Game/NoWorld/Characters/Luminous/{FORM}/KoikatsuOriginalStudioMotionQA"
NAME=f"AN_G12_KK_Original_{STATE}_{CLIP}" if FORM=="Alter" else f"AN_G12_KK_{FORM}_{STATE}_{CLIP}"
RECEIPT=R/"Saved/Gate12Readiness"/(f"GATE12_NATIVE_KOIKATSU_STUDIO_{STATE}_{CLIP}_20261022.json" if FORM=="Alter" else f"GATE12_{FORM}_{STATE}_{CLIP}_ORIGINAL_NATIVE_20261023.json")
rep={"state":STATE,"source_clip":CLIP,
     "status":"STARTED","original_koikatsu_sourced":True,
     "source_geometry_unchanged":True,"prior_mocap_unchanged":True,
     "not_gameplay_approved":True,"no_rejected_dance_reintroduced":True,
     "rig_mapping_visual_approval":"NOT_YET_CHECKED",
     "native_original_pose_delta_only":True}
try:
 assert R.name in unreal.Paths.get_project_file_path()
 assert INPUT.is_file()
 with gzip.open(INPUT,"rt",encoding="utf8") as f:source=json.load(f)
 assert source["status"]=="SOURCE_AUTHENTIC_QUATERNION_TRACKS_CALIBRATED_FOR_NATIVE_UE_REST_BASIS"
 assert source["clip"]==CLIP and source["relative_source_pose_only"] is True
 assert source["animated_rotation_bone_count"]>=10
 min_bones=80 if source.get("rig_family")=="mixamo_koikatsu_compatible" else 140
 assert source["resolved_rotation_bone_count"]>=min_bones
 frame_count=source["frame_count"]
 assert 5<=frame_count<15000 and source["frame_rate"]==30.0
 path=f"/Game/NoWorld/Characters/Luminous/{FORM}/NativeOriginal{STATE.title()}/SK_Luminous_{FORM}_{STATE}_OriginalPart_00_Skeleton"
 sk=unreal.load_asset(path)
 assert isinstance(sk,unreal.Skeleton)
 pose=sk.get_reference_pose()
 bone_names=set(str(b)for b in pose.get_bone_names())
 assert len(bone_names)==((1123 if STATE=="NUDE" else 1131) if FORM=="Alter" else 886)
 mapped=[]
 for track in source["bones"]:
  if track["target_bone"] not in bone_names:continue
  assert len(track["deltas"])==frame_count
  mapped.append(track)
 distinct={x["target_bone"]for x in mapped}
 assert len(mapped)==len(distinct) and len(mapped)>=min_bones,("Native rig compatibility too low",len(mapped))
 changed=sum(x["angular_span_deg"]>.05 for x in mapped)
 assert changed>=10
 output=TARGET_DIR+"/"+NAME
 assert not unreal.EditorAssetLibrary.does_asset_exist(output),"Never overwrite an existing authored native motion"
 factory=unreal.AnimSequenceFactory()
 factory.set_editor_property("target_skeleton",sk)
 unreal.EditorAssetLibrary.make_directory(TARGET_DIR)
 seq=unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME,TARGET_DIR,unreal.AnimSequence,factory)
 assert isinstance(seq,unreal.AnimSequence) and seq.get_editor_property("skeleton")==sk
 ctl=seq.get_editor_property("controller")
 ctl.set_frame_rate(unreal.FrameRate(numerator=30,denominator=1),should_transact=False)
 ctl.set_number_of_frames(unreal.FrameNumber(value=frame_count-1),should_transact=False)
 written=[]
 ctl.open_bracket("Exact installed Unity KK Studio clip "+CLIP,False)
 try:
  for t in mapped:
   bone=t["target_bone"]
   ref=pose.get_bone_pose(unreal.Name(bone),unreal.AnimPoseSpaces.LOCAL)
   q=ref.rotation
   p=unreal.Vector(ref.translation.x,ref.translation.y,ref.translation.z)
   s=unreal.Vector(ref.scale3d.x,ref.scale3d.y,ref.scale3d.z)
   native=[]
   for d in t["deltas"]:
    qd=unreal.Quat(float(d[0]),float(d[1]),float(d[2]),float(d[3]))
    qq=q * qd
    qq.normalize()
    native.append(qq)
   assert bool(ctl.add_bone_track(unreal.Name(bone),False)),bone
   assert bool(ctl.set_bone_track_keys(unreal.Name(bone),[p]*frame_count,native,[s]*frame_count,False)),bone
   written.append(bone)
 finally:
  ctl.close_bracket(False)
 assert len(written)>=min_bones
 actual=set(str(v).casefold() for v in ctl.get_model_interface().get_bone_track_names())
 written_folded=set(v.casefold() for v in written)
 rep["written_track_count"]=len(written_folded)
 rep["model_track_count"]=len(actual)
 rep["missing_bone_names_in_model"]=sorted(written_folded-actual)[:45]
 rep["unexpected_model_bones"]=sorted(actual-written_folded)[:45]
 # Unreal FName's case-insensitive identity is authoritative; comparing
 # its display names case-sensitively previously gave 187 false mismatches.
 assert len(actual)>=min_bones and actual==written_folded,rep
 assert abs(seq.get_play_length()-(frame_count-1)/30.0)<.07
 assert unreal.EditorAssetLibrary.save_loaded_asset(seq)
 rep.update({"native_animsequence":seq.get_path_name(),"original_source_curve":source["source_curve_path"],
             "source_unity_path_id":source["source_path_id"],
             "source_bundle_sha256":source["source_sha256"],
             "source_frames":frame_count,"duration_s":seq.get_play_length(),
             "bone_tracks":len(written),"moving_bone_tracks":changed,
             "native_skeleton":sk.get_path_name(),
             "unmapped_rotation_bones":len(source["bones"])-len(written),
             "status":"TRUE_ORIGINAL_KK_STUDIO_CLIP_IMPORTED_AS_NATIVE_ANIMSEQUENCE_PRE_VISUAL"})
except Exception:
 rep["status"]="NATIVE_REAL_KK_SOURCE_CLIP_NOT_YET_ACCEPTED"
 rep["error"]=traceback.format_exc()[-5200:]
finally:
 RECEIPT.parent.mkdir(parents=True,exist_ok=True)
 RECEIPT.write_text(json.dumps(rep,indent=2,default=str)+"\n",encoding="utf8")
 unreal.log("GATE12_KK_NATIVE_STUDIO_"+STATE+"_"+CLIP+"="+rep["status"])
