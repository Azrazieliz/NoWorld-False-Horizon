# Gate 01 — Luminous Valentine 3D source intake UPDATED 2026-10-11

## Authenticated source discovered
User uploaded **one genuine embedded-data Koikatsu Party character-card PNG** corresponding to previously discovered Luminous Valentine source listing `https://koikatsucards.com/contents/pixiv-121515691` (creator attributed by listing to うまべや（休業）, original Pixiv `https://www.pixiv.net/artworks/121515691`). The previous claim that the uploaded 252×352 file was merely a preview was INCORRECT; image dimensions do not determine embedded data presence.

**Raw user-supplied bytes**: 14,461,753 bytes, SHA-256 `995e533c63f5db60a1d1dc6bd0ba6fc5a94b5704e1a7f94ac489e764d60e85a8`. The 252x352 visible PNG ends at byte 157143. Appended block contains `【KoiKatuChara】`, a nested PNG and a parsed MessagePack block directory with `Custom` (3672), `Coordinate` (70732), `Parameter` (595), `Status` (767), and `KKEx` (14082625). The extended character content references additional custom hairstyles and outfit assets including HEXA/Quokka. **This establishes a native-format KK card structure, but NOT an actual successful Party import/visible model.**

The card was preserved bit-for-bit and packaged in the conversation artifact `Gate01_LuminousValentine_KK_Authentic_Source_Intake_20261011.zip` alongside:
- Frozen 2D master `Gate01_Luminous_Valentine_LN_2D_FROZEN_2026-10-11.png` 941x1672 RGB, 1,939,595 bytes, SHA-256 `daea9a014a250a5e74dcbfc707bd0829992fc91d321d217fb690dad727d50413`.
- SHA256SUMS, structure and plugin inventory, staged read-only importer.
- No other candidate source cards. **No original-model fallback needed for this commission.**

## Current engineering boundary
The uploaded original PNG **resides in the current ChatGPT artifact runtime**; it is **NOT** mounted in the authorized Windows workstation's `Koikatsu Party/UserData/chara/female`. User needs to download the 15-MB intake ZIP into the connected PC's Downloads directory, after which Gate01 can stage via verified SHA and test in real Party. Never claim remote delivery or physical Party load before observed.

Windows Party preflight detected BepInEx KKAPI, Sideloader, KKBP Exporter and MoreAccessories; only one installed pose ZIPMOD in `mods`. Gate 1.5 HF Patch v4.2 and other packs are STAGED and not installed, so missing hair/outfit assets and plugin-dependent appearance remain expected risks. Observe actual effect before adding mods; back up saves and avoid duplicate dependency installs.

Frozen Foundation `3f54f07cacbf193e160d0be15a7be5a7bc730c3b`, existing saves, unmerged PR16 and G12-KK-NONANIM reference untouched.

## Gate flags
2D_APPROVED=true; RAW_KK_SOURCE_RECEIVED=true; EMBEDDED_KK_BLOCKS_PARSED=true; WINDOWS_PARTY_SOURCE_COPIED=false; PARTY_LOAD_VERIFIED=false; MODEL_RECONSTRUCTED=false; GATE00_3D_FIDELITY_APPROVED=false; GATE11_ARTIFACT_READY=false; GATE12_NATIVE_PASS=false; GATE12_ANDROID_PASS=false.

Original creator rights for game redistribution have not been established; retain provenance and do not distribute source card assets in shipped build until reviewed.


## Later verified on the authorized Windows workstation (2026-10-11)
- The exact uploaded native KK original and approved 2D PNG were transferred automatically from connected Galaxy S26 Ultra Downloads using ADB. Both SHA-256 hashes revalidated on Windows: original `995e533c63f5db60a1d1dc6bd0ba6fc5a94b5704e1a7f94ac489e764d60e85a8`; frozen 2D `daea9a014a250a5e74dcbfc707bd0829992fc91d321d217fb690dad727d50413`. No user transfer required.
- Unmodified `KoiKatuChara` raw card is preserved under `D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\source_original`. A DISTINCT test copy changed only the header to `KoiKatuCharaS` (SHA-256 `17605a7b21b348b916bd365890abb884757483dce2086fed72872056fa503e13`); **the test copy was listed and actually loaded inside Koikatsu Party** as `バレンタインルミナス`, confirmed through Maker screenshot. Thus `PARTY_LOAD_VERIFIED=true` for the compatibility copy only.
- Actual pre-mod KKBP export produced a 6,264,381-byte PMX plus materials/textures; staged 228 files in `characters\LUMINOUS_VALENTINE_V3\diagnostics\initial_preHF_patch_export`, 47,590,412 bytes. The appearance is NOT acceptable: missing Gothic dress and custom hairstyle, misplaced face parts due absent mods. The pre-mod PMX is diagnostics only, NOT a G11 or UE artifact.
- Verified reversible 80-file backup of Party BepInEx, mods and UserData at `backups\pre_luminous_mod_install_20261011_verified`. HF Patch v4.2 full recommended 39.65-GB configuration was launched with GUI into the correct D: Party directory; archive validation succeeded, installation and extraction are **IN PROGRESS**. Do not interpret this note as install success.
- Eight precise Quokka/NullPopPo card-specific ZIPMOD dependencies were sourced from the public Sideloader Modpack, ZIP validated and GUID verified. Staged only pending HF duplicate audit. Character-specific source refs: Quokka `com.Quokka.classicmaidskirtlong`, `com.Quokka.maidapron`, `com.Quokka.maidinnerclassicshoulderlong`; NullPopPo `com.Nullpoppo.BangM`, `com.Nullpoppo.IdolCutL`, `com.Nullpoppo.IdolCutR`.
- 26 standalone editable Blender accessory objects (head bow, cross, four bodice ribbons, cuffs, gold boot accents) and 11 layered skirt/sleeve cloth objects have been authored, stored in `production\signature_geometry`. These are **unrigged draft meshes**, not modeled/approved final character or working Koikatsu ZIPMOD.
- A headless Blender KKBP crash was isolated to the add-on console toggle; session-local console guard applied for subsequent imports. Actual modeling must use restored post-mod source. No final 3D fidelity approval; G11/G12 acceptance remains locked.
