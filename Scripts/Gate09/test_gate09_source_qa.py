"""Gate09 regression: check only read-only real-source receipts, never ARTIFACT_READY."""
import pathlib,sys,unittest,json,hashlib
ROOT=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
import audit_luminous_rig as rig
def read(n):return json.loads((ROOT/n).read_text(encoding='utf-8'))
def sha(p):return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
class Gate09SourceEvidence(unittest.TestCase):
 def test_core_classifier_not_inherited_from_parent(self):
  self.assertEqual(rig.kind('Hips/Head/_yure_hair_h_F01a'),'possible_secondary')
  self.assertNotEqual(rig.kind('Hips/Head/N_move 15620'),'possible_core')
 def test_source_glb_joints_and_weights(self):
  d=read('G09_Luminous_Rig_Influences_Bind_v2.json')
  self.assertEqual([d['states'][k]['joints'] for k in ['clothed','underwear','nude']],[975]*3)
  self.assertEqual([d['states'][k]['weighted_joints'] for k in ['clothed','underwear','nude']],[324,297,295])
 def test_joint_paths_and_rest_are_not_interoperable(self):
  d=read('G09_Luminous_Rig_Influences_Bind_v2.json')
  self.assertEqual([p['shared'] for p in d['pairs'].values()],[782]*3)
  self.assertTrue(all(not p['same_skeleton_accepted'] and p['only_a']==193 and p['only_b']==193 for p in d['pairs'].values()))
  self.assertTrue(all(p['mismatch_affects_weighted_joint_count']>0 for p in d['pairs'].values()))
 def test_source_glb_hashes(self):
  d=read('G09_Luminous_Rig_Influences_Bind_v2.json')
  base=pathlib.Path(r'D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011\BLENDER_FRESH')
  for k in ['clothed','underwear','nude']:
   self.assertEqual(sha(base/k.upper()/('LUMINOUS_V3_'+k.upper()+'_skinned.glb')),d['states'][k]['sha256'])
 def test_proof_original_source_never_mutated(self):
  d=read('G09_Luminous_CMU144_20_Retarget_PROOF.json')
  for path,expected in d['source_hashes'].items():self.assertEqual(sha(path),expected)
 def test_real_bone_motion_and_skin(self):
  d=read('G09_Luminous_CMU144_20_Retarget_PROOF.json')
  self.assertEqual(d['mapped_count'],22)
  self.assertEqual(d['target_bones'],975)
  self.assertGreater(d['vertex_deformation']['start_to_middle_m'],0.01)
  self.assertFalse(any('unreal' in d['status'].lower() for _ in [0]))
 def test_proof_files_hashes(self):
  d=read('G09_Luminous_CMU144_20_Retarget_PROOF.json')
  for name,rec in d['files'].items():self.assertEqual(sha(ROOT/name),rec['sha256'])
 def test_roundtrip_has_real_motion(self):
  d=read('G09_Luminous_CMU144_20_FBX_Reload.json')
  self.assertEqual(d['bone_count'],975)
  self.assertIn('TIME_VARYING',d['status'])
  self.assertEqual(sha(ROOT/'G09_Luminous_CMU144_20_AnimOnly_PROOF.fbx'),d['input_sha256'])
 def test_views_belong_to_exact_blend(self):
  d=read('G09_CMU144_20_Captures.json')
  self.assertEqual(d['source_blend_sha256'],sha(ROOT/'G09_Luminous_CMU144_20_Retarget_PROOF.blend'))
  self.assertEqual(len(d['shots']),3)
  for rec in d['shots']:self.assertEqual(sha(ROOT/rec['name']),rec['sha256'])
 def test_dance_and_android_cannot_be_claimed(self):
  d=read('G09_Luminous_CMU144_20_Retarget_PROOF.json')
  self.assertIn('VISUAL_QA_PENDING',d['status'])
  self.assertIn('No Unreal import or Android evidence',d['limitations'])
if __name__=='__main__':
 suite=unittest.defaultTestLoader.loadTestsFromTestCase(Gate09SourceEvidence)
 r=unittest.TextTestRunner(verbosity=2).run(suite)
 sys.exit(0 if r.wasSuccessful() else 1)
