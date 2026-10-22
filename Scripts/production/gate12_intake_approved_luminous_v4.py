"""Copy *exact* Gate 00 approved v4 source bytes into the native project's data intake.
Never replace previous v6/v1 assets, edit user configuration, or canonize unapproved art.
"""
from __future__ import annotations
import hashlib
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[2]
REF = "coordination/gate00-live-ops-20261016"
SOURCES = {
    "Gate04_Luminous_FiveForms_Combat_v4_ORIGINAL_APPROVED.json":
        ROOT / "Content/NoWorld/Combat/AuthoritativeDesign/Gate04_Luminous_FiveForms_Combat_v4_ORIGINAL_APPROVED.json",
    "Gate04_GP_AND_CHARACTER_LINKS_CONTRACT_v4_ORIGINAL_APPROVED.md":
        ROOT / "docs/production/gate12/approved_sources/Gate04_GP_AND_CHARACTER_LINKS_CONTRACT_v4_ORIGINAL_APPROVED.md",
}
EXPECTED_COUNTS = {"Base": (3,6,2), "Alter": (3,6,2),
                   "Awakened": (4,5,1), "Berserker": (2,7,1),
                   "Grand": (3,6,3)}

def main():
    receipts = {}
    for name, output in SOURCES.items():
        ref = f"{REF}:docs/production/gate00/approved_sources/{name}"
        original = subprocess.check_output(["git", "-C", str(ROOT), "show", ref])
        if output.exists() and output.read_bytes() != original:
            raise RuntimeError(f"Existing differing production asset preserved: {output}")
        output.parent.mkdir(parents=True, exist_ok=True)
        if not output.exists():
            output.write_bytes(original)
        actual = output.read_bytes()
        assert hashlib.sha256(original).digest() == hashlib.sha256(actual).digest()
        receipts[name] = {"sha256": hashlib.sha256(original).hexdigest(), "bytes": len(actual)}
    source = json.loads(SOURCES[next(iter(SOURCES))].read_text(encoding="utf-8"))
    for form, (actives, passives, ultimates) in EXPECTED_COUNTS.items():
        row = source["forms"][form]
        actual = (len(row["actives"]), len(row["passives"]), len(row["ultimates"]))
        if actual != (actives, passives, ultimates):
            raise RuntimeError(f"Approved v4 ability counts mismatch: {form}: {actual}")
    print(json.dumps({"validated_v4_forms": list(EXPECTED_COUNTS),
                      "exact_verified_copies": receipts}, indent=2))
if __name__ == "__main__":
    main()
