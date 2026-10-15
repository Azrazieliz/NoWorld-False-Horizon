"""Native UE5.8.3 isolated AnimSequence event authoring: 2 timed reusable sound hook notifies.
Sandbox only, no gameplay Gate or real character assets changed.
"""
import unreal,json,os,traceback
BASE=r"D:\UnrealProjects\NW_BlenderInterop_Sandbox_20261015"
r={"engine":unreal.SystemLibrary.get_engine_version(),"status":"STARTED"}
try:
 src="/Game/NW_BlenderImportTests/Mount_Walk_AnimOnly/NW_Mount_Walk_Test_AnimOnly.NW_Mount_Walk_Test_AnimOnly"
 dst="/Game/NW_BlenderLearning/NW_AnimActionWithTimedEvents"
 assert unreal.EditorAssetLibrary.does_asset_exist(src),"Source AnimSequence absent"
 if unreal.EditorAssetLibrary.does_asset_exist(dst):
  unreal.EditorAssetLibrary.delete_asset(dst)
 ok=unreal.EditorAssetLibrary.duplicate_asset(src,dst)
 assert ok,"Duplicating animation asset failed"
 a=unreal.EditorAssetLibrary.load_asset(dst)
 assert a and a.get_class().get_name()=="AnimSequence"
 tracks=["NW_Action_Cues"]
 for n in tracks:unreal.AnimationLibrary.add_animation_notify_track(a,n)
 events=[]
 for idx,(t,label) in enumerate([(0.22,"strike_windup_audio_hook"),(0.72,"impact_audio_hook")]):
  ev=unreal.AnimationLibrary.add_animation_notify_event(a,"NW_Action_Cues",t,unreal.AnimNotify_PlaySound)
  assert ev
  events.append({"label":label,"seconds":t,"notify_type":ev.get_class().get_name()})
 found=unreal.AnimationLibrary.get_animation_notify_events(a)
 names=[str(x.notify_name) for x in found]
 track_names=[str(t) for t in unreal.AnimationLibrary.get_animation_notify_track_names(a)]
 r["track_names"]=track_names;r["event_count"]=len(found)
 r["events"]=events;r["notify_names"]=names
 assert len(found)==2 and "NW_Action_Cues" in track_names
 assert unreal.EditorAssetLibrary.save_loaded_asset(a)
 reloaded=unreal.EditorAssetLibrary.load_asset(dst)
 assert len(unreal.AnimationLibrary.get_animation_notify_events(reloaded))==2
 r["asset_path"]=dst;r["asset_class"]=a.get_class().get_name()
 r["native_asset_saved"]=True
 r["status"]="PASS_TECHNICAL_TEST_ONLY"
except Exception:
 r["status"]="FAILED";r["exception"]=traceback.format_exc()
finally:
 with open(os.path.join(BASE,"ue_notify_authoring_results.json"),"w") as f:json.dump(r,f,indent=2)
 print("NOWORLD_NATIVE_NOTIFY="+json.dumps(r))
