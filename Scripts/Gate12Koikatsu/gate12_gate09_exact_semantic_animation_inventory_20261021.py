"""Gate09 authoritative clip inventory -> Gate12 exact-source intake ledger.
This is deliberately NOT a conversion to playback. 2741 objects describe
Unity source observations, including duplicates, not approved animations.
Source identity is (bundle path, exact inner clip name, length, fps, bindings).
Prior accepted native assets are re-used; no new Luminous version or license.
"""
import json,hashlib,collections,pathlib
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
src=pathlib.Path(r"D:\UnrealProjects\Gate09_Protected_Production_20261016\G09_InstalledAnimationInventory_ReadOnly_20261020.json")
out=R/"SourceAssets/Gate12/Animations20261020/Gate09_SourceClassification_ExactActions_20261021.json"
d=json.loads(src.read_text(encoding="utf-8"))
assert d["observed_clip_objects"]==2741 and d["bundle_records_with_clips"]==47
entries=[];exact=collections.defaultdict(list);by_family=collections.Counter()
for bundle_index,b in enumerate(d["bundles"]):
 source=pathlib.Path(b["path"])
 # Preserve asset-local source metadata; do not copy/export restricted zipmods.
 for clip in b["clips"]:
  identity=[str(source),clip["name"],clip.get("seconds"),clip.get("fps"),clip.get("binding_count")]
  key=hashlib.sha256(json.dumps(identity,ensure_ascii=False,separators=(",",":")).encode("utf8")).hexdigest()
  category=clip.get("category","other")
  by_family[category]+=1
  item={"source_content_key":key,"source_category":b["source"],
        "source_bundle_path":str(source),"inner_clip_name":clip["name"],
        "source_seconds":clip.get("seconds"),"source_fps":clip.get("fps"),
        "source_binding_count":clip.get("binding_count"),
        "motion_family":category,"status":"SOURCE_ONLY",
        "reason":"Unretargeted original Unity motion; no rig, pairing, native playback or artistic acceptance verified"}
  exact[key].append(len(entries))
  entries.append(item)
for ids in exact.values():
 if len(ids)>1:
  for i in ids[1:]:
   entries[i]["status"]="SOURCE_ONLY_DUPLICATE_OBSERVATION"
   entries[i]["reason"]="Identical exact source bundle/clip metadata already inventoried; do not import twice"
native=json.loads((R/"Saved/Gate12AnimationAcceptance/Gate12_ActualFourNativeMotion_GAME_COLD_20261020.json").read_text())
assert native["status"]=="PASS_GATE12_GAME_FOUR_REAL_NATIVE_KOIKATSU_SOURCE_CLIPS_COLD"
routes=[
  ("Flight_Start","luminous_base.traversal.flight_takeoff","World Mode / MovementMode=Flying entry, airborne movement","BOUND_PENDING_VISUAL"),
  ("Flight_Hover","luminous_base.traversal.flight_hover","World Mode / Flying and near-zero velocity","BOUND_PENDING_VISUAL"),
  ("Swimming_Stroke","luminous_base.traversal.swim_stroke","World Mode / MovementMode=Swimming","BOUND_PENDING_VISUAL"),
  ("Dance_Performance","luminous_base.emote.dance_qa","World Mode / explicit Dance emote, stationary+grounded+Clothed","REVIEW_REQUIRED")]
mapped=[]
for raw,action,trigger,status in routes:
 match=[x for x in native["events"] if x["role"]==raw]
 assert len(match)==1
 e=match[0]
 assert e["tracks"]==975 and e["actual_valid_skeleton"] and e["duration_s"]>0
 mapped.append({"action":action,"native_animsequence":e["asset"],
                "verified_bone_tracks":e["tracks"],"duration_seconds":e["duration_s"],
                "semantic_trigger":trigger,
                "native_owner":"AOGLuminousV3PlayableCharacter",
                "runtime_entry":"ResolveAuthoredMotionClip" if raw!="Dance_Performance" else "PlayGate09AuthoredEmote",
                "rig":"LuminousV3_Base_CLOTHED_975",
                "wardrobe":"clothed only",
                "status":status,
                "visual_acceptance":"NOT_VERIFIED_PIE",
                "evidence":"Saved/Gate12AnimationAcceptance/Gate12_ActualFourNativeMotion_GAME_COLD_20261020.json"})
for emote in ("Thoughtful","Greeting","Expressive","Bow"):
 mapped.append({"action":"luminous_base.emote."+emote.lower(),
                "semantic_trigger":"World Mode / explicit "+emote+" emote, grounded+stationary+clothed",
                "native_owner":"AOGLuminousV3PlayableCharacter",
                "runtime_entry":"PlayGate09AuthoredEmote",
                "rig":"LuminousV3_Base_CLOTHED_975",
                "status":"BOUND_PENDING_VISUAL",
                "visual_acceptance":"NOT_VERIFIED_PIE"})
result={"schema":"noworld.gate12.gate09_exact_source_semantic_import_v1",
        "authority":"User-supplied Gate09 Animation Source Import & Runtime Integration handoff, 21 Oct 2026",
        "source_inventory":str(src),
        "source_observations":len(entries),
        "source_bundles":len(d["bundles"]),
        "distinct_exact_source_observations":len(exact),
        "duplicate_exact_metadata_observations":len(entries)-len(exact),
        "counts_by_original_source_classifier":dict(by_family),
        "native_975_koikatsu_sequences":len(native["events"]),
        "known_routed_actions":mapped,
        "source_only":entries,
        "excluded_approvals":{
          "Dancing 2":"Technical QA only; user questioned visual quality; not accepted",
          "Unity source collection":"Clip inventory count cannot become gameplay count",
          "external CC0 Gres/Quaternius":"SOURCE_ONLY until explicit retarget on approved 975 skeleton and visual in-game acceptance",
          "adult-context paired actions":"SOURCE_ONLY until correct paired actor rig, choreography, consent/context, wardrobe and gameplay event supported",
          "experimental unverified artwork":"Not imported as approved game content"},
        "readiness":"GATE09_EXACT_SOURCE_INVENTORY_AND_EIGHT_KNOWN_ACTION_ROUTING_LEDGER_ONLY"}
out.write_text(json.dumps(result,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
print("CREATED_AUTHORITATIVE_SOURCE_INVENTORY",out)
print("OBSERVATIONS",len(entries),"DISTINCT_EXACT_IDENTITIES",len(exact),"KNOWN_ROUTED_ACTIONS",len(mapped))
