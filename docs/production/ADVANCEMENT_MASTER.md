# NoWorld: False Horizon — ADVANCEMENT MASTER

**Current production priority register — 23 October 2026.**
**Authority:** Director's Gate 00 → Gate 12 full-production instruction, 23 October 2026; later direct instructions take precedence. Prior binding Gate 00 approval overlay of 20 October governs Gate07, Gate11 retirement, First World ownership and the four-clip Android qualification. This is an **engineering completion tracker**, not a new design, Registry activation or creative approval.

**Target:** The actual offline, fully playable, visibly finished game running in Unreal Engine 5.8.3 and on authorized target hardware, not a technical showcase.

**Canonical Luminous character:** five forms — **Base, Alter, Awakened, Berserker, Grand Convergence**. The character is attached to **one** shared semantic animation bank; each form uses its actual native meshes and exact-compatible skeleton derivatives. Never create five independent motion libraries or duplicate common animations to route a form.

**Mirrored execution progression:** [`docs/production/ADVANCEMENT_PROGRESS.json`](ADVANCEMENT_PROGRESS.json). Every `G12-XXXX` entry below has exactly one matching progression record. A source change is not a native build or device acceptance.

**Authoritative production repository:** `D:/UnrealProjects/OfflineGame_Gate12_Koikatsu_20261010`
**Work branch at register creation:** `production/gate12-full-continuation-20261023`; audited HEAD `7165a21`. The tracked user modification to `Config/DefaultEditor.ini` and all original/untracked assets must remain untouched.

## Execution order and prioritization

Execute the numbered phases **in this order**, making the quickest executable corrections first inside each phase. Independent Gate source/art production can run in parallel, but integration proceeds as soon as valid files exist. A missing artistic dependency never justifies invented content, nor does it stop unrelated native work. A passing compile/asset import/test is not enough to mark a player-visible requirement finished.

**Task statuses:** `[x]` = strictly scoped verified implementation, NOT entire family completion; `[ ]` = unfinished. **P0** = blocks finished playable game/core runtime; **P1** = essential full scope or high-impact polish; **P2** = final release acceptance. **FAST** = likely bounded source/flow correction; **INTEGRATION** = substantial native wiring; **CONTENT** = depends on accepted actual assets; **DEVICE** = must be verified physically. These are sequencing classes, not invented timelines.

**Preserved confirmed baseline (do not rebuild):** persistent local SQLite / Foundation; real Ruler creation/navigation and recorded 23/23 *isolated* 18 Oct journey; Gate10 approved Gacha appearance; shared semantic animation controller/scene; 3,217 native motion registrations (16 gameplay-approved; 3,201 QA-gated; 900 tagged mature; zero registered participant roles), six native Luminous Alter parts in three wardrobes and shared Walk playback; First World R1 maps/native modules; approved Base v6 + separately approved Alter v1 designs. 23 Oct `7165a21` built successfully and passed genuine six-part NullRHI QA preview with 1,067 CLOTHED candidates, including 300 mature-tagged; no art/visual sign-off follows from that.

### 00 — Recover working visual/game production without regression (P0; fastest closures)

- [x] **G12-0001 / verified** — Confirm active Windows project, production branch, historical HEAD and protected editor modification; continuation branch established.
- [x] **G12-0002 / verified** — Preserve old successful and failed animation QA receipts and source assets; isolate new changes in recoverable commit `7165a21`.
- [ ] **G12-0003 / FAST** — Resolve Windows interactive display access for genuine editor/game screenshots. Latest display capture shows PIN lock screen; do not describe it as gameplay. Retest actual unlocked display once available, while continuing other tasks.
- [ ] **G12-0004 / FAST** — Eliminate Zen DDC `507 Insufficient Storage` (C: last observed ~2 GB available vs D: ~163 GB); relocate disposable derived cache non-destructively or recover safe space without touching original assets, saves, source, packages or user files.
- [ ] **G12-0005 / FAST** — Run one bounded source/asset/approval diff against **current** Gate00 frozen contracts and full worktree. Classify pending untracked imports without resetting, cleaning, mass reimporting or altering protected assets; distinguish existing functional runtime from missing actual content.
- [ ] **G12-0006 / FAST** — Confirm actual Android ADB/device/package state and available RAM/SDK. Use a separate dev package and preserve installed unrelated applications, existing APKs, user world data and device backups.
- [ ] **G12-0007 / INTEGRATION** — Preserve a stable real editor/game startup, selected owned player, persistence, World↔Ruler transition, optional World interaction and no mandatory first encounter; only fix observed regressions, not rebuild the existing 23/23 route.

