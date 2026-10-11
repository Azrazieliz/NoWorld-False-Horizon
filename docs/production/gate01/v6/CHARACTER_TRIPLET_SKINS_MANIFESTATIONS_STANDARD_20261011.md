# Gate 01 — Character states, skins and manifestations (Gate 00 directive 2026-10-11)

## Authority and scope
Apply to Original Character Creation, Feminisation and Existing Character Adaptation as a Gate 01 production-output rule. Preserve all existing methodologies and 2D-first creative gates. This contract is a new packaging/variants overlay; it does not alter frozen Foundation, Gate 11 Registry v2, or Gate 12 validation requirements.

## One directory per actual character / variant
`characters/<canonical_character_ID>/<form_ID>/` is the primary full editable production directory. Under each approved base and later separately approved skin/manifestation maintain:
- `00_FROZEN_2D/`: exact user-approved 2D art, immutable SHA-256, lineage and approvals;
- `01_CARDS/`: three individually loadable, hash-verified authentic Koikatsu Party character PNGs: `CLOTHED.png`, `UNDERWEAR.png`, and `NUDE.png`;
- `02_EDITABLE_3D/`: rig, blend masters, ZIPMODs, outfit sources, textures and morph files;
- `03_QA/`: load-test captures, per-state selected coordinate and 3D-vs-2D fidelity tests;
- `04_PROVENANCE/`: original untouched card(s), author, URL, source hash, rights and dependency manifests;
- `MANIFEST.json`: hashes, lineage, approval status, physical load QA and related forms.

Never confuse (A) the Maker's Clothed/Underwear/Naked **display** switches with (B) permanent stored outfit data in a character card. Every one of the three cards must encode its own effective clothing-part configuration *in every intended outfit coordinate*, and be independently loaded in Koikatsu Party with the display switch set to `Clothed` to demonstrate that the card itself owns the state. Do not mark a card PARTY_LOAD_VERIFIED from thumbnail, PNG structural correctness, filename or checksum alone. Preserve identity/body/face/hair/non-clothing accessories across the three states. Handle clothing attached as accessories case-by-case so explicit garment remnants do not remain on nude/underwear versions; preserve canonical nonhuman anatomical features.

Each character output requires 3 separate cards, and the same applies to each approved skin and each approved manifestation after its 3D design is completed. Do not mutate one shared card in place. Do not delete older saves; isolate legacy files and label old display-only exports non-authoritative. Preserve original source bytes as before.

## Skins and manifestations
- **Skin:** identical established character identity, anatomy and species, recognizable face, baseline powers/ontology; changes may include costume, hairstyle, accessories, surface textures, overall stylistic presentation. The new skin has its OWN proposed concept art, user-refinement loop and explicit 2D freeze before any skin-specific 3D authoring.
- **Manifestation:** a character-specific alternate form comparable to a Fate Alter; face, anatomy, supernatural morphology, transformation system and costume can evolve meaningfully. Must remain unmistakably the same canonical character. Each form requires its OWN canonical-anchor/collision check, user-approved frozen 2D first, and any unusual anatomy escalated rather than simplified in Koikatsu or Unreal.
- Skins cannot be mere recolors; manifestations cannot become generic replacement characters or unrelated originals.

## Stop-and-choose checkpoint
Once the BASE three-card set is copied, hashed, **independently opened and visually validated**, STOP further character production and submit several distinct character-specific SKIN and MANIFESTATION concepts with actual functional user choice/revision controls. Do not start variant art or modeling without selection. User chooses candidates, approves individual 2D freeze, then and only then proceed to Party 3D, three persistent clothing-state card outputs and independent fidelity review. Base 3D fidelity QA remains a separate mandatory sign-off.

## Luminous Valentine checkpoint (2026-10-11)
Source original SHA-256 `995e533c63f5db60a1d1dc6bd0ba6fc5a94b5704e1a7f94ac489e764d60e85a8` untouched. Frozen v3 2D SHA-256 `daea9a014a250a5e74dcbfc707bd0829992fc91d321d217fb690dad727d50413`.
Character directory: `D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\`; immutable safe backup ZIP `LUMINOUS_VALENTINE_BASE_STATES_SAFE_20261011.zip`.
State copies: `base_states/LUMINOUS_BASE_CLOTHED_KK.png` SHA `484190e00cfa00295cfe2e492012eb10721f0e8fa98abe47d18b17bbe0e69f9c`; `base_states/LUMINOUS_BASE_UNDERWEAR_KK.png` SHA `01eb529484692a5462fd784643d66867f31d83f447d24c28a17c5c303773626a`; `base_states/LUMINOUS_BASE_NUDE_KK.png` SHA `5b68f5a2c01d2de6338c28dca730c926dc4590ec1bcc6de6050120c2e605b05c`.
All three embedded card data and all 7 outfit-coordinate clothing IDs have been verified and archive ZIP entries rehashed. They have been deployed to the in-game directory `Koikatsu Party\UserData\chara\female\LUMINOUS_VALENTINE_V3\`. Earlier `Koikatu_F_*` saves were generated using UI display toggles and are NOT authoritative costume-state files.
**QA NOT FULLY CLOSED**: nude variant has not been independently observed after a fresh explicit Party load in `Clothed` display mode (one prior diagnostic screenshot showed the underwear variant rather than nude). Do not misrepresent the base triplet as fully Party-validated. Clothed and underwear were observed. Once user is no longer interacting with the currently open game, verify nude explicitly, then seal QA. No original card/2D alteration, no new variant artwork, no Gate11 ARTIFACT_READY or UE/Android PASS.
