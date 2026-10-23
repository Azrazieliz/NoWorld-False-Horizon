import unreal,pathlib,gzip,json
r=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
p=r/"SourceAssets/Gate12/Animations20261023/KoikatsuExtendedSourcePlan.json"
rows=json.loads(p.read_text())["motions"]
for state in ("CLOTHED","NUDE"):
 sk=unreal.load_asset(f"/Game/NoWorld/Characters/Luminous/Alter/NativeOriginal{state.title()}/SK_Luminous_Alter_{state}_OriginalPart_00_Skeleton")
 assert sk
 names=set(str(x) for x in sk.get_reference_pose().get_bone_names())
 if state=="CLOTHED":
  print("MIXAMO_TARGET_FINGERS",sorted(x for x in names if any(y in x.casefold() for y in ("index","thumb","middle","ring","pinky","little","toe","waist","foot")))[:180])
 for row in rows[:3]:
  src=r/"SourceAssets/Gate12/Animations20261023/KoikatsuCalibratedNativeTracks"/row["native_file"]
  if not src.exists():continue
  with gzip.open(src,"rt") as f:j=json.load(f)
  matched={x["target_bone"] for x in j["bones"] if x["target_bone"] in names}
  absent=sorted({x["target_bone"] for x in j["bones"]}-matched)
  essential={"Center","Hips","Spine","Chest","Upper-Chest","Neck","Head",
     "Left-arm","Right-arm","Left-elbow","Right-elbow","Left-wrist","Right-wrist",
     "Left-leg","Right-leg","Left-knee","Right-knee","Left-ankle","Right-ankle"}
  print("MIXAMO_UE_SKELETON",state,"source",j["original_source_clip"],
    "source_tracks",len(j["bones"]),"target_matches",len(matched),
    "absent_essentials",sorted(essential-matched),"unmapped_35",absent[:40])
