# Gate 00 — First-World R1 region-kit intake and executable downstream handoff

**19 October 2026 | P0 environment | Gate02 source milestone RECEIVED/VERIFIED; art NOT APPROVED; Unreal/Android NOT TESTED.** This is a production intake, not permission to activate canonical locations, overwrite the temporary StartingWorld, or ship the first World.

## Actual receipt (independently verified on authorized Windows PC)

- ZIP: `D:\NoWorld_Art_Production\NoWorld_FirstWorld_RegionKit_R1_20261019.zip` — **715,075,060 bytes**; SHA-256 `0b12490cf3559f134b1ec40514c318604e0f4c27ae9a5468a1328460b7eb1db9`.
- Unpacked: `D:\NoWorld_Art_Production\NoWorld_FirstWorld_RegionKit_R1_20261019`; do not alter original package or publish binaries. Editable full P34 source at `source/NW_FirstWorld_R1_Master_EDITABLE.blend`, P12–P34 history preserved separately.
- Actual Gate00 rerun: `python qa/verify_region_kit.py` **PASS** (53 verified file hashes, 5 binary FBX anchors, 8 valid GLB, no duplicate among 10,454 IDs); `python procedural/regenerate_scatter.py --check` **PASS**, 10,454 / 625 128m tiles, SHA-256 `f6375aa36356164e76bedf8713b77fe35223b2c6d7e05088408d3f90a28fd8f5`, byte-identical; ZIP 54 entries, whole-archive CRC **PASS**.
- Actual Blender Cycles screenshots A–E were viewed. Valid distinct anchors are present, but painterly anime PBR / AAA finish does **not** pass: the citadel reads small against harsh raw geological masses, the walled market has tiny/repeated houses and thin civic/street life, fields are broad flat empty rectangular patches, woodland settlement lacks strong inhabited detail, slopes are visually raw, and the river is not a traversable connected water system. No independent frozen-reference side-by-side pixel match or in-game render acceptance is claimed.
- The candidate hierarchy is retained for isolated content staging: citadel, Eastern market town, Southern fields/windmill, Watchwood hamlet/lookout and regional roads. **This is not final artistic approval, a canonical region, or the entire first World.** R1 supplies a 3,200×3,200m subregion only; broader World scope remains outstanding.

## Gate12 — execute native intake using real source, no replacement architecture

**Authoritative read:** `contracts/GATE12_UNREAL_PCG_HANDOFF.md`, `contracts/native_FBX_exports.json`, `contracts/authored_GLTF_asset_exports.json`, `contracts/anchor_registry_candidate.json`, `contracts/world_package_contract.json`, `procedural/baked_seed1412_scatter.jsonl`, `qa/FULL_FILE_SHA256_MANIFEST.json` under extracted R1 root above.

1. Preserve dirty `D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010` and frozen Foundation. Respect Gate12's later user instruction to defer further app builds while completing feasible source work. Check processes/RAM before any native import; do not interrupt a running job. **Stage in an isolated QA world**, not `Content/Maps/StartingWorld.umap` or active World release.
2. Start with **one small real source anchor** `anchors/regional_roads.fbx` (98,140 bytes) plus `modules/moss_rock.glb` (6,781,532 bytes) to measure FBX embedded WORLD-SPACE versus glTF LOCAL/Y-up, pivot and cm scaling. Then trial the actual other anchors, preserving authored local-to-world transforms and avoiding doubled center offsets. Record material/texture mapping, screenshots, bounding boxes and evidence. The eastern town FBX alone is 263,914,108 bytes, so do not blindly bulk import on known low-memory hardware.
3. Verify heightmap actual 513×513 R16 / float32 encoding `z=-20+(u16/65535)*770m`, 6.25m sample pitch, 3.2km terrain, river/rock/soil masks and road/settlement alignment. Demonstrate character-scale collision/nav and first 128m tile's exact stable IDs with deterministic replay, then staged multiple tiles, persistence after world reload, streaming/LOD/memory. No fake scenery or rerolling destroyed/harvested objects.
4. **Existing `OfflineGame.uproject` (Gate12 `0e1004c`) lists SQLiteCore, ProceduralMeshComponent, AndroidPermission, PlatformCrypto and PythonScriptPlugin, but NOT an enabled PCG plugin.** Native World uses `Content/Maps/StartingWorld.umap`, `OGStartingRegionPresentation` and `OGFoundationIntegrationWorld`. Gate12 must inspect actual UE5.8.3 PCG/World Partition availability and adapt the existing project in its own authorized branch; do not assert PCG graphs/World Partition already work, nor introduce a redundant World architecture.
5. Commit/push **native evidence**, staged import/source, sample terrain and tile reconstruction, screenshots, collision/nav/streaming receipts and specific mobile optimization findings in the Gate12 isolated branch. Full scenic activation requires Gate00 artistic signoff, valid Gate11 provenance/content acceptance and actual in-game gameplay/Android tests. No hand-authored new spawn/faction/quest/boss or forced first fight.

## Gate02 — specific refinement commission on existing editable P34 production

Keep all established anchor locations, source history, deterministic seed/IDs and approved broad medieval fantasy direction. **Do not discard/restart working geometry.** For the next artistically reviewable revision: (a) architectural scale, silhouette and visual dominance in in-world player-camera views; (b) more inhabited, varied Eastern town with credible street widths, crafted facades, grounded market objects and aging materials; (c) real graded planted-field texture and crop/track/bank details, integrated windmill and service paths; (d) woodland canopy variety/density and hamlet path integration; (e) geological layering, river water volume/banks/crossings, bridges, caves, traversable collision-friendly paths and first accessible interior prototypes. Include matched before/after native Blender renders, FBX/GLB updated source, transform provenance and Android-aware mesh/material LOD proposal. **Do not make fixed routes or compulsory opening encounters.** Close art quality per actual screenshot/render comparisons, not counts of placed objects.

## Gate11 — provenance gate only when eligible

Review original P03 reused/licensed third-party source material, redistribution rights and authored-vs-reused attribution. Compare exact original hash-locked R1 inventory, Gate02 approved visual revisions and Gate12 actual native import/streaming proof. Do not register `NW_FW_R1_*` **candidate** anchor labels as canonical IDs, activate locations, create Registry content or claim `ARTIFACT_READY` before applicable approvals.

## Stop conditions

- **DONE now:** Gate00 receipt / independent QA / staged hierarchy decision and routed actionable owner requirements.
- **OPEN:** professional art quality, connected water/bridges/interiors/caves, gameplay traversal, native terrain/PCG/partition, collision, streaming, provenance and Android budgets. Do not overstate a 3.2km kit as the completed first World.
- All work proceeds alongside separate Gate12 gameplay tasks; no circular blocking on Gate02 refinement for a small isolated integration QA.
