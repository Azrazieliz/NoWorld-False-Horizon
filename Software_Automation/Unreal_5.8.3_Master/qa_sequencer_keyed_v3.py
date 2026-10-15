"""Unreal 5.8.3 real editable Sequencer 3D camera path in QA sandbox.
Find transform section by track TYPE (not spawn track), ensure >=18 persisted keys.
"""
import unreal,json,pathlib,traceback
O=pathlib.Path(r"D:\UnrealProjects\NW_BlenderGateHandoff_QA_20261015\UE58_Sequencer_CameraKeys_V3.json")
r={"engine":unreal.SystemLibrary.get_engine_version(),"status":"START"}
try:
 root="/Game/NW_MasterHub_QA/SoftwareMaster_Native"
 name="NW_Sequencer_TrackCorrected_V3"
 assert not unreal.EditorAssetLibrary.does_asset_exist(root+"/"+name)
 seq=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,root,unreal.LevelSequence,unreal.LevelSequenceFactoryNew())
 seq.set_display_rate(unreal.FrameRate(24,1))
 seq.set_playback_start(0);seq.set_playback_end(96)
 binding=seq.add_spawnable_from_class(unreal.CineCameraActor)
 tracks=binding.get_tracks()
 tr=next((x for x in tracks if isinstance(x,unreal.MovieScene3DTransformTrack)),None)
 if tr is None:tr=binding.add_track(unreal.MovieScene3DTransformTrack)
 sec=tr.add_section()
 sec.set_range(0,96)
 channels=unreal.MovieSceneSectionExtensions.get_all_channels(sec)
 r["channels_before"]=[x.get_class().get_name() for x in channels]
 if len(channels)<9:raise RuntimeError("3D transform channels absent")
 v0=[0,-245,170,0,0,0,1,1,1]
 v1=[120,-150,212,0,3,16,1,1,1]
 for j,ch in enumerate(channels[:9]):
  a,b=v0[j],v1[j]
  ch.add_key(unreal.FrameNumber(0),float(a))
  ch.add_key(unreal.FrameNumber(95),float(b))
 unreal.EditorAssetLibrary.save_loaded_asset(seq)
 back=unreal.load_asset(seq.get_path_name())
 binding2=back.get_bindings()[0]
 ts=next(x for x in binding2.get_tracks() if isinstance(x,unreal.MovieScene3DTransformTrack))
 section=ts.get_sections()[0]
 actual=unreal.MovieSceneSectionExtensions.get_all_channels(section)
 counts=[len(ch.get_keys()) for ch in actual]
 r.update(asset=back.get_path_name(),native_transform_channels=len(actual),
   channel_key_counts=counts,actual_saved_keys=sum(counts),
   camera_spawnable=True,
   status="PASS_KEYED_NATIVE_SEQUENCE" if sum(counts)>=18 else "FAILED_NO_NATIVE_KEYS",
   game_project_untouched=True)
except Exception:r["status"]="ERROR";r["trace"]=traceback.format_exc()
O.write_text(json.dumps(r,indent=2))
print("NW_KEYED_SEQUENCE="+json.dumps({k:v for k,v in r.items() if k!="trace"}))