**Exit:** Real application can be visually inspected, provenance/source safety retained, no unsafe editor/config/disk change.

### 01 — Remove unauthorized mature-content action exclusions (P0; quick and bounded)

- [x] **G12-0101 / verified** — Remove the **separate** `adult_restricted` reject in the central animation resolver; preserve strict per-source technical/visual approval.
- [x] **G12-0102 / verified** — Expose mature-tagged source animations in isolated editor QA without silently approving gameplay; repair transient native QA initialization, six-part source motion played.
- [x] **G12-0103 / verified** — Stop SFW visual wardrobe preference from independently rejecting an otherwise valid native gameplay motion; actual vs effective wardrobe policy remains distinct.
- [ ] **G12-0104 / FAST** — Inspect remaining adult/mature authorization and preview/action filters in production World interactions, UMG, combat and scene dispatch. Remove **only** gates not present in approved game rules; no newly invented access/consent/unlock mechanism or removal of approved player settings.
- [ ] **G12-0105 / INTEGRATION** — Keep actual clothing, physical state, inventory, ownership, consequences and completed actions identical across SFW/privacy render changes; verify all full-body/face/interaction/roster/world views mask only presentation as configured.
- [ ] **G12-0106 / CONTENT** — Implement all available technically sound mature interactions, anatomy layers, injury/gore/execution/death visuals and appropriate linked consequences **as approved**. A content tag alone is never proof of working animation or encounter.

**Exit:** No unauthorized censorship or eligibility check; content classification, game legality, source/art QA and privacy rendering are independent.

### 02 — One central reusable animation architecture, all characters (P0)

- [x] **G12-0201 / verified scoped** — Preserve game-wide shared source clip IDs, semantic action resolver, native controller, rig-based exact-Skeleton checks, multi-actor scene component and cross-wardrobe compatible selection from `7165a21`.
- [ ] **G12-0202 / INTEGRATION** — Make `/Game/NoWorld/Animation` + the shared catalog the discoverable **logical owner** of native motions, with named category/action/clip/rig/retarget/participant/quality mappings; retain legacy asset object paths and all references with redirects/aliases rather than destructive mass moves.
- [ ] **G12-0203 / FAST** — Ensure the JSON catalog is actually staged in development/shipping package, loaded without editor dependencies and resolves the correct pre-existing native clip by source ID (packaging rule is authored but not package-verified).
- [ ] **G12-0204 / INTEGRATION** — Attach **one canonical character identity** to the shared animation controller and global source catalog. All its Versions and wardrobes resolve the same semantic actions through the active mesh rig/anatomy/capabilities and compatible native derivatives; do not branch or author libraries separately per Version. Complete cross-character adapters and native verification before checking this item.
- [ ] **G12-0205 / INTEGRATION** — Bind semantic movement/combat/emote/interactions from the **real** action/ability/traversal state machines, with correct cancel, contact notify, duration, hit event, blend, montage/AnimBP, interruption, root motion, rotation, additive face/morph and post-action recovery.
- [ ] **G12-0206 / INTEGRATION** — Synchronize true multi-participant roles and tracks on one scene clock, with correct skeleton for every role (including other body types), relative positioning, contact windows, facing, time changes, abort/recovery and scene consequence dispatch.
- [ ] **G12-0207 / CONTENT** — Retarget/author compatible technical derivatives only when required for the actual native Skeleton; keep original SHA, source, artistic approval, rejected versions and deduplication provenance. No artificial asset-count inflation.
- [ ] **G12-0208 / INTEGRATION** — Verify wardrobe swap, character form change, equipment and SFW setting can continue/restart the proper shared motion without reauthoring a new library.
- [ ] **G12-0209 / DEVICE** — Qualify reused semantic animation in actual game visuals on real display/device with correct whole-body deformation, no pose tearing, foot slide, transform mismatch, freezes or clipping.

**Exit:** An approved common motion can be requested by multiple compatible real characters/Versions; a game state triggers it naturally; ineligible clips stay QA-only for genuine quality/rig reasons.

**30 October source checkpoint (not device proof):** `UOGSharedMotionControllerComponent::BindCanonicalCharacter` binds one character identity; Luminous's validated owned Manifestation attaches once, and legacy traversal fallback no longer selects a separate Base/Alter animation namespace. Shared resolver continues to enforce exact skeleton. Existing approved source catalog remains intact. Remaining: expanded rigs/retargeting, source validation, native compilation and Android gameplay qualification. See `ADVANCEMENT_PROGRESS.json` for task-by-task state.

