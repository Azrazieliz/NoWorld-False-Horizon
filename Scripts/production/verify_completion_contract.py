#!/usr/bin/env python3
"""Fail-closed proof requirement for every formal production DONE assertion."""
import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ALLOWED = {"NOT_STARTED", "IN_PROGRESS", "BLOCKED", "SOURCE_IMPLEMENTED",
           "RUNTIME_VERIFIED", "VISUAL_VERIFIED", "ACCEPTED", "DONE"}
KINDS = {"source", "native_test", "device_test", "visual", "source_asset",
         "gate_approval", "user_approval"}
FROZEN = "3f54f07cacbf193e160d0be15a7be5a7bc730c3b"


def tracked(repo, name):
    return subprocess.run(
        ["git", "-C", str(repo), "ls-files", "--error-unmatch", "--", name],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        check=False).returncode == 0


def validate(repo, ledger, require_tracked=True):
    problems = []
    if ledger.get("schema") != "NOWORLD_COMPLETION_V1":
        problems.append("invalid ledger schema")
    if ledger.get("frozen_foundation") != FROZEN:
        problems.append("immutable Foundation commit mismatch")
    items = ledger.get("items")
    if not isinstance(items, list) or not items:
        return problems + ["nonempty completion items list required"]
    seen = set()
    for item in items:
        if not isinstance(item, dict):
            problems.append("invalid item")
            continue
        key, status = item.get("id"), item.get("status")
        if not isinstance(key, str) or not re.fullmatch(r"gate\d{2}\.[a-z0-9_.-]+", key):
            problems.append("invalid gate-qualified item id")
            continue
        if key in seen:
            problems.append(f"{key}: duplicate id")
        seen.add(key)
        if status not in ALLOWED:
            problems.append(f"{key}: invalid status {status!r}")
            continue
        reqs = item.get("requirements")
        if not isinstance(reqs, list) or not reqs or any(
            not isinstance(r, str) or not r for r in reqs
        ):
            problems.append(f"{key}: missing frozen requirements")
            continue
        if not item.get("frozen_source"):
            problems.append(f"{key}: no named upstream Gate authority")
        delivered = item.get("delivered", [])
        remaining = item.get("remaining", [])
        if not isinstance(delivered, list) or set(delivered) - set(reqs):
            problems.append(f"{key}: invalid delivered requirements")
            continue
        if not isinstance(remaining, list):
            problems.append(f"{key}: remaining must be list")
            continue
        if status == "DONE":
            if set(delivered) != set(reqs):
                problems.append(f"{key}: DONE missing frozen requirement(s)")
            if remaining:
                problems.append(f"{key}: DONE with outstanding blockers")
        proofs = item.get("evidence", [])
        if not isinstance(proofs, list):
            problems.append(f"{key}: evidence must be list")
            continue
        types = set()
        for proof in proofs:
            if not isinstance(proof, dict):
                problems.append(f"{key}: malformed evidence entry")
                continue
            name, expected, kind = proof.get("path"), proof.get("sha256"), proof.get("kind")
            if kind not in KINDS:
                problems.append(f"{key}: invalid evidence kind {kind!r}")
                continue
            types.add(kind)
            if (not isinstance(name, str) or not name or
                Path(name).is_absolute() or ".." in Path(name).parts or "\\" in name):
                problems.append(f"{key}: unsafe evidence path")
                continue
            source = (repo / name).resolve()
            try:
                source.relative_to(repo.resolve())
            except ValueError:
                problems.append(f"{key}: evidence escaped repository")
                continue
            if not source.is_file():
                problems.append(f"{key}: evidence missing: {name}")
            elif not isinstance(expected, str) or not re.fullmatch("[0-9a-f]{64}", expected):
                problems.append(f"{key}: missing exact SHA256")
            elif hashlib.sha256(source.read_bytes()).hexdigest() != expected:
                problems.append(f"{key}: evidence HASH MISMATCH: {name}")
            if require_tracked and not tracked(repo, name):
                problems.append(f"{key}: evidence not committed to Git: {name}")
        if status in {"RUNTIME_VERIFIED", "VISUAL_VERIFIED", "ACCEPTED", "DONE"}:
            if "native_test" not in types:
                problems.append(f"{key}: {status} requires committed native-test evidence")
        if status == "VISUAL_VERIFIED" and "visual" not in types:
            problems.append(f"{key}: visual verification missing real renderer evidence")
        if status == "ACCEPTED" and not {"source", "gate_approval"} <= types:
            problems.append(f"{key}: ACCEPTED without independent source/Gate approval")
        if status == "DONE":
            mandatory = {"source", "native_test", "gate_approval"}
            if item.get("type") in {"ui", "opening", "hud", "visual", "character_art"}:
                mandatory |= {"visual", "source_asset", "user_approval"}
            if item.get("type") in {"android", "release"}:
                mandatory |= {"device_test", "user_approval"}
            if not mandatory <= types:
                problems.append(f"{key}: DONE without independent proofs {sorted(mandatory - types)}")
            if item.get("accepted_against_frozen_source") is not True:
                problems.append(f"{key}: DONE without actual upstream Gate acceptance")
            if not item.get("acceptance_commit") or not item.get("tests_run_on_commit"):
                problems.append(f"{key}: DONE not pinned to accepted tested commit")
    # A report cannot bypass the ledger by writing "Status: DONE" in
    # Markdown while declaring BLOCKED in the machine-readable source.
    # Historic prose mentioning future DONE conditions is not a status.
    reports = repo / "docs" / "production"
    if reports.is_dir():
        formal_done = re.compile(
            r"(?im)^\s*(?:\*\*)?(?:status|completion)(?:\*\*)?"
            r"\s*:\s*(?:\*\*)?DONE\b"
        )
        claim_id = re.compile(
            r"(?im)^\s*Completion-ID:\s*(gate\d{2}\.[a-z0-9_.-]+)\s*$"
        )
        statuses = {row["id"]: row["status"] for row in items if isinstance(row, dict) and "id" in row}
        for report in reports.rglob("*.md"):
            try:
                text = report.read_text(encoding="utf-8-sig")
            except (OSError, UnicodeError):
                continue
            if formal_done.search(text):
                match = claim_id.search(text)
                key = match.group(1) if match else ""
                if not key or statuses.get(key) != "DONE":
                    problems.append(
                        f"{report.relative_to(repo).as_posix()}: "
                        "unregistered formal DONE claim"
                    )
    return problems


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", type=Path, default=ROOT)
    ap.add_argument("--ledger", default="docs/production/ACCEPTANCE_LEDGER.json")
    args = ap.parse_args()
    repo = args.repo.resolve()
    path = repo / args.ledger
    if not path.is_file():
        print("COMPLETION_CONTRACT_FAIL: no authoritative ledger", file=sys.stderr)
        return 1
    try:
        obj = json.loads(path.read_text(encoding="utf-8-sig"))
    except (OSError, ValueError) as exc:
        print(f"COMPLETION_CONTRACT_FAIL: {exc}", file=sys.stderr)
        return 1
    errors = validate(repo, obj)
    for error in errors:
        print("FAIL:", error, file=sys.stderr)
    if errors:
        print(f"COMPLETION_CONTRACT_FAIL count={len(errors)}", file=sys.stderr)
        return 1
    states = {}
    for item in obj["items"]:
        states[item["status"]] = states.get(item["status"], 0) + 1
    print("COMPLETION_CONTRACT_PASS", json.dumps(states, sort_keys=True))
    print("PASS only confirms truthful declared statuses; not production acceptance.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
