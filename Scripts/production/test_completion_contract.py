#!/usr/bin/env python3
"""Regression checks proving false DONE claims fail closed."""
import copy
import hashlib
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

MODULE = Path(__file__).with_name("verify_completion_contract.py")
spec = importlib.util.spec_from_file_location("verify_completion_contract", MODULE)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
ROOT = MODULE.parents[2]


class CompletionContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ledger = json.loads(
            (ROOT / "docs/production/ACCEPTANCE_LEDGER.json").read_text(encoding="utf-8-sig")
        )

    def first(self):
        return copy.deepcopy(self.ledger)

    def test_repository_current_open_items_are_valid(self):
        self.assertEqual(mod.validate(ROOT, self.first()), [])

    def test_all_dones_fail_without_acceptance(self):
        doc = self.first()
        for row in doc["items"]:
            row["status"] = "DONE"
        errors = mod.validate(ROOT, doc, require_tracked=False)
        self.assertTrue(any("DONE missing frozen requirement" in error for error in errors))
        self.assertTrue(any("independent proofs" in error for error in errors))

    def test_claiming_frozen_opening_from_runtime_pass_is_rejected(self):
        doc = self.first()
        row = next(x for x in doc["items"] if x["id"] == "gate10.opening")
        row["delivered"] = list(row["requirements"])
        row["remaining"] = []
        row["status"] = "DONE"
        errors = mod.validate(ROOT, doc, require_tracked=False)
        self.assertTrue(any("user_approval" in msg and "visual" in msg for msg in errors))

    def test_source_implemented_is_not_automatically_done(self):
        doc = self.first()
        row = doc["items"][0]
        row["status"] = "SOURCE_IMPLEMENTED"
        self.assertEqual(mod.validate(ROOT, doc), [])

    def test_missing_gate_authority_is_rejected(self):
        doc = self.first()
        doc["items"][0].pop("frozen_source")
        self.assertTrue(any("upstream Gate" in e for e in mod.validate(ROOT, doc)))

    def test_cannot_escape_repo_with_fake_file(self):
        with tempfile.TemporaryDirectory() as path:
            root = Path(path)
            proof = root / "fixture.txt"
            proof.write_text("real", encoding="utf-8")
            doc = self.first()
            doc["items"][0]["evidence"] = [
                {"kind": "visual", "path": "../fixture.txt", "sha256": hashlib.sha256(b"real").hexdigest()}
            ]
            self.assertTrue(any("unsafe evidence path" in e for e in mod.validate(root, doc, False)))

    def test_mutated_proof_hash_is_rejected(self):
        with tempfile.TemporaryDirectory() as path:
            root = Path(path)
            proof = root / "evidence.txt"
            proof.write_text("original", encoding="utf-8")
            doc = self.first()
            doc["items"][0]["evidence"] = [
                {"kind": "native_test", "path": "evidence.txt", "sha256": hashlib.sha256(b"tampered").hexdigest()}
            ]
            self.assertTrue(any("HASH MISMATCH" in e for e in mod.validate(root, doc, False)))

    def test_json_evidence_hash_is_platform_newline_stable_but_fail_closed(self):
        with tempfile.TemporaryDirectory() as path:
            repo = Path(path)
            source = repo / "actual_runtime.json"
            source.write_bytes(b'{\r\n  "step": "passed"\r\n}\r\n')
            expected_lf = b'{\n  "step": "passed"\n}\n'
            row = {
                "id": "gate12.fixture_test", "type": "runtime",
                "status": "BLOCKED", "frozen_source": "QA regression",
                "requirements": ["real source result"],
                "delivered": [], "remaining": ["independent acceptance"],
                "evidence": [{"path": "actual_runtime.json",
                              "kind": "native_test",
                              "sha256": hashlib.sha256(expected_lf).hexdigest()}],
            }
            doc = {"schema": "NOWORLD_COMPLETION_V1",
                   "frozen_foundation": mod.FROZEN, "items": [row]}
            self.assertEqual(mod.validate(repo, doc, require_tracked=False), [])
            source.write_bytes(b'{\n  "step": "failed"\n}\n')
            self.assertTrue(any("HASH MISMATCH" in error
                                for error in mod.validate(repo, doc, require_tracked=False)))

    def test_production_report_cannot_claim_done_without_ledger(self):
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            reports = repo / "docs" / "production" / "gate10"
            reports.mkdir(parents=True)
            (reports / "REPORT.md").write_text(
                "# Opening" + chr(10) + "Status: DONE" + chr(10), encoding="utf-8")
            errors = mod.validate(repo, self.first(), require_tracked=False)
            self.assertTrue(any("unregistered formal DONE" in e for e in errors))

    def test_android_done_requires_real_device_and_user_evidence(self):
        doc = self.first()
        row = next(x for x in doc["items"] if x["id"] == "gate12.android_release")
        row["status"] = "DONE"
        row["delivered"] = list(row["requirements"])
        row["remaining"] = []
        row["accepted_against_frozen_source"] = True
        row["acceptance_commit"] = "deadbeef"
        row["tests_run_on_commit"] = True
        errors = mod.validate(ROOT, doc, require_tracked=False)
        self.assertTrue(any("device_test" in e for e in errors))


if __name__ == "__main__":
    unittest.main()