### 03 — Complete actual animation coverage, not more registrations (P0)

- [ ] **G12-0301 / DEVICE — FIRST CHECKPOINT** — Demonstrate **the four existing genuine Gate09 Koikatsu native clips in actual Android gameplay** on an intended character, including animation, blend, correct rig and measured loading; Gate00's 20 Oct approval requires this before more *bulk* transfer. Do not discard the 3,217 already imported registrations.
- [ ] **G12-0302 / FAST** — Audit 3,217 catalog entries against source hashes, actual exported curves, real native Skeletons, compatibility, distinct clips, accepted/rejected QA; record source→native→rig→semantic→gameplay→visual/device progression independently.
- [ ] **G12-0303 / CONTENT** — Connect idle, directional walk/run/sprint, acceleration/deceleration, stop/turn, jumping, falling, landing, climbing, crawling and swimming through live traversal, camera and physics, including shared meaningful variations.
- [ ] **G12-0304 / CONTENT** — Connect hover, takeoff, landing, flying travel, aerial combat and aquatic transitions; respect anatomy, abilities, velocity and collisions, not a one-pose hovering approximation.
- [ ] **G12-0305 / CONTENT** — Integrate true attack, defense, dodge, parry, hit, stagger, counterattack, weapon grip and weapon-class motions (unarmed, sling/bow/projectile, blades, guns, futuristic, innate/magic, nonhuman).
- [ ] **G12-0306 / CONTENT** — Integrate dialogue, speech-like face, eyes, brows, gaze, emotions, idle reactions, facial morphs and secondary hair/clothing/appendage movement.
- [ ] **G12-0307 / CONTENT** — Integrate sit, stand, sleep, eat, drink, interact, carry, pick up, use item and contextual environmental interactions.
- [ ] **G12-0308 / CONTENT** — Deliver accepted full-length **8–20 second** dance/choreography, transitions, beats, foot-planting, body/face/garments and playable start/interrupt/stop; do not reuse rejected sway output as final dance.
- [ ] **G12-0309 / CONTENT** — Author/retarget **authentic synchronized interpersonal/adult scene role tracks** (current catalog role count = zero), with correct compatible participant skeletons, scene staging, timing and actual world interaction entry.
- [ ] **G12-0310 / CONTENT** — Integrate mature combat injury, blood/gore, execution, death/revival and aftermath motion where specified, with camera and actual state progression.
- [ ] **G12-0311 / CONTENT** — Integrate creature-specific, unusual body structure, aquatic/aerial and nonhuman movements without forcing humanoid assets.
- [ ] **G12-0312 / INTEGRATION** — Implement source-grade acceptance for every missing action category: authentic data + target compatible rigs + actual gameplay dispatch + synchronized timing + native playback + visual review (and device review for mobile).
- [ ] **G12-0313 / FAST** — Defer genuinely missing original Gate09/source assets by **exact motion, anatomical rig, participant role and acceptance blocker**; finish independent existing assets immediately instead of stopping at the animation inventory.

**Exit:** Each commissioned animation family exists, meets the game's art standard and plays in the actual applicable game system; QA entries are not counted as delivered actions.

### 04 — Rendered turn-by-turn/action/hybrid combat as real gameplay (P0)

