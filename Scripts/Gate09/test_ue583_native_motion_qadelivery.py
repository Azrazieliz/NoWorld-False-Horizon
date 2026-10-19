"""Gate09 verified isolated Unreal 5.8.3 native four-clip QA and source preservation."""
import json,hashlib,pathlib,unittest,zipfile
R=pathlib.Path(r"D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019")
P=pathlib.Path(r"D:\UnrealProjects\Gate09_Protected_Production_20261016")
G12=pathlib.Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
def read(n):return json.loads((R/"Saved"/"Gate09QA"/n).read_text(encoding="utf8"))
def sha(p):
 h=hashlib.sha256()
 with p.open("rb") as f:
  for chunk in iter(lambda:f.read(1024*1024),b""):h.update(chunk)
 return h.hexdigest()
class G09UnrealCold(unittest.TestCase):
 def test_ue583_all_four_assets_are_persisted(self):
  a=read("unreal_02_track_verified_batch.json")
  self.assertEqual(a["status"],"PASS_ALL_FOUR_NATIVE_UNREAL_SEQUENCES_TRACKS_TIMING_SKELETON")
  self.assertEqual(len(a["records"]),4)
  for row in a["records"]:
   p=R/"Content"/(row["asset"].split(".")[0].replace("/Game/","")+".uasset")
   self.assertTrue(p.is_file(),p)
   self.assertEqual(row["num_tracks"],975)
   self.assertEqual(row["bones"],975)
   self.assertEqual(row["frame_rate"].split("numerator: ")[1][:2],"30")
   self.assertGreater(row["model_frame_count"],130)
   self.assertEqual(sha(P/"UnrealInterchange_20261019"/row["filename"]),row["sha256"])
 def test_fresh_cold_loaded_native_animation_poses(self):
  a=read("unreal_03_cold_pose_qa.json")
  self.assertEqual(a["status"],"PASS_FOUR_NATIVE_UNREAL_COLD_LOAD_AND_CASEFOLD_BONE_MAPPING")
  self.assertEqual(len(a["clips"]),4)
  for row in a["clips"]:
   self.assertEqual(row["case_insensitive_bone_matches"],975)
   self.assertEqual(row["skeleton_bones"],975)
   self.assertGreater(row["sampled_bone_pose_changes"]["Head"],0.015)
   self.assertGreater(row["sampled_bone_pose_changes"]["Center"],0.015)
 def test_g12_canonical_meshes_are_copies_only(self):
  folders=list((R/"Content"/"Experimental").rglob("*.uasset"))
  self.assertEqual(len(folders),6)
  for dest in folders:
   source=G12/"Content"/dest.relative_to(R/"Content")
   self.assertTrue(source.is_file())
   self.assertEqual(sha(source),sha(dest))
 def test_portable_qa_archive_hash_and_integrity(self):
  zipfilepath=P/"GATE09_UNREAL583_NATIVE_FOUR_CLIPS_20261019.zip"
  self.assertEqual(sha(zipfilepath),"c197b1ae2445ec454656b92372ae2b8834c62121c07a59065ae3b356be0ea57d")
  with zipfile.ZipFile(zipfilepath) as archive:
   self.assertIsNone(archive.testzip())
   self.assertEqual(len([x for x in archive.namelist() if x.endswith(".uasset")]),10)
 def test_no_unreal_android_fidelity_overclaim(self):
  a=read("unreal_02_track_verified_batch.json")
  self.assertFalse(a["artistic_validation"])
  self.assertFalse(a["production_modified"])
  self.assertTrue(read("unreal_03_cold_pose_qa.json")["read_only"])
if __name__=="__main__":unittest.main(verbosity=2)
