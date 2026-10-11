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