- [ ] **G12-0401 / INTEGRATION** — Replace the black/text-only or diagnostic combat view **without rebuilding either frozen executor**; render actual owned combatants and enemies in the active World/encounter.
- [ ] **G12-0402 / INTEGRATION** — Set team formation/lanes, true actor position, facing, reach, distance, movement, range/elevation/line of sight and camera framing from real battle state.
- [ ] **G12-0403 / INTEGRATION** — Display legal current turn/actor, actions, skill set, Ultimate branches, targets, target previews, energy/resources and effects through actual Gate10 controls; keyboard/touch input and turn advancement must reach the canonical battle.
- [ ] **G12-0404 / INTEGRATION** — Couple authoritative action commit to synced semantic animation, actual weapon/ability release, contact frames, projectile/hit trace, defense reaction, sound, VFX and visual consequence; no invented invisible hit or independent fake HP ledger.
- [ ] **G12-0405 / CONTENT** — Integrate the exact **approved Luminous Base v6** (three actives, six passives, Day/Night Ultimate, World Fantasm) in the existing combat executors. Preserve resource/status/target/buff/heal rules; no diluted Fantasm.
- [ ] **G12-0406 / CONTENT** — Integrate independently approved **Alter v1** (three actives, six passives, Red Tithe, Bleed/Severing Wound, Rupture Seals, Black Mass branches, Cathedral Fantasm four effects, exact revival) from **original full approved numerical payload**; do not reconstruct missing multipliers/conditions from summary and never copy Base effects into Alter.
- [ ] **G12-0407 / CONTENT** — Support all other exact approved kits, weapons, natural anatomy, learned abilities, Ranks, statuses, special effects and non-human skills with matching source-specific visuals. Never default to a firearm or sword for unrelated equipment/ability.
- [ ] **G12-0408 / INTEGRATION** — Implement hit reactions, shields, healing, injuries, bleed, death, revive, gore, post-action state and real World consequences, respecting camera/Privacy and legitimate gameplay rules.
- [ ] **G12-0409 / INTEGRATION** — Integrate distinct enemies and encounters whose actual authored power is coherent with approved Gate04; no universal global level-scaling or forced enemy match to player.
- [ ] **G12-0410 / INTEGRATION** — Preserve deterministic turn order, action-value timing, interruptions, counters, cooldowns, triggers, World Manifestations/Negations, camera and transitions between turn/action modes.
- [ ] **G12-0411 / INTEGRATION** — Persist/restore encounter identities, state and aftermath; correctly handle victory/defeat, retreat, intervention, replaced participants, death and rewards without an arbitrary reset.
- [ ] **G12-0412 / VISUAL** — Film/inspect real end-to-end *player-selects-target → actor animates → actual hit/defense → HP/status changes → enemy takes next visible turn → fight resolves* using true authoritative gameplay systems, not a demonstration simulation.

**Exit:** Real visibly rendered interactive fight for a legitimately owned approved character, with believable attacks and damage, turn/action progression and persisted consequences. No mandatory first fight.

### 05 — Actual approved playable characters, Versions and Registry identity (P0)

**Recovered immutable Koikatsu 3D source cards — 2026-10-30:** Five Luminous forms (Base, Alter, Awakened, Berserker, Grand Convergence) each have three distinct original Koikatsu character-card files (CLOTHED, UNDERWEAR, NUDE), all preserved byte-for-byte in `Assets/NoWorld/Luminous/KoikatsuOriginalCards/` using Git LFS. Per-source hashes, original filenames and verified card-to-form/wardrobe mappings are in `SOURCE_CARDS_MANIFEST.json`. This is ORIGINAL SOURCE RECOVERY, **not** UE skeletal import, visual acceptance of 3D, animation-device verification or completed combat kit. The single canonical character connects to one shared semantic animation controller; source cards are not independent character identities or motion libraries.



- [ ] **G12-0501 / INTEGRATION** — Resolve authentic source→Identity→Version→owned independent Manifestation binding using canonical Registry v2 provenance; Gate00 is Registry acceptance owner, **Gate11 not an operational queue**. Never assign fake owned/test IDs.
- [ ] **G12-0502 / CONTENT** — Finalize true user-approved Luminous Base and Alter native meshes from existing authentic Koikatsu/Blender/UE work, comparing face, morphology, eyes/alpha, materials, scale, rig, costumes, anatomical components and whole-body appearance with reference (no redesign/re-export of already accepted parts unless corrective).
- [ ] **G12-0503 / INTEGRATION** — Integrate actual CLOTHED/UNDERWEAR/NUDE appearance components, clothing vs Privacy effective rendering, equipment changes, damage/injury and animation for the **same** Identity/Version, not separate characters.
- [ ] **G12-0504 / INTEGRATION** — Make owned Luminous controllable for exploration, navigation, encounter, action/turn combat, interactions, cinematic and Ruler 2D/3D full-size detail.
- [ ] **G12-0505 / INTEGRATION** — Wire each Version's **own** accepted skills/Ultimate/Fantasm and stat/progression state to real owned duplicates, respecting mutually exclusive party identity rules and source provenance.
- [ ] **G12-0506 / CONTENT** — Integrate other already approved, source-ready character and wardrobe families without re-opening closed approvals; each binds existing shared animation rigs.
- [ ] **G12-0507 / CONTENT — APPROVAL DEPENDENT** — Integrate Awakened, Berserker and Grand Convergence using their actual approved form content and exact rules, preserving owned source copies and progression. Reuse the single character-level shared motion controller with the correct native skeletons.
- [ ] **G12-0508 / INTEGRATION** — Prove two legitimately obtained copies of one Identity persist separately as independent Manifestations, with independent development/wardrobe/equipment. No character chips or fabricated duplicate conversion.
- [ ] **G12-0509 / VISUAL** — Compare live three-wardrobe full-body geometry, facial/morph expression, combat and traversal across valid hardware/viewers. Save, cold-reload and confirm original appearance, ownership and physical state retained.

