"""Repair only new QA-stage derivative tracks using measured Alter finger
bone names. Never touches Unity sources, old production motions or approved UAssets.
A byte-preserved pre-correction copy is retained for inspection/rollback.
"""
import gzip,json,pathlib,shutil,sys
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
sys.path.insert(0,str(R/"Scripts/production"))
import gate12_decode_genuine_koikatsu_studio_to_native_joint_curves_20261022 as core
base=R/"SourceAssets/Gate12/Animations20261023/KoikatsuCalibratedNativeTracks"
plan=json.loads((R/"SourceAssets/Gate12/Animations20261023/KoikatsuExtendedSourcePlan.json").read_text())["motions"]
backup=R/"SourceAssets/Gate12/Animations20261023/KoikatsuMixamoPreMeasuredRetarget"
backup.mkdir(exist_ok=True,parents=True)
verified=0
for row in plan[:45]:
    path=base/row["native_file"]
    if not path.is_file():continue
    with gzip.open(path,"rt",encoding="utf8")as f:data=json.load(f)
    if data.get("measured_unreal_finger_mapping")==True:
        verified+=1
        continue
    assert data["source_path_id"]==row["source_original_path_id"]
    dest=backup/row["native_file"]
    if not dest.is_file():shutil.copy2(path,dest)
    changes=0
    for track in data["bones"]:
        new=core.EXTRA.get(track["source_bone"],track["target_bone"])
        if new!=track["target_bone"]:
            track["target_bone"]=new
            changes+=1
    names=[t["target_bone"] for t in data["bones"]]
    assert len(names)==len(set(names)), "Duplicate target bone after mapping"
    assert changes>=25,(row["source_clip"],changes)
    data["measured_unreal_finger_mapping"]=True
    data["measured_mapping_new_bones"]=changes
    # Atomic publication only after validating original identity and uniqueness
    tmp=path.with_suffix(".gz.new")
    with gzip.open(tmp,"wt",encoding="utf8",compresslevel=4)as f:json.dump(data,f,separators=(",",":"))
    tmp.replace(path)
    verified+=1
    print("MIXAMO_BONE_MAPPING",row["source_clip"],changes,flush=True)
print("MIXAMO_BONE_MAPPING_COMPLETE",verified,flush=True)
