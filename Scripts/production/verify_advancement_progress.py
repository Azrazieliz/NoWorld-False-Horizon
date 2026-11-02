#!/usr/bin/env python3
"""Fail closed if the GitHub progression ledger drifts from ADVANCEMENT_MASTER."""
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MASTER = ROOT / "docs/production/ADVANCEMENT_MASTER.md"
PROGRESS = ROOT / "docs/production/ADVANCEMENT_PROGRESS.json"
TASK = re.compile(r"^- \[([x ])\] \*\*(G12-\d{4})(?: / ([^*]+))?\*\* — (.+)$", re.MULTILINE)


def main() -> None:
    text = MASTER.read_text(encoding="utf-8")
    rows = TASK.findall(text)
    assert rows, "ADVANCEMENT_MASTER has no recognized G12 tasks"
    expected = {
        ident: (checked == "x", descriptor.strip(), requirement)
        for checked, ident, descriptor, requirement in rows
    }
    assert len(expected) == len(rows), "Duplicate master task ID"

    data = json.loads(PROGRESS.read_text(encoding="utf-8"))
    assert data["schema"] == "noworld.advancement_progress.v1"
    assert data["master"] == "docs/production/ADVANCEMENT_MASTER.md"
    records = data["tasks"]
    actual = {row["id"]: row for row in records}
    assert len(actual) == len(records), "Duplicate progress task ID"
    assert set(actual) == set(expected), (
        "Master/progress mismatch: missing "
        + str(sorted(set(expected) - set(actual))) + " extra "
        + str(sorted(set(actual) - set(expected)))
    )
    for ident, (checked, descriptor, requirement) in expected.items():
        row = actual[ident]
        assert row["master_checked"] is checked, f"{ident}: stale checkbox"
        assert row["master_descriptor"] == descriptor, f"{ident}: stale descriptor"
        assert row["master_requirement"] == requirement, f"{ident}: stale requirement"
        state = row["state"]
        assert state in ("MASTER_CHECKED_SCOPED", "OPEN", "SOURCE_PARTIAL"), (
            f"{ident}: unknown progress status {state}"
        )
        if checked:
            assert state == "MASTER_CHECKED_SCOPED", f"{ident}: altered verified scope"
        elif state == "SOURCE_PARTIAL":
            assert row.get("source_paths") and row.get("remaining"), (
                f"{ident}: partial work needs paths and remaining steps"
            )
            assert all((ROOT / path).exists() for path in row["source_paths"]), (
                f"{ident}: nonexistent source path"
            )
        else:
            assert state == "OPEN", f"{ident}: open task misrepresented"
    summary = data["summary"]
    assert summary["master_task_count"] == len(rows)
    assert summary["master_checked"] == sum(row[0] == "x" for row in rows)
    assert summary["master_open"] == sum(row[0] == " " for row in rows)
    assert summary["source_partial"] == sum(
        row["state"] == "SOURCE_PARTIAL" for row in records
    )
    # A future verified build/device run is allowed to advance the ledger,
    # but only when its original reproducible evidence has been committed.
    for flag in ("build_verified", "device_verified", "user_accepted"):
        assert isinstance(summary[flag], bool), f"{flag} must be boolean"
        if summary[flag]:
            proof = data.get("verification_evidence", {}).get(flag)
            assert isinstance(proof, dict) and proof.get("reference"), (
                f"{flag}: missing actual verification reference"
            )
    print(f"ADVANCEMENT_PROGRESS matches {len(rows)} ADVANCEMENT_MASTER tasks")


if __name__ == "__main__":
    main()