**Exit:** Genuine approved character selected from real owned roster can freely explore, fight, interact, transform/appear according to approved Versions and retain state; no mock hero or unapproved source.

### 06 — Real complete explorable First World (P0)

- [ ] **G12-0601 / INTEGRATION** — Continue **existing** Gate02 R1 sourced assets and streaming/source assembly maps; retain native original kingdom anchors, citadel, Eastern Market, farms, Watchwood, lookout, roads and authentic source hashes. No reset to grey terrain.
- [ ] **G12-0602 / CONTENT** — Build real full First World terrain in Unreal with approved art-directed PCG/Landscape/World Partition or equivalent, seeded persistence; use Gate02 as the authored anchor supplier, not as the owner of procedurally building the entire World.
- [ ] **G12-0603 / CONTENT** — Expand coherent Three-Valley Confluence geography toward approved **100 × 100 km** first kingdom target, largely plains where specified, with appropriate landscape scale, hydrology, river/banks, bridge systems, cliffs, forests and farms.
- [ ] **G12-0604 / CONTENT** — Build inhabitable settlements, village/town scale, markets, castle/citadel proportion, roads, agricultural plots, ecology, landmarks, caves and traversable interiors using accepted existing anchor/module materials.
- [ ] **G12-0605 / INTEGRATION** — Configure correct UE world-space scale, source basis/pivots, material/texture and shaders, terrain seams, collision, navmesh, swimming/flying/climbing anchors, crowd and creature navigation.
- [ ] **G12-0606 / INTEGRATION** — Implement stable seed/tile/region/entity IDs, deterministic placement and streaming, LOD/HLOD/PCG population that does not overwrite dynamically changed player state.
- [ ] **G12-0607 / INTEGRATION** — Connect same canonical world tick, NPC/faction/trading/quests/loot/persistent changes and optional encounters to visited/unvisited regions, including elapsed events when player is elsewhere.
- [ ] **G12-0608 / INTEGRATION** — Preserve no mandatory first fight, no boss corridor, no predetermined route, no universally scaled foes, causal intervention/death/retreat and freedom of destination.
- [ ] **G12-0609 / VISUAL** — Actually traverse citadel/settlement/field/road/water/cave/transition routes with an authentic owned character and verify camera, interaction, source-authentic art, density and nonrepetition against Gate02 reference.
- [ ] **G12-0610 / DEVICE** — Profile streaming, lighting, shader compilation, frame rate, memory, collision and world persistence on real PC and target Android before switching player-facing map from preserved `StartingWorld.umap`.

**Exit:** A professional, navigable, populated and persistent First World that looks like approved art and responds to player actions; the existing R1 test scene alone does not qualify.

### 07 — Remaining Gate-owned mechanics and authored content, all required (P0/P1)

