# Gate 01 — Luminous Valentine V3: Gate 00-approved 3D baseline handoff (2026-10-11)

## Production decision
Gate 00 issued **PROCEED**. The current Koikatsu Party character is accepted as a production-usable 3D baseline, retaining her recognizable identity, white/silver hair, heterochromia, reserved demeanor, and coherent face/body continuity across the three accepted CLOTHED, UNDERWEAR, and NUDE states. The remaining differences from the premium frozen 2D are explicitly **non-blocking**. Do not waste time on cosmetic reconstruction. This approval is not a Registry v2 acceptance or an Unreal/Android PASS.

## Verified handoff file (private authorized Windows workstation)
`D:\Tools\GameAssetPipeline\KoikatsuParty\handoffs\GATE01_LUMINOUS_V3_GATE11_GATE12_APPROVED_3D_BASELINE_20261011.zip`

- **Bytes:** 125,920,991
- **SHA-256:** `c83d71b687e7760cd4d3f4f2f8b2ec11d3ac28c78c8030367b83723e0dce2456`
- **Archive verification:** ZIP CRC passed; 32 original/supplemental asset payloads independently rehashed to SHA-256. Three additional root documents include the decision, handoff instructions, approval manifest and hash ledger.
- **Existing untouched preapproval package:** SHA-256 `0fd7d10d09b925bbd27ae26771a3559d6664171984c1c916e0cc0bc96c62f782`.
- **Selected splash artwork/frozen art component A:** `GATE01_LuminousValentine_Gate00_SelectedArt_and_Frozen2D_20261011.zip`, SHA-256 `edb9a048ac978e2cdb629557130482cd605b86f1496dba9ad11e904eed7d8164`. Previously retrieved by Gate00. Not a separate character form.

## Primary real Koikatsu Party user-saved cards (three clothing states of one character)

| Role | Relative path in handoff | SHA-256 |
|---|---|---|
| Clothed | `GATE01_LUMINOUS_V3/03_KK_PARTY_CARDS/PRIMARY_USER_SAVED/CLOTHED.png` | `34f4ee644087797cdad955dcc53f69a90479136b582a4359bf3eaa3673e04424` |
| Underwear | `GATE01_LUMINOUS_V3/03_KK_PARTY_CARDS/PRIMARY_USER_SAVED/UNDERWEAR.png` | `f9770a7432fb32433b701b33b064786ad021e1ac7eccfd6f339211b0af4524d5` |
| Nude | `GATE01_LUMINOUS_V3/03_KK_PARTY_CARDS/PRIMARY_USER_SAVED/NUDE.png` | `b0298bbb3ebad6ce5e45e0d11437ed56891a106788ee6f6d8c510fbab22ec3de` |

**Important:** Source cards' meaningful clothing contents are in coordinate **0**. Other coordinate presets are not completely normalized. In Party, load each card, choose coordinate 0 and display mode CLOTHED; check physical clothing and accessories, not just the Maker's superficial mode toggle. Older all-seven-coordinate auto-normalized cards are retained for troubleshooting but not authoritative.

The original creator KK PNG SHA-256 is `995e533c63f5db60a1d1dc6bd0ba6fc5a94b5704e1a7f94ac489e764d60e85a8`. The frozen V3 modeling 2D image SHA-256 is `daea9a014a250a5e74dcbfc707bd0829992fc91d321d217fb690dad727d50413`. Both originals remain unmodified.

## Dependencies and editable materials
The new handoff includes eight exact required Quokka/NullPopPo ZIPMODs, totaling about 18.83 MiB, with per-mod GUID, version, original install path and SHA-256. HF Patch v4.2 was installed on the workstation with 4,524 ZIPMODs and 248 plugin DLLs at audit time; its whole 40-GB collection is **not** duplicated into the handoff. Check actual Sideloader output for any further missing GUIDs. The auxiliary working Koikatsu card and source Blizzard Blender/.blend+GLB outfit drafts are preserved. The standalone drafts are **unfitted** and **not** final meshes.

## Gate 11 handoff
Review creator and third-party mod licensing and provenance, source bytes, hashes, Registry v2 format/checklist and submission. One character and three clothing states, not three manifestations. **Status: SOURCE INTAKE READY, not ARTIFACT_READY.**

## Gate 12 handoff
Open each primary genuine Party card, validate state and dependencies, export the actual approved **post-HF Patch** model (not the earlier prepatch diagnostic Blender export), preserve skeleton, clothing meshes, morph targets and texture masters; assemble in Unreal Engine **5.8.3** and test the real runtime on the **Samsung Galaxy S26 Ultra**. Validate clothing states, expressions, animation/bone synchronization, clipping, renderer and performance independently. **Status: SOURCE READY FOR EXPORT, not UE native PASS, not Android PASS.**

## Additional safeguards
Creator credited `うまべや（休業）` (Pixiv: https://www.pixiv.net/artworks/121515691), license/redistribution permission not yet verified; keep archive private while Gate11 checks. Preserve Foundation commit `3f54f07cacbf193e160d0be15a7be5a7bc730c3b`, existing saves and unmerged PR #16. Previous Gate 01 mismatch audit remains truthful historical observation, but its former aesthetic blocking status is superseded by Gate 00's explicit **PROCEED** decision.
