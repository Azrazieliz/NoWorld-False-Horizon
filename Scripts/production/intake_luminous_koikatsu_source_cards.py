#!/usr/bin/env python3
"""Preserve the 15 genuine Koikatsu Luminous source cards, unchanged.

This is source intake, not completed Unreal mesh/animation import or 3D approval.
An original Koikatsu PNG contains binary character data AFTER the thumbnail;
NEVER crop, re-encode or save it using an image editor.
"""
from __future__ import annotations
import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(r"D:\Tools\Steam\steamapps\common\Koikatsu Party\UserData\chara\female")
TARGET = ROOT / "Assets/NoWorld/Luminous/KoikatsuOriginalCards"
# Direct visual inspection of the 15 original game-card thumbnails on 2026-10-30.
# Mapping is from actual file names plus card imagery, not a synthetic mesh.
CARDS = (
    ("Base", "CLOTHED", "LUMINOUS_VALENTINE_V3/LUMINOUS_BASE_CLOTHED_KK.png"),
    ("Base", "UNDERWEAR", "LUMINOUS_VALENTINE_V3/LUMINOUS_BASE_UNDERWEAR_KK.png"),
    ("Base", "NUDE", "LUMINOUS_VALENTINE_V3/LUMINOUS_BASE_NUDE_KK.png"),
    ("Alter", "CLOTHED", "Koikatu_F_20261013225235666_Valentine Luminous.png"),
    ("Alter", "UNDERWEAR", "Koikatu_F_20261013224655353_Valentine Luminous.png"),
    ("Alter", "NUDE", "Koikatu_F_20261013224249541_Valentine Luminous.png"),
    ("Awakened", "CLOTHED", "Koikatu_F_20261020163646604_Valentine Luminous.png"),
    ("Awakened", "UNDERWEAR", "Koikatu_F_20261020163254893_Valentine Luminous.png"),
    ("Awakened", "NUDE", "Koikatu_F_20261020164224738_Valentine Luminous.png"),
    ("Berserker", "CLOTHED", "Koikatu_F_20261020181953705_Valentine Luminous.png"),
    ("Berserker", "UNDERWEAR", "Koikatu_F_20261020190327674_Valentine Luminous.png"),
    ("Berserker", "NUDE", "Koikatu_F_20261020190405616_Valentine Luminous.png"),
    ("Grand Convergence", "CLOTHED", "Koikatu_F_20261020202050734_Valentine Luminous.png"),
    ("Grand Convergence", "UNDERWEAR", "Koikatu_F_20261020200911508_Valentine Luminous.png"),
    ("Grand Convergence", "NUDE", "Koikatu_F_20261020200119808_Valentine Luminous.png"),
)

def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024*1024), b""):
            digest.update(block)
    return digest.hexdigest()

def intake() -> None:
    TARGET.mkdir(parents=True, exist_ok=True)
    records = []
    for form, wardrobe, relative in CARDS:
        source = SOURCE / relative
        if not source.is_file():
            raise FileNotFoundError(source)
        name = "LUMINOUS_" + form.upper().replace(" ", "_") + "_" + wardrobe + "_KK.png"
        dest = TARGET / name
        expected = sha256(source)
        if dest.exists():
            if sha256(dest) != expected:
                raise ValueError("Refusing to overwrite different source card: "+str(dest))
        else:
            shutil.copy2(source, dest)
        if sha256(dest) != expected:
            raise RuntimeError("Failed byte-identical source copy: "+name)
        with dest.open("rb") as data:
            if data.read(8) != bytes.fromhex("89504e470d0a1a0a"):
                raise RuntimeError("Not an original PNG source card: "+name)
        records.append({
            "identity": "character:luminous_valentine.identity",
            "form": form,
            "wardrobe": wardrobe,
            "source_original_filename": source.name,
            "repo_path": dest.relative_to(ROOT).as_posix(),
            "sha256": expected,
            "size_bytes": source.stat().st_size,
            "file_kind": "KOIKATSU_SOURCE_CARD_PNG_WITH_EMBEDDED_CHARACTER_DATA",
            "runtime_3d_import": "NOT_VERIFIED",
        })
        print(form, wardrobe, name, expected)
    assert len(records) == 15 and len({r["sha256"] for r in records}) == 15
    manifest = {
        "schema": "noworld.luminous_original_koikatsu_source_cards.v1",
        "original_source_directory": str(SOURCE),
        "artwork_reference_manifest": "Content/NoWorld/Characters/Luminous/Art/Accepted2D/APPROVED_FIVE_FORMS_MANIFEST.json",
        "canonical_identity": "character:luminous_valentine.identity",
        "source_card_count": 15,
        "form_count": 5,
        "character_level_animation_library": "Content/NoWorld/Animation/SharedMotionCatalog.json",
        "status": "BYTE_IDENTICAL_SOURCES_STORED_NOT_ENGINE_IMPORT_OR_GAMEPLAY_ACCEPTED",
        "sources": records
    }
    (TARGET / "SOURCE_CARDS_MANIFEST.json").write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+"\n", encoding="utf-8")
    print("SOURCE_MANIFEST", TARGET / "SOURCE_CARDS_MANIFEST.json")

if __name__ == "__main__":
    intake()