- [ ] **G12-0701 / Gate03 / CONTENT** — Import actually authored original World/character/scene score, Japanese-direction songs, stems/masters, dynamic layers and transition cues; connect combat/faction/world state, avoid silent framework-only audio.
- [ ] **G12-0702 / Gate04 / INTEGRATION** — Finish both existing combat executors and shared rules for damage, Rank/scale, skill trigger timings, progression, speed/action-value, World Manifestation, character power and learned effects without new unusable passive conditions.
- [ ] **G12-0703 / Gate04 / CONTENT** — Add accepted higher-order progression, World Fantasm visuals, ability-specific animations and context-sensitive counterplay; no weakening of approved epithets or synthesized numerical kits.
- [ ] **G12-0704 / Gate05 / CONTENT** — Install accepted branching opening/world quests, dialogue, Codex, Chronicle, lore knowledge and protagonist ontology; preserve protected spoilers and all alternative player routes.
- [ ] **G12-0705 / Gate05 / INTEGRATION** — Implement persistent quest/event conditions and state graphs linked to time, discovery, NPC survival, relationships, missed events and changed sovereignty.
- [ ] **G12-0706 / Gate06 / CONTENT** — Populate the accepted actual first-region faction/civilization ecosystem with native NPCs, distinct material culture, leaders, careers, relations, motives and location-specific inhabitants. The opening ecosystem deck remains **proposed** until director approval.
- [ ] **G12-0707 / Gate06 / INTEGRATION** — Give NPCs stable saved identities, memory/knowledge, behavior/schedules/relationships, promotions, deaths, resource/faction consequences and settlements not reduced to background décor.
- [ ] **G12-0708 / Gate07 / INTEGRATION** — Fully integrate **all 18 approved** gacha/economy design decisions into EXISTING wallet/inventory/gacha/WorldStore architecture; retain prior actual pity, featured guarantees, true physical item uniqueness and source provenance.
- [ ] **G12-0709 / Gate07 / CONTENT** — Install authorized R/SR/SSR/UR/LR/MR/Absolute rarity, eligible source-accepted pools/banners, source-authored schedules/prices/currency/tickets and correctly settled x1/x10 (last R upgraded to genuine SR when the accepted ten-pull rule applies). No fake rewards, fake banner or fabricated currency IDs.
- [ ] **G12-0710 / Gate07 / INTEGRATION** — Implement one legitimate offline-earned pull currency, ticket-first/request-id idempotent atomic debit/grant/receipt, real pity/featured state and earned farming through approved gameplay.
- [ ] **G12-0711 / Gate07 / INTEGRATION** — Finish original items/materials, anatomy-specific real equipment slots, gear damage/durability/repairs, crafting/invention, consumable/effect definitions, rarity and provenance; inventory has real mass/volume/container/shape limits and unique items stay unique.
- [ ] **G12-0712 / Gate07 / INTEGRATION** — Connect faction-specific production/trade/vendors, logistics, supply-demand, provenance-based loot/drop tables and causal economy consequences without generic universal sellers/prices.
- [ ] **G12-0713 / Gate08 / CONTENT** — Implement approved seed-compatible encounter ecology, authored bosses and persistent dungeons, hazards, random/conditional events and regional enemy deployment. Opening encounter alternatives remain proposals until approved.
- [ ] **G12-0714 / Gate08 / INTEGRATION** — Run encounters against real World state and current party, including optional entry, retreat, diplomacy, stealth, loss/loot, time passage, intervention, cleared/reclaimed dungeon state and dynamic enemy/faction changes.
- [ ] **G12-0715 / Gate09 / CONTENT** — Integrate authored effects, weapons/impacts, injury and mature gore, actual voice/expressions, responsive music/SFX, environmental audio, choreography, multi-character and cinematic time-cued effects as real game events.
- [ ] **G12-0716 / Gate10 / INTEGRATION** — Preserve the approved native visual grammar and genuine gameplay bindings for World HUD, Opening, Home, Characters/details, Gacha, Territory, Records, rarity and application icon; finish the remaining approved screen work without redesign.
- [ ] **G12-0717 / Registry / INTEGRATION** — Connect all eligible accepted source packages, hashes and unique identity/Version/motion/wardrobe records through Gate00's additive Registry v2 intake. Retain original 31 historical Registry records and true ownership; no Gate11 manual queue.
- [ ] **G12-0718 / Gate12 / INTEGRATION** — Complete save/resume, time/World persistence, sovereignty/strategy, domains, quests, encounters, equipment, character state, recovery, offline package hosting and source hash integrity in actual use, leveraging existing code.
- [ ] **G12-0719 / Director ownership** — Treat approved proposals as frozen only when actually approved. Future unapproved NPC ecosystems, encounter decks, new characters, Versions and exact missing kit payloads are **content dependencies**, not invitations for Gate12 to invent canon.

**Exit:** Real authored data is installed and exercised through actual actions on persistent game entities; no gate stays an indefinitely independent design-only folder.

### 08 — All Gate10 visual/interface requirements, user-visible completion (P1)

