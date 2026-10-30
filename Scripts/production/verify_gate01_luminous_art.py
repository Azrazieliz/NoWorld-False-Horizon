#!/usr/bin/env python3
"""Verify the exact Gate01 accepted Luminous images and native form bindings."""
from __future__ import annotations

import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ART = ROOT / "Content/NoWorld/Characters/Luminous/Art/Accepted2D"
MANIFEST = ART / "APPROVED_FIVE_FORMS_MANIFEST.json"
CPP = ROOT / "Source/OfflineGame/Private/UI/OGGate12CharacterVisualCatalog.cpp"
BUILD = ROOT / "Source/OfflineGame/OfflineGame.Build.cs"

EXPECTED = {
    "Base": ("01_BASE.png", "a1c52d137e24512528adc60556092727f0e37d38db35a470343eb3a429307272", "character:luminous_valentine.version_ln_base"),
    "Alter": ("02_ALTER.png", "90a8d6f93b97cdd035fdbcf316975ca08efffd4615357c8b3abffcf05a28d759", "character:luminous_valentine.version_alter_v1"),
    "Awakened": ("03_AWAKENED.png", "65184e18f3809dc85dd40e3f4af6fdda6d7b8266cfe59f46fdc60a8a4eede27b", "character:luminous_valentine.version_awakened_v4"),
    "Berserker": ("04_BERSERKER.png", "84b394e8c433ade6fae0dbacd3d58ac2785b40784969220b2b46a5b7f75542e8", "character:luminous_valentine.version_berserker_v4"),
    "Grand Convergence": ("05_GRAND_CONVERGENCE.png", "3f2d4aea44a2977ac675bbc2b036ebff8a15463d64b5eb7c3211e52e0e6d22e3", "character:luminous_valentine.version_grand_v4"),
}


def verify() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    assert manifest["schema"] == "G01_2D_ACCEPTED_LUMINOUS_FIVE_FORMS_1"
    forms = manifest["forms"]
    assert len(forms) == len(EXPECTED)
    assert {f["form"] for f in forms} == set(EXPECTED)
    cpp = CPP.read_text(encoding="utf-8")
    build = BUILD.read_text(encoding="utf-8")
    for form in forms:
        name, digest, version = EXPECTED[form["form"]]
        assert form["sha256"] == digest, form["form"]
        assert form["approved_art_file"].endswith("/" + name), name
        assert form["approval"] == "USER_VISUALLY_ACCEPTED_2026-10-20", name
        data = (ART / name).read_bytes()
        assert hashlib.sha256(data).hexdigest() == digest, name
        assert data[:8] == b"\x89PNG\r\n\x1a\n", name
        assert struct.unpack(">II", data[16:24]) == tuple(form["resolution"]), name
        assert name in cpp and version in cpp, name
        assert name in build, name
    assert "APPROVED_FIVE_FORMS_MANIFEST.json" in build
    assert "BindCanonicalCharacter" in (
        ROOT / "Source/OfflineGame/Private/Koikatsu/OGLuminousV3PlayableCharacter.cpp"
    ).read_text(encoding="utf-8")
    print("PASS: 5 exact SHA-256 original artwork files; native version mapping; packaging contract")


if __name__ == "__main__":
    verify()
