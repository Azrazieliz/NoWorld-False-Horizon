# Gate 01-3D — native editable character production recovery

**Role:** native Koikatsu Party asset creation, saved cards, source materials, mods, Blender rigs/morphs, 3D outfit states and technically accurate per-character source packages. Distinct from Gate01-2D. Consume existing approved visual contract; **never remake Luminous Base or user-authored Alter**. Gate11 alone canonical packages/identity, Gate12 Unreal actor implementation.

**Native source authority verified on Windows, 2026-10-16:**
- Approved Luminous V3 Base handoff:
  `D:\Tools\GameAssetPipeline\KoikatsuParty\handoffs\GATE01_LUMINOUS_V3_GATE11_GATE12_APPROVED_3D_BASELINE_20261011.zip`
  SHA256 `c83d71b687e7760cd4d3f4f2f8b2ec11d3ac28c78c8030367b83723e0dce2456`. 32/32 internal source hashes previously verified.
- **New combined Base and Alter source handoff**:
  `D:\Tools\GameAssetPipeline\KoikatsuParty\handoffs\GATE01_LUMINOUS_BASE_AND_ALTER_NATIVE_3D_SOURCES_20261016.zip`
  SHA256 `3fe24e74842bef3a268de4c8acd055372b6f50993ee26e3424c1e3de504f9812` (215,485,842 bytes; 131 entries in verified receipt).
- Authoritative local manifest:
  `D:\Tools\GameAssetPipeline\ProductionAutomation\Gate01\LuminousAlterCandidates_20261016\02_USER_CONFIRMED_FINAL_NATIVE_SOURCE_MANIFEST.json`;
  receipt `03_HASH_LOCKED_HANDOFF_RECEIPT.json`.
- **Alter state mapping USER-CONFIRMED AND HASH-LOCKED (DO NOT REPEAT AUDIT):**
  - CLOTHED `Koikatu_F_20261013225235666_Valentine Luminous.png` — SHA `9cd089e70890ae7a120c14156652b03919433ed0108530e00c5eedac2f608c9a`.
  - UNDERWEAR `Koikatu_F_20261013224655353_Valentine Luminous.png` — SHA `4aac8ec3e4d6c76828c5adcf0a126680c7d15c937257f894dec0d5f04d4e124a`.
  - NUDE **latest overwrite saved 2026-10-16** `Koikatu_F_20261013224249541_Valentine Luminous.png` — SHA `15d51bcebde9599a986c0e585313c34119d428327b713c770e874b6fe2f0f254`.
  - Old NUDE audit-only candidate SHA `fccab2193785ac5bf1d13ecd5d861901345a7ec50d22000c58a7ff4ebe8a8be1` is not the current design. Other coords 1–6 are inherited, not approved Alter states.
- Exact mod/shader/texture inventory exists in `MATERIAL_SOURCE/`, `READ_ONLY_AUDITS/`. Existing Base 3D Blender/FBX/GLB three-state files must stay intact. `kkbp_native_export`/FBX/GLB new Alter outputs not accepted final (manifest says NOT_YET_PERFORMED; later separate raw PMX forensic exports may exist, don't conflate with accepted pipeline).

**Quality HOLD:** Existing current generic Blender glTF conversion demonstrably discards material/normal channels (GLB 40 base-color texture bindings and ZERO normalTexture bindings for Base CLOTHED) and EU opaque import loses upper-eyeline gradient. Source geometry survives scale within measured stages; perceived size is camera-dependent. Body 15 nonzero morphs→9 UE imported despite zero threshold trials. 975-joint wardrobe states differ 193 paths pairwise. Do not convert/export *mass new characters* through uncorrected pipeline; isolated provenance-matched QA is allowed.

**Software Mastery:** `D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Unified_Master`. Run preflight, `gate_start.py --gate 01`, learned-method/regression suite. Previous authoring/analysis script `native_3d_material_scale_guard.py` and method K029; check current master before repeating work. Source master head may have advanced.

**Next SINGLE executable step:** hand Gate12 and Gate11 the already hash-locked combined archive/manifest and write compact `CHARACTER_3D_CONTRACT.md` preserving materials/wardrobe and reporting exact Unreal bindings not yet supplied. Continue controlled pipeline QA only with actual comparative images; no design reapproval. Commit small reproducible source/scripts/manifests on your isolated 3D branch, keep protected private binaries locally by path+SHA.

**Stop rule:** if 3D source evidence and 3 wardrobe mappings are complete, mark `NATIVE_3D_SOURCE_DONE` and stop recapture. Do not require optional 2D Alter drawing. Separate downstream Gate12 visual fidelity and Gate11 ARTIFACT_READY stay open.