- [ ] **G12-0801 / VISUAL** — Compare native **Opening** against selected Concept04 direction, proper portrait composition, native buttons and existing safe-load/recovery flow; approved cinematic instead of static placeholder.
- [ ] **G12-0802 / VISUAL** — Make **Ruler Home** full-height visually premium, character/world-led and context-true, with working World entry and actual meaningful state; no duplicated navigation.
- [ ] **G12-0803 / VISUAL** — Make **Characters/Detail** real full-height, exactly three tall owned cards per roster row where applicable, accurate full-body approved 2D or live 3D, wardrobe and Version truth. No chips, fake cards or rejected source image.
- [ ] **G12-0804 / VISUAL** — Preserve **already approved Gacha appearance**; wire true pool display, costs, pity, x1/x10, receipts, owned independent Manifestations and animated results when approved assets exist.
- [ ] **G12-0805 / VISUAL** — Make **Territory** an actual geographic atlas of discovered and governed lands, not a coordinate-only panel; real zoom/pan, claims, structures and reports.
- [ ] **G12-0806 / VISUAL** — Make **Records** genuinely useful Chronicle/knowledge/intelligence/reports with conditional hidden information and persistent progression.
- [ ] **G12-0807 / VISUAL** — Finish **World HUD** targeting, commands, camera, traversals, combat, interaction and contextual state in true landscape with approved visual grammar, not orange diagnostic pawn.
- [ ] **G12-0808 / VISUAL** — Render accepted tier insignia, distinctive fine angular rarity metals R→Absolute, original application icon, typography, 2D artwork/cinematic/animations and all correct symbols/scale.
- [ ] **G12-0809 / DEVICE** — Verify **Ruler and Opening portrait / World landscape**, safe areas, legible text, touch/gesture/camera, orientation change and state recovery on the real target Android device.

**Exit:** All approved screens work and visually match frozen reference quality and actual save data; approved Gacha is not silently redesigned.

### 09 — Complete real end-to-end gameplay, not just separate systems (P0/P1)

- [ ] **G12-0901 / INTEGRATION** — New and existing Ruler safely open legitimate saved/empty World, choose a real character only when owned, move and explore freely, interact with actual entities and return to Ruler/World with state intact.
- [ ] **G12-0902 / INTEGRATION** — Discover/acquire actual items/skills/characters from authorized systems, equip/upgrade, perform quests/encounters and see persistent narrative/faction/market/world consequences.
- [ ] **G12-0903 / INTEGRATION** — Enter a real rendered optional combat, select target/actions, experience correct character-specific animations/Ultimate/Fantasm, exit win/lose/retreat and preserve consequences across save/reload.
- [ ] **G12-0904 / INTEGRATION** — Start/stop real multi-character and mature World interactions with correctly rendered participants and persistent physical/world changes; Privacy only changes visible representation.
- [ ] **G12-0905 / INTEGRATION** — Move between zones, settlements, water/air, inside/outside and quest/encounter/dungeon sessions without losing world time, entity IDs, equipment, ongoing faction changes or player agency.
- [ ] **G12-0906 / VISUAL** — Perform actual PC demonstration of normal play (not a debug fixture): Opening→Ruler→World→authentic traversal→NPC/item/quest→visible optional battle→reward/aftermath→save/cold resume→World continuity. Repeat representative mature and content-specific routes.
- [ ] **G12-0907 / DEVICE** — Repeat core journey on actual Android hardware, including real touch, native animation, audio, rendering, UI, slow hardware response and persistence.

**Exit:** Core-to-endgame functionality and all required authored content form one playable product; no isolated asset or unit-test success substituted for game acceptance.

### 10 — Quality, packaging, release (P2; only when real features exist)

- [ ] **G12-1001** — Perform actual final artistic comparison for every character/Version/wardrobe, First World anchor/region, combat VFX/animation, UI, choreography and cinematic, correcting failures rather than changing specifications.
- [ ] **G12-1002** — Verify Android/Windows builds package ALL runtime-referenced accepted clips, meshes, JSON source authority, fonts, maps, encounters, music, audio, UI and world-source metadata. QA-only assets must not become production entitlement.
- [ ] **G12-1003** — Produce identifiable native Win64 game and correctly signed isolated/current Android APK/AAB as required, with reproducible commit/source hash, build output and proper app icon/orientation; preserve existing installed packages and saves.
- [ ] **G12-1004** — Test install/update/migrate/recover/rollback, WAL snapshot and SQLite schema version, duplicate identities, save integrity, corrupt/missing record recovery, privacy changes and interrupted game launch.
- [ ] **G12-1005** — Test simultaneous actor memory/animation/scene counts (including specified **13 visibly animated participants**), Android thermal/FPS/loading/streaming, collision, heavy World population and combat VFX performance with measured evidence.
- [ ] **G12-1006** — Validate native input, gestures, touch capture/release, device safe areas, landscape/portrait transitions, alternative controls, gameplay camera obstruction, volume and full physical interaction.
- [ ] **G12-1007** — Validate offline operation without server assumptions, content integrity and package inclusion, persistent time/NPC/faction/dungeon behavior, actual encounter causality, repeatability and no forced route or blanket scaling.
- [ ] **G12-1008** — Protect/save user files, frozen Foundation, Registry, approved UI, original Koikatsu/Blender sources, protected Git history and separate production branches; no force-push, accidental replacement, unauthorized migration or destructive workspace cleanup.
- [ ] **G12-1009** — Capture exact native gameplay screenshots/video, reproducible full-journey receipts, build/hash/version metadata, real-art comparisons and outstanding objective limitations; QA reports are evidence only, not product substitution.
- [ ] **G12-1010** — Mark the game finished **only after every commissioned feature, approved source and quality/visual/device gate is actually closed**. No category may be marked DONE from a folder, imported registration, test scaffold or successful code compilation.

