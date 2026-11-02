"""Expand real installed CharaStudio/Mixamo source clips using their exact Unity
object IDs and compatible 93-bone humanoid subset. This is source-derived,
NOT accepted 3D visual/physical root-motion retargeting. Never replace originals.
"""
import argparse,collections,hashlib,json,pathlib,sys,gzip,re
R=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
sys.path.insert(0,str(R/"Scripts/production"))
import gate12_decode_genuine_koikatsu_studio_to_native_joint_curves_20261022 as core
manifest=R/"SourceAssets/Gate12/Animations20261021/ImportedGate09OriginalUnityCurves/GATE09_ALL_SOURCE_INTAKE_PROGRESS.json"
root=R/"SourceAssets/Gate12/Animations20261023/KoikatsuCalibratedNativeTracks"
planfile=R/"SourceAssets/Gate12/Animations20261023/KoikatsuExtendedSourcePlan.json"
def semantic(row):
  n=row["source_clip_name"];l=n.casefold();fam=row["source_family"]
  if "dance pose" in l:return "Interaction.DancePose","Interaction","emote",0
  if "dance" in l:return "Interaction.Dance","Interaction","emote",0
  if any(x in l for x in ("flying","flight","hover","takeoff")):return "Traversal.Flight","Traversal","flying",0
  if any(x in l for x in ("sword","dagger","slash","blade","katana")):return "Combat.Sword","Combat","combat",1
  if "wheelbarrow" in l or "formal bow" in l or "informal bow" in l:
    return "Interaction.Gesture","Interaction","emote",1
  if re.search(r"\b(?:shoot\w*|bow|archery|arrow\w*|pistol|rifle|gun\w*)\b",l):
    return "Combat.Ranged","Combat","combat",1
  if any(x in l for x in ("spell","magic","casting","aura")):return "Combat.Cast","Combat","combat",1
  if any(x in l for x in ("kick","punch","attack","fight","melee","uppercut","block","dodge")):return "Combat.Melee","Combat","combat",1
  if "reaction_death"==fam or any(x in l for x in ("death","knockdown","hit by","reacting")):return "Combat.Reaction","Combat","combat",2
  if "climb" in l:return "Traversal.Climb","Traversal","climbing",2
  if "swim" in l:return "Traversal.Swim","Traversal","swimming",2
  if any(x in l for x in ("run","walk","jump","crouch","crawl")):return "Traversal.Locomotion","Traversal","grounded",3
  if fam in ("combat_unarmed","melee_sword"):return "Combat.Melee","Combat","combat",3
  if fam in ("traversal","locomotion"):return "Traversal.Misc","Traversal","grounded",4
  if fam=="spell_magic":return "Combat.Cast","Combat","combat",4
  if fam=="reaction_death":return "Combat.Reaction","Combat","combat",4
  if fam=="emote":return "Interaction.Emote","Interaction","emote",5
  return None
def buildplan():
  rows=json.loads(manifest.read_text("utf8"))["source_files"]
  selected=[]
  for e in rows:
    if e["source_bundle_index"]<10:continue
    sem=semantic(e)
    if sem is None:continue
    context,effect,cap,priority=sem
    id=str(e["source_key"])
    alias="KKX_"+str(e["source_bundle_index"]).zfill(3)+"_"+id[:21]
    selected.append({
      "source_clip":alias,"original_clip_name":e["source_clip_name"],
      "source_original_path_id":e["original_clip_path_id"],
      "source_unity_bundle_index":e["source_bundle_index"],
      "source_unity_bundle_hash":e["source_hash"],
      "source_curve_path":e["raw_curve_path"],
      "source_curve_sha256":e["raw_sha256"],
      "native_file":alias+".genuine_unity_kk_stream_native_deltas.json.gz",
      "action":context+"."+alias,"context":effect,"capability":cap,
      "semantic_category":context,"source_family":e["source_family"],
      "priority":priority,"qa_only":True,"reduced_humanoid_rig":True})
  selected.sort(key=lambda x:(x["priority"],x["source_unity_bundle_index"],x["source_clip"]))
  assert len(set(x["source_clip"] for x in selected))==len(selected)
  return selected
def main(start,end):
  plan=buildplan()
  if planfile.exists():
    prev=json.loads(planfile.read_text("utf8"))["motions"]
    assert {x["source_clip"]:x["source_original_path_id"] for x in prev} == {x["source_clip"]:x["source_original_path_id"] for x in plan}, "Original clip identity set changed"
    # Frozen indices are used by native batch receipts. Semantic corrections
    # update metadata, never reorder or overwrite existing native assets.
    plan=prev
  else:
    planfile.write_text(json.dumps({"schema":"noworld.gate12.mixamo_koikatsu_93bone_QA.v1",
      "motions":plan,"approved_for_gameplay":False,"original_source_unchanged":True},indent=2)+"\n",encoding="utf8")
  assert 0<=start<end<=len(plan),(start,end,len(plan))
  root.mkdir(parents=True,exist_ok=True);core.OUT=root
  ref=core.source_bone_catalog()
  report=root/f"ExtendedSource_{start:04d}_{end:04d}.json"
  status={"start":start,"end":end,"results":[],"kind":"source_calibration_not_native"}
  for index in range(start,end):
    e=plan[index];f=root/e["native_file"]
    try:
      if f.is_file():
        with gzip.open(f,"rt",encoding="utf8") as z:j=json.load(z)
        assert j["source_path_id"]==e["source_original_path_id"]
        result="PRESERVED"
      else:
        core.convert(e["original_clip_name"],ref,
          source_row={"source_clip_name":e["original_clip_name"],
            "source_bundle_index":e["source_unity_bundle_index"],
            "raw_curve_path":e["source_curve_path"],"source_hash":e["source_unity_bundle_hash"]},
          output_alias=e["source_clip"],rig_family="mixamo_koikatsu_compatible")
        result="CALIBRATED"
    except Exception as ex:
      result="FAILED_"+type(ex).__name__+"_"+str(ex)[:180]
    status["results"].append({"index":index,"source":e["original_clip_name"],
      "alias":e["source_clip"],"result":result})
    report.write_text(json.dumps(status,indent=2)+"\n",encoding="utf8")
    print("EXTENDED_SOURCE",index,e["original_clip_name"],result,flush=True)
  print("EXTENDED_SOURCE_BATCH",start,end,"complete",
        sum(z["result"] in ("CALIBRATED","PRESERVED") for z in status["results"]),
        "of",end-start,"overall_plan",len(plan),flush=True)
if __name__=="__main__":
  parser=argparse.ArgumentParser()
  parser.add_argument("--start",type=int,required=True)
  parser.add_argument("--end",type=int,required=True)
  a=parser.parse_args();main(a.start,a.end)
