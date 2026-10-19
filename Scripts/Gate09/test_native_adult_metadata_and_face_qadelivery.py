"""Immutable-source checks for sampled Koikatsu adult-mode metadata and genuine UE facial curves."""
import pathlib,unittest,json,hashlib,zipfile
P=pathlib.Path(r'D:\UnrealProjects\Gate09_Protected_Production_20261016')
Q=pathlib.Path(r'D:\UnrealProjects\Gate09_Unreal583_Animation_QA_20261019')
G=pathlib.Path(r'D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010')
def read(p):return json.loads(p.read_text(encoding='utf8'))
def hash(p):return hashlib.sha256(p.read_bytes()).hexdigest()
class FaceNativeProof(unittest.TestCase):
 def test_real_native_animation_headers(self):
  a=read(P/'G09_InstalledAdultNativeAnimation_MetadataOnly_20261019.json')
  self.assertEqual(a['sampled_bundle_count'],25)
  self.assertEqual(a['sampled_native_clip_count'],750)
  self.assertEqual(a['available_bundle_count'],288)
  self.assertFalse(a['animation_curve_values_exported'])
 def test_native_hashes_against_original_prefab(self):
  a=read(P/'G09_AdultNativeRigBindingCompatibility_20261019.json')
  self.assertEqual(a['bundles_audited'],25)
  self.assertTrue(all(x['resolved_native_paths']==343 and x['not_resolved']==0 and not x['ambiguous_crc_paths'] for x in a['sampled']))
  self.assertFalse(a['motion_keys_exported'])
 def test_ue_actual_imported_morphs_and_cold_reopen(self):
  base=Q/'Saved/Gate09QA'
  a=read(base/'unreal_04_authentic_source_face_morphs.json')
  b=read(base/'unreal_05_native_facial_cold.json')
  self.assertEqual(a['status'],'PASS_NATIVE_UNREAL_ORIGINAL_LUMINOUS_FACE_CURVES_SOURCE_PROOF')
  self.assertEqual(b['status'],'PASS_FACIAL_SOURCE_COLD_RELOAD')
  self.assertEqual(b['mesh_native_morphs'],96)
  self.assertEqual(b['num_float_curves'],2)
  self.assertEqual(set(b['native_identified_curve_names']),set(a['curves']))
  self.assertEqual(b['model_frames'],60)
  self.assertGreater(a['curves']['Eye_Closed']['maximum'],0.95)
 def test_unmodified_approved_meshes(self):
  path=Q/'Content/Experimental/LuminousValentineV3/CLOTHED/Parts/Mesh003Prim000'
  copies=list(path.rglob('*.uasset'))
  self.assertEqual(len(copies),3)
  for cp in copies:self.assertEqual(hash(cp),hash(G/'Content'/cp.relative_to(Q/'Content')))
 def test_package_is_reproducible_and_non_explicit(self):
  p=P/'GATE09_NATIVE_FACE_AND_ADULT_SOURCE_METADATA_QA_20261019.zip'
  self.assertEqual(hash(p),'a46933e7fdf170179985630399ffcbfa4d5c13bd7ea88d95961741f5c96bd8f8')
  with zipfile.ZipFile(p) as z:
   self.assertIsNone(z.testzip())
   self.assertEqual(sum(x.endswith('.uasset') for x in z.namelist()),4)
   meta=json.loads(z.read('Gate09_Proof_Manifest.json'))
   self.assertFalse(meta['source_payloads_included'])
   self.assertFalse(meta['explicit_scene_imported'])
if __name__=='__main__':unittest.main(verbosity=2)