## Fastest real progress route (within the required order)

**Immediately:** clear live visual/disk blocker safely → finish remaining unauthorized content-gate audit → finalize central action/rig binding for existing clips → qualify the four genuine Koikatsu native clips on the real Android game → finish in-game motion playback for existing verified categories. **Next:** native visible combat from the real executor using genuine registered characters, target/turn/camera/VFX → authentic Luminous Base/Alter + ownership/wardrobes → fully explorable procedural First World built around existing Gate02 anchors → populate real approved NPC/economy/quest/encounter content → finish frozen UI and full game journey → release validation. Run other independent tasks in parallel **without changing this director-defined dependency/priority order**.

**No silent abandonment:** Any remaining item must carry the exact `UNIMPLEMENTED`, `PARTIAL`, `TECHNICAL_QA`, `VISUAL_QA`, `DEVICE_QA` or `APPROVED_SOURCE_REQUIRED` state with a file/source link and acceptance proof. A blocked source does not stop other tasks.

## Evidence and preservation

- 23 Oct implemented checkpoint: `docs/production/gate12/GATE12_20261023_SHARED_MOTION_CONTENT_AND_QA_REPAIR.md`, commit `7165a21`.
- Native animation acceptance: `Saved/Gate12AnimationAcceptance/GATE12_REAL_NATIVE_QA_SOURCE_PREVIEW_EDITOR_FIXED_20261023.json`.
- Historic actual World/Ruler navigation: `docs/production/gate12/evidence/20261018_fullpage_characters/native_final_23of23.json` (bounded isolated 23/23; no completed real combat/roster/World).
- Current 18-system assessment: `docs/production/gate12/GATE12_USER_REQUIREMENTS_CURRENT_STATE_20261018.md`.
- Binding Gate00 Oct20 directives, with exact original approved sources: `origin/coordination/gate00-live-ops-20261016:docs/production/gate00/GATE00_20261020_ALTER_G07_WORLD_MOTION_APPROVALS.md` and `GATE00_AUTHORITY_HANDOVER_AND_P0_ORDERS_20261020.md`.
- Gate00 Issue #25 is the coordination board. This tracker does **not** overwrite Gate00's canonical Registry, frozen approvals or approval overlay.

## Gate01 accepted five-form 2D artwork integration — 2026-10-30

The exact five original 20 October user-accepted artwork PNGs have been recovered from the original uploaded archive on the connected Android phone and copied byte-for-byte into `Content/NoWorld/Characters/Luminous/Art/Accepted2D/`. The unchanged Gate01 `APPROVED_FIVE_FORMS_MANIFEST.json` carries each exact accepted form, original SHA-256, pixel dimensions, and artwork approval. The five original art identities are Base, Alter, Awakened, Berserker, and Grand Convergence. This is one canonical Luminous character, not five unrelated characters. The extra qualifier previously applied to Grand is not part of the game design.

The existing `UOGGate12CharacterVisualCatalog::FindExact` preserves actual 3D/world-playable/wardrobe entries, chooses the exact approved original 2D image for each existing Version ID and caches imported runtime textures. Original PNG files are staged as offline UFS, rather than requiring an editor-only texture import. `Scripts/production/verify_gate01_luminous_art.py` validates all five original SHA-256 values and version/path mappings.

This **completes accepted-source recovery and source-level 2D binding**, not the full Characters/Detail requirement G12-0803. Verify native Android build, packaged PNG lookup and physical device appearance, and complete the real 3D, kits, combat and World integration before changing any related master checkbox. The existing shared animation bank remains bound once to character identity; never branch animations by Version.