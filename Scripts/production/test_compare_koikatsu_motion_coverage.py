"""Standard-library tests using synthetic metadata; no proprietary source data."""
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch


SCRIPT = Path(__file__).with_name("compare_koikatsu_motion_coverage.py")


def load_tool():
    spec = importlib.util.spec_from_file_location("motion_coverage", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class CoverageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.tool = load_tool()
        self.h = self.root / self.tool.H_MANIFEST
        self.g = self.root / self.tool.GATE09_MANIFEST
        self.a = self.root / self.tool.ADDITIONAL_DIRECTORY / (
            "GATE12_ALL_INSTALLED_KK_ADDITIONAL_ANIMATIONS_PART_0000_0002.json")
        self.catalog = self.root / "Content/NoWorld/Animation/SharedMotionCatalog.json"
        self.curve = self.root / "SourceAssets/private-original.gz"
        self.curve.parent.mkdir(parents=True)
        self.curve.write_bytes(b"UNREADABLE ORIGINAL FIXTURE: NEVER COPY")
        self.native = self.root / "Content/Fixture/Native.uasset"
        self.native.parent.mkdir(parents=True)
        self.native.write_bytes(b"UNREADABLE NATIVE FIXTURE: NEVER HASH")
        self.write(self.h, {"schema": self.tool.H_SCHEMA, "actual_native_curve_clip_count": 2,
            "status": "ALL_INSTALLED_H_SOURCE_CURVES_CAPTURED_NOT_YET_RETARGETED", "clip_rows": [
                self.hrow("a", 1), self.hrow("b", 1)]})
        self.write(self.g, {"schema": self.tool.GATE09_SCHEMA, "actual_curve_objects_extracted": 1,
            "status": "ALL_SOURCE_BUNDLES_PROCESSED", "bundles_expected": 1, "bundles_attempted": 1,
            "source_files": [{"source_hash": "a" * 64, "original_clip_path_id": 1,
                              "raw_curve_path": str(self.curve), "source_clip_name": "SECRET_SOURCE_NAME"}]})
        self.write(self.a, {"schema": self.tool.ADDITIONAL_SCHEMA, "clip_count": 1,
            "batch_start": 0, "batch_end": 2, "progress_next_index": 2, "additional_bundles": 2,
            "status": "SOURCE_ARCHIVE_COMPLETE_NOT_NATIVE_UNREAL", "clips": [
                {"source_bundle_sha256": "c" * 64, "path_id": -2,
                 "native_curve_path": str(self.curve), "name": "SECRET_SOURCE_NAME"}]})
        self.write(self.catalog, {"schema": self.tool.CATALOG_SCHEMA, "motions": [self.motion("a", 1)]})

    @staticmethod
    def write(path, value):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(value), encoding="utf-8")

    def hrow(self, sha, path_id):
        return {"source_digest_sha256": sha * 64, "native_path_id": path_id,
                "curve_file": str(self.curve), "clip_name": "SECRET_SOURCE_NAME"}

    @staticmethod
    def motion(sha, path_id):
        return {"motion_id": "SECRET_MOTION_NAME", "source_clip_id": "same-display-id",
                "source_bundle_sha256": sha * 64, "source_hash": "f" * 64,
                "source_unity_path_id": path_id, "native_sequence": "/Game/Fixture/Native.Native",
                "participants": 1, "approved_for_gameplay": True}

    def compare(self):
        return self.tool.compare(self.root, self.catalog)

    def test_exact_union_uses_bundle_and_path_not_names_or_catalog_clip_hash(self):
        self.write(self.catalog, {"schema": self.tool.CATALOG_SCHEMA, "motions": [
            self.motion("a", 1), self.motion("a", "1"), self.motion("b", 1), self.motion("d", 1),
            {"source_clip_id": "without-provenance", "source_hash": "a" * 64, "participants": 1}]})
        receipt = self.compare()
        self.assertEqual(receipt["inventory"]["rows"], 4)
        self.assertEqual(receipt["inventory"]["unique_actual_ids"], 3)
        self.assertEqual(receipt["inventory"]["duplicate_identity_rows"], 1)
        self.assertEqual(receipt["catalog"]["unique_actual_ids"], 3)
        self.assertEqual(receipt["catalog"]["rows_without_valid_actual_id"], 1)
        self.assertEqual(receipt["coverage"]["matched_unique_actual_ids"], 2)
        self.assertEqual(receipt["coverage"]["inventory_ids_without_catalog_entry"], 1)
        self.assertEqual(receipt["coverage"]["catalog_ids_outside_inventory"], 1)
        self.assertEqual(receipt["catalog"]["unique_source_clip_labels"], 2)

    def test_scene_field_presence_is_distinct_from_valid_metadata(self):
        rows = [self.motion("a", 1), self.motion("b", 1), self.motion("c", -2)]
        rows[1].update(participants=2, scene_id="", scene_role=None)
        rows[2].update(participants=2, scene_id="fixture-scene", scene_role="fixture-role")
        self.write(self.catalog, {"schema": self.tool.CATALOG_SCHEMA, "motions": rows})
        catalog = self.compare()["catalog"]
        self.assertEqual(catalog["metadata"]["scene_id"], {"present": 2, "populated": 1, "valid": 1})
        self.assertEqual(catalog["rows_with_complete_scene_metadata"], 1)
        self.assertEqual(catalog["multi_participant_rows_missing_scene_metadata"], 1)

    def test_missing_and_outside_paths_are_aggregate_only(self):
        data = json.loads(self.h.read_text())
        data["clip_rows"][0]["curve_file"] = str(self.root / "missing-private-name.gz")
        data["clip_rows"][1]["curve_file"] = str(self.root.parent / "outside-private-name.gz")
        self.write(self.h, data)
        self.native.unlink()
        receipt = self.compare()
        self.assertEqual(receipt["inventory"]["curve_paths"]["rows"]["missing"], 1)
        self.assertEqual(receipt["inventory"]["curve_paths"]["rows"]["outside_root"], 1)
        self.assertEqual(receipt["catalog"]["native_paths"]["rows"]["missing"], 1)
        self.assertFalse(receipt["inventory"]["intake_complete"])
        self.assertTrue(receipt["inventory"]["manifest_intake_complete"])
        encoded = json.dumps(receipt)
        for secret in ["SECRET_SOURCE_NAME", "SECRET_MOTION_NAME", "private-original", "missing-private-name", "outside-private-name", "same-display-id"]:
            self.assertNotIn(secret, encoded)

    def test_only_manifest_documents_are_opened(self):
        allowed = {p.resolve() for p in (self.h, self.g, self.a, self.catalog)}
        original = Path.open
        def guarded(path, *args, **kwargs):
            self.assertIn(path.resolve(), allowed, "A curve, bundle or native asset was opened")
            return original(path, *args, **kwargs)
        with patch.object(Path, "open", guarded):
            receipt = self.compare()
        self.assertEqual(receipt["native_files_hashed"], 0)

    def test_invalid_ids_are_not_fabricated_or_coerced(self):
        rows = [self.motion("a", True), self.motion("a", 1.0), self.motion("a", "01"),
                self.motion("a", 2**63), self.motion("a", "-2")]
        rows[4]["source_bundle_sha256"] = "not-a-sha"
        self.write(self.catalog, {"schema": self.tool.CATALOG_SCHEMA, "motions": rows})
        receipt = self.compare()
        self.assertEqual(receipt["catalog"]["unique_actual_ids"], 0)
        self.assertEqual(receipt["catalog"]["rows_without_valid_actual_id"], 5)

    def test_declared_count_and_batch_gap_cannot_claim_complete_intake(self):
        data = json.loads(self.h.read_text()); data["actual_native_curve_clip_count"] = 99
        self.write(self.h, data)
        data = json.loads(self.a.read_text()); data["batch_start"] = 1
        self.a.unlink()
        self.a = self.a.with_name("GATE12_ALL_INSTALLED_KK_ADDITIONAL_ANIMATIONS_PART_0001_0002.json")
        self.write(self.a, data)
        receipt = self.compare()
        self.assertFalse(receipt["inventory"]["intake_complete"])
        self.assertIn("declared_count_mismatch", receipt["inventory"]["groups"]["h"]["issues"])
        self.assertIn("batch_gap_or_overlap", receipt["inventory"]["groups"]["additional"]["issues"])

    def test_missing_required_manifest_fails_closed(self):
        self.g.unlink()
        with self.assertRaises(self.tool.CoverageError):
            self.compare()

    def test_strict_json_and_wrong_schema_fail_without_echoing_source(self):
        for text in ['{"schema":"SECRET_SOURCE_NAME","schema":"duplicate"}',
                     '{"schema":"SECRET_SOURCE_NAME","motions":[NaN]}',
                     '{"schema":"SECRET_SOURCE_NAME","motions":[]}']:
            self.catalog.write_text(text)
            with self.assertRaises(self.tool.CoverageError) as caught:
                self.compare()
            self.assertNotIn("SECRET_SOURCE_NAME", str(caught.exception))

    def test_unreal_path_cannot_escape_content_or_access_network(self):
        rows = [self.motion("a", 1), self.motion("b", 1)]
        rows[0]["native_sequence"] = "/Game/../private/Asset.Asset"
        rows[1]["native_sequence"] = "https://invalid.example/Asset.Asset"
        self.write(self.catalog, {"schema": self.tool.CATALOG_SCHEMA, "motions": rows})
        self.assertEqual(self.compare()["catalog"]["native_paths"]["rows"]["invalid"], 2)

    def test_cli_writes_only_receipt_and_preserves_inputs(self):
        before = {p: p.read_bytes() for p in (self.h, self.g, self.a, self.catalog, self.curve, self.native)}
        output = self.root / "evidence/receipt.json"
        result = subprocess.run([sys.executable, "-B", str(SCRIPT), "--source-root", str(self.root),
            "--catalog", str(self.catalog), "--output", str(output)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        receipt = json.loads(output.read_text())
        self.assertEqual(receipt["inventory"]["unique_actual_ids"], 3)
        self.assertEqual(receipt["acceptance"], "metadata_coverage_only_not_native_or_gameplay_acceptance")
        self.assertNotIn("SECRET", result.stdout + result.stderr)
        for path, content in before.items():
            self.assertEqual(path.read_bytes(), content)
        self.assertFalse(list(output.parent.glob("*.tmp")))

    def test_output_refuses_source_or_catalog_overwrite(self):
        for output in (self.h, self.catalog, self.curve):
            result = subprocess.run([sys.executable, "-B", str(SCRIPT), "--source-root", str(self.root),
                "--catalog", str(self.catalog), "--output", str(output)], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
