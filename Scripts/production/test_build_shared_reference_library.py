"""Pure producer integration tests. All source data below are synthetic fixtures."""
import copy
import gzip
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

SCRIPT = Path(__file__).with_name("build_shared_reference_library.py")
SPEC = importlib.util.spec_from_file_location("shared_reference_builder", SCRIPT)
builder = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(builder)


def fixture_document(path_id=-9007199254740993, bundle="a" * 64):
    return {"schema": builder.SOURCE_SCHEMA, "source_sha256": bundle,
            "source_path_id": path_id, "source_clip_sha256": "b" * 64,
            "frame_rate": 2, "frame_count": 3, "original_duration_s": 1.0,
            "rig_family": "fixture_original_rig", "relative_source_pose_only": True,
            "bones": [{"source_bone": "source_root", "target_bone": "adapter_root",
                       "deltas": [[0, 0, 0, 1], [0, 0, 0, 1], [0, 0, 0, 1]]}]}


def fixture_row(document, outfit="A", approved=False):
    return {"source_bundle_sha256": document["source_sha256"],
            "source_unity_path_id": document["source_path_id"],
            "source_hash": "c" * 64, "source_clip_id": "legacy.actual-source",
            "action": "Fixture.DeclaredAction", "context": "Fixture",
            "rig": "fixture_catalog_rig", "anatomy": "fixture_anatomy",
            "capabilities": ["fixture_capability"], "participants": 1,
            "priority": 3, "loop": True, "play_rate": 1.25,
            "approved_for_gameplay": approved, "outfit": outfit}


class SharedReferenceLibraryTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.sources = self.root / "SourceAssets"
        self.sources.mkdir()
        self.catalog = self.root / "catalog.json"
        self.output = self.root / "Content/SharedSource"
        self.document = fixture_document()
        self.put_source("source", self.document)
        self.put_catalog([fixture_row(self.document, "A"), fixture_row(self.document, "B")])

    def put_source(self, name, document):
        path = self.sources / (name + builder.SOURCE_SUFFIX)
        path.write_bytes(gzip.compress(json.dumps(document, allow_nan=False).encode(), mtime=0))
        return path

    def put_catalog(self, rows):
        self.catalog.write_text(json.dumps({"schema": builder.CATALOG_SCHEMA, "motions": rows}),
                                encoding="utf-8")

    def build(self):
        return builder.build_library(self.sources, self.catalog, self.output)

    def packet(self, manifest=None):
        manifest = manifest or json.loads((self.output / builder.MANIFEST_NAME).read_text())
        return (self.output / manifest["clips"][0]["path"]).read_bytes()

    def test_full_build_one_packet_no_wardrobe_route_copies_and_exact_int64(self):
        before = {p.name: p.read_bytes() for p in self.sources.iterdir()}
        manifest = self.build()
        self.assertEqual(len(manifest["clips"]), 1)
        self.assertEqual(len(manifest["routes"]), 1)
        clip = manifest["clips"][0]
        expected = "kk." + "a" * 64 + "." + format((-9007199254740993) & ((1 << 64) - 1), "016x")
        self.assertEqual(clip["id"], expected)
        self.assertEqual(clip["source_path_id"], "-9007199254740993")
        self.assertEqual(clip["path"], "Clips/" + expected + ".nwmotion")
        self.assertEqual(clip["rig_profiles"], ["fixture_catalog_rig", "fixture_original_rig"])
        self.assertNotIn("outfit", manifest["routes"][0])
        self.assertFalse(manifest["routes"][0]["approved_legacy"])
        self.assertEqual({p.name: p.read_bytes() for p in self.sources.iterdir()}, before)
        self.assertEqual(builder.validate_library(self.output), manifest)

    def test_packet_byte_contract_and_crc(self):
        self.build()
        packet = self.packet()
        self.assertEqual(packet[:8], b"NWMRJ001")
        length, crc = struct.unpack("<II", packet[8:16])
        plain = zlib.decompress(packet[16:])
        self.assertEqual(length, len(plain))
        self.assertEqual(crc, zlib.crc32(plain) & 0xffffffff)
        document = json.loads(plain)
        self.assertEqual(document["schema"], builder.PACKET_SCHEMA)
        self.assertEqual(document["tracks"][0]["bone"], "source_root")
        self.assertEqual(document["tracks"][0]["aliases"], ["adapter_root"])
        self.assertEqual(builder.decode_packet(packet), document)

    def test_old_rotation_only_does_not_invent_translation_scale_or_complete_source(self):
        self.build()
        motion = builder.decode_packet(self.packet())
        self.assertEqual(motion["source_fields_originally_absent"], ["translation", "scale"])
        self.assertNotIn("translations", motion["tracks"][0])
        self.assertNotIn("scales", motion["tracks"][0])
        self.assertFalse(motion["source_data_complete"])
        self.assertFalse(motion["provenance"]["source_events_available"])

    def test_actual_translation_scale_samples_are_preserved(self):
        document = copy.deepcopy(self.document)
        document["bones"][0]["translations"] = [[0, 0, 0], [1, 2, 3], [4, 5, 6]]
        document["bones"][0]["scales"] = [[1, 1, 1], [2, 2, 2], [3, 3, 3]]
        self.put_source("source", document)
        self.build()
        motion = builder.decode_packet(self.packet())
        self.assertEqual(motion["source_fields_originally_absent"], [])
        self.assertEqual(motion["tracks"][0]["translations"], document["bones"][0]["translations"])
        self.assertEqual(motion["tracks"][0]["scales"], document["bones"][0]["scales"])

    def test_duplicate_source_identity_merges_declared_aliases_and_provenance_only(self):
        second = copy.deepcopy(self.document)
        second["bones"][0]["target_bone"] = "second_declared_adapter_alias"
        second["source_clip_sha256"] = "d" * 64
        self.put_source("second", second)
        manifest = self.build()
        self.assertEqual(len(manifest["clips"]), 1)
        motion = builder.decode_packet(self.packet())
        self.assertEqual(motion["tracks"][0]["aliases"], ["adapter_root", "second_declared_adapter_alias"])
        self.assertEqual(motion["provenance"]["source_clip_sha256"], ["b" * 64, "d" * 64])

    def test_conflicting_same_identity_is_rejected_before_output_changes(self):
        manifest = self.build()
        old = self.packet(manifest)
        changed = copy.deepcopy(self.document)
        changed["bones"][0]["deltas"][1] = [0, 0, 1, 0]
        self.put_source("conflict", changed)
        with self.assertRaises(builder.LibraryError):
            self.build()
        self.assertEqual(self.packet(manifest), old)

    def test_direct_library_includes_available_sources_without_catalog_semantics(self):
        other = fixture_document(3, "d" * 64)
        self.put_source("direct_only", other)
        manifest = self.build()
        self.assertEqual(len(manifest["clips"]), 2)
        self.assertEqual(len(manifest["routes"]), 1)
        self.assertFalse(manifest["coverage"]["source_inventory_complete"])

    def test_no_provenance_or_unavailable_catalog_rows_are_not_guessed(self):
        missing = fixture_row(fixture_document(4, "e" * 64))
        no_provenance = fixture_row(self.document)
        del no_provenance["source_bundle_sha256"]
        self.put_catalog([fixture_row(self.document), missing, no_provenance])
        manifest = self.build()
        self.assertEqual(len(manifest["routes"]), 1)
        self.assertEqual(manifest["coverage"]["catalog_rows_without_actual_id"], 1)
        self.assertEqual(manifest["coverage"]["catalog_rows_without_available_source"], 1)

    def test_source_hash_is_not_substituted_for_catalog_bundle_sha(self):
        row = fixture_row(self.document)
        row["source_bundle_sha256"] = "d" * 64
        row["source_hash"] = self.document["source_sha256"]
        self.put_catalog([row])
        self.assertEqual(self.build()["routes"], [])

    def test_declared_legacy_id_root_contact_and_morph_metadata_survive(self):
        row = fixture_row(self.document, approved=True)
        row.update(root_motion=True, contact_seconds=0.25,
                   morph_tracks=[{"morph": "fixture_morph", "keys": [{"time": 0.25, "weight": 0.5}]}])
        self.put_catalog([row])
        route = self.build()["routes"][0]
        self.assertEqual(route["legacy_source_id"], "legacy.actual-source")
        self.assertTrue(route["root_motion"])
        self.assertEqual(route["contact_seconds"], 0.25)
        self.assertEqual(route["morph_tracks"], row["morph_tracks"])
        self.assertTrue(route["approved_legacy"])

    def test_scene_ids_and_roles_are_only_copied_from_actual_source_metadata(self):
        row = fixture_row(self.document)
        row.update(participants=2, scene_id="declared-fixture-scene", scene_role="declared-role")
        self.put_catalog([row])
        route = self.build()["routes"][0]
        self.assertEqual(route["scene_id"], row["scene_id"])
        self.assertEqual(route["scene_role"], row["scene_role"])
        row["scene_role"] = ""
        self.put_catalog([row])
        with self.assertRaises(builder.LibraryError):
            self.build()

    def test_invalid_sources_fail_before_any_packet_is_written(self):
        for edit in (lambda d: d.update(source_path_id=1.0),
                     lambda d: d.update(source_path_id=True),
                     lambda d: d.update(frame_count=5),
                     lambda d: d["bones"][0]["deltas"].__setitem__(0, [0, 0, 0, 0]),
                     lambda d: d["bones"][0].update(positions=[[1, 2, 3]] * 3)):
            document = copy.deepcopy(self.document)
            edit(document)
            self.put_source("source", document)
            with self.assertRaises(builder.LibraryError):
                self.build()
            self.assertFalse((self.output / builder.MANIFEST_NAME).exists())

    def test_duplicate_fname_bones_and_nonfinite_numbers_rejected(self):
        document = copy.deepcopy(self.document)
        second = copy.deepcopy(document["bones"][0])
        second["source_bone"] = "SOURCE_ROOT"
        document["bones"].append(second)
        self.put_source("source", document)
        with self.assertRaises(builder.LibraryError):
            self.build()
        with self.assertRaises(builder.LibraryError):
            builder.parse_json(b'{"duplicate":1,"duplicate":2}')
        with self.assertRaises(builder.LibraryError):
            builder.parse_json(b'{"number":NaN}')

    def test_bad_magic_crc_length_zlib_and_extra_stream_data_rejected(self):
        self.build()
        good = self.packet()
        bad_crc = bytearray(good); bad_crc[12] ^= 1
        bad_length = bytearray(good); bad_length[8:12] = struct.pack("<I", 1)
        for data in (b"wrong" + good[5:], bytes(bad_crc), bytes(bad_length),
                     good[:-2], good + b"trailing", good + zlib.compress(b"second")):
            with self.assertRaises(builder.LibraryError):
                builder.decode_packet(data)

    def test_rebuild_is_deterministic_and_stale_owned_packets_are_removed(self):
        other = fixture_document(3, "d" * 64)
        extra = self.put_source("other", other)
        first = self.build()
        snapshots = {p.relative_to(self.output): p.read_bytes() for p in self.output.rglob("*") if p.is_file()}
        self.assertEqual(self.build(), first)
        self.assertEqual({p.relative_to(self.output): p.read_bytes() for p in self.output.rglob("*") if p.is_file()}, snapshots)
        extra.unlink()
        manifest = self.build()
        self.assertEqual(len(manifest["clips"]), 1)
        self.assertEqual(len(list((self.output / "Clips").glob("*.nwmotion"))), 1)

    def test_manifest_path_escape_and_corrupted_packet_are_rejected(self):
        manifest = self.build()
        entry = manifest["clips"][0]
        manifest["clips"][0]["path"] = "../source.gz"
        (self.output / builder.MANIFEST_NAME).write_text(json.dumps(manifest))
        with self.assertRaises(builder.LibraryError):
            builder.validate_library(self.output)
        manifest["clips"][0]["path"] = "Clips/" + entry["id"] + ".nwmotion"
        (self.output / builder.MANIFEST_NAME).write_text(json.dumps(manifest))
        path = self.output / manifest["clips"][0]["path"]
        packet = bytearray(path.read_bytes()); packet[-1] ^= 1; path.write_bytes(packet)
        with self.assertRaises(builder.LibraryError):
            builder.validate_library(self.output)

    def test_cli_build_and_validate_all_packets(self):
        args = [sys.executable, "-B", str(SCRIPT), "--source-root", str(self.sources),
                "--catalog", str(self.catalog), "--output-root", str(self.output)]
        built = subprocess.run(args, text=True, capture_output=True)
        self.assertEqual(built.returncode, 0, built.stderr)
        checked = subprocess.run(args + ["--validate-only"], text=True, capture_output=True)
        self.assertEqual(checked.returncode, 0, checked.stderr)
        self.assertTrue(json.loads(checked.stdout)["all_packet_crc_checks_passed"])


if __name__ == "__main__":
    unittest.main()
