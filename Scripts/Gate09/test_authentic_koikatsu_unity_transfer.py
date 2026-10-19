"""Gate09 genuine Koikatsu source/retarget receipts. No artistic or UE acceptance implied."""
import unittest,pathlib,json,gzip,hashlib,math
P=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
def read(name):return json.loads((P/name).read_text(encoding='utf8'))
def digest(file):
 h=hashlib.sha256()
 with pathlib.Path(file).open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
class Motion(unittest.TestCase):
 def test_real_game_native_clip_source(self):
  data=read('koikatsu_idle_sources_20261019/MANIFEST.json')
  self.assertEqual(len(data['clips']),3)
  self.assertEqual(digest(data['source']),data['source_sha256'])
  for row in data['clips']:
   self.assertEqual(digest(P/'koikatsu_idle_sources_20261019'/row['file']),row['sha256'])
 def test_all_unity_paths_resolved(self):
  doc=read('G09_Unity_Native_Transform_Hash_Map_20261019.json')
  self.assertEqual(doc['mapped_binding_paths'],doc['total_binding_paths'])
  self.assertEqual(doc['mapped_binding_paths'],234)
  self.assertTrue(all(len({r['path'] for r in rows})==1 for rows in doc['resolved'].values()))
 def test_sampled_quaternions(self):
  for p in P.glob('f_tachi_*_G09_Native_30FPS_Samples.json.gz'):
   with gzip.open(p,'rt',encoding='utf8') as f:d=json.load(f)
   self.assertTrue(140<=len(d['frames'])<500)
   self.assertAlmostEqual(d['minimum_sampled_quaternion_norm'],1.0,delta=0.0002)
   self.assertAlmostEqual(d['maximum_sampled_quaternion_norm'],1.0,delta=0.0002)
   self.assertFalse(d['native_rest_conflicts'])
 def test_actual_blender_source(self):
  p=P/'G09_AuthenticIdleAndBow_Source_Review_v4.blend'
  self.assertGreater(p.stat().st_size,1000000)
  self.assertEqual(digest(p),'d7bb6a98ef362410fbe23d649ab70569d2ee30716ecbd2df9ab3c404bf43f8b2')
 def test_animated_fbx_reimport(self):
  q=json.loads((P/'UnrealInterchange_20261019/BLENDER_FBX_REIMPORT_QA.json').read_text(encoding='utf8'))
  self.assertEqual(len(q['results']),4)
  for x in q['results']:
   self.assertEqual(x['bones'],975)
   self.assertGreater(x['maximum_three_sample_movement_m'],0.005)
   self.assertEqual(digest(P/'UnrealInterchange_20261019'/x['filename']),x['sha256'])
 def test_source_photos(self):
  q=read('G09_Koikatsu_IdlesAndBow_VisualSourceReview_v4.json')
  self.assertEqual(len(q['captures']),4)
  for x in q['captures']:self.assertEqual(digest(x['path']),x['sha256'])
 def test_faces_and_mod_rights_honest(self):
  face=read('G09_Facial_Source_Unreal_Contract_20261019.json')
  self.assertTrue(face['KK_SexFaces_plugin_installed'])
  self.assertEqual(face['unreal_morph_animation'],'NOT_TESTED')
  q=read('G09_Koikatsu_Adult_Face_Mod_Source_Inventory_20261019.json')
  self.assertGreater(q['total_installed_zipmod_packages'],100)
  self.assertEqual(q['local_source_transfer'],'SOURCE_INDEX_ONLY_PAYLOADS_LEFT_INSTALLED_IN_PLACE')
  self.assertEqual(q['license_review'],'REQUIRED_PER_3RD_PARTY_PACKAGE_BEFORE_EMBEDDING_OR_REDISTRIBUTION')
if __name__=='__main__':unittest.main(verbosity=2)
