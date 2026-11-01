# NoWorld: False Horizon — 7★ Canonical Game Decisions & Integrated Systems
**Authority grade: 7/7 — sole entry point for user decisions and complete-game system coverage.**
**Updated: 2026-11-01. THIS IS CONTINUATION, NOT A REDESIGN.**

This file is the SINGLE 7★ **index**. It does not supplant or paraphrase away detailed original approved source contracts, artwork, numeric kit payloads, Registry definitions, world anchors or the actual 36 live milestone acceptance requirements. Where a complete original contract is more detailed, read the original. A later explicit user decision takes precedence over earlier project text; do not invent missing decisions.

## Non-negotiable game purpose and decisions
- Finish the existing offline, full-scope, real Unreal Engine 5.8 game, **NoWorld: False Horizon**, playable on the owner's Android phone. Windows release separately. No standalone mockups, prototype executors, fake stand-in game, truncated scope or arbitrary simplification.
- Retain every original accepted visual, character, source 3D card, wardrobe, rig, audio, design source, approved nine native UI reference scopes, gameplay mechanic, Registry, Manifestation, narrative decision, economy rule, persistence contract, mature content scope, and original First World anchor.
- **Luminous has five visual/form kits, not five ordinary derivations:** Base, Alter, Awakened, Berserker, Grand Convergence; Grand is exceptional. Preserve approved v4 kit counts: Base 3/6/2, Alter 3/6/2, Awakened 4/5/1, Berserker 2/7/1, Grand 3/6/3. The actual original numeric sources govern; older Base v6 / Alter v1 summaries in historic engineering trackers are NOT authority for replacing v4. ALL AoE hits every applicable field participant.
- **One shared semantic action / original animation source bank** covering characters, rigs and activities. Identity binds to the character once; Versions, wardrobes, and owned Manifestation instances do not get duplicate animation libraries. Attach authentic exact-Skeleton derivatives of the real source. Use the existing shared resolver and source catalog. Never silently approve source clips or invent contact markers; author exact content when necessary. Shared action controller must not override authoritative combat.
- **Real World:** coherent 100 × 100 km First World, accepted Region R1, World Partition/PCG/landscape, terrain/geography/settlements/biomes and causal persistent ecology. Free first entry; no forced first fight or forced tutorial, and preserve nonhuman routes/identities.
- **Real combat:** 2 existing authorized executors (turn and World), actual enemy actors and AI, true action/animation contact and consequences, exact five v4 kits, revive/status/resources, physical hit/projectile/VFX, World persistence and win/loss. Never grant or simulate an owned character or invent a damage packet.
- **Real in-game UI:** Gate10 discarded. Gate12 owns original premium native Unreal UMG/World UI for home, World, combat, roster, inventory, equipment, gacha, settings, narrative and other integrated systems. Preserve accepted art direction rather than reintroducing discarded visual Gate.
- **Ownership/economy:** Registry identities, real independent owned Manifestations with duplicates, 18 approved Gate07 D01-D18 systems, atomic local pulls, pity, genuine earned currencies/rewards, inventory, equipment, crafting, trade, domain/territory and persistable world decisions. No character chips, fake IDs or bogus ownership.
- **Device and delivery:** GitHub-first source branches/commits/PRs. PC only for genuine Unreal/Blender/Koikatsu/Gaea native authoring, import, package or install. The user operates only a phone. Do not perform repeated tests/compilation now; complete requested native features and intersystem wiring first, then compile/package once as a finished integration. Only the owner accepts real phone gameplay.
- No private/licensed raw content, credentials or keystores in the public repository. Preserve saves, approved assets, and all prior sources. Do not reset/clean/delete the native Windows workspace.

## Priority grade and continuation protocol
| Grade | Sole responsibility | Entry |
|---|---|---|
| **7★** | Game purpose, existing user-approved decisions, all integrated systems and acceptance. This file is the index; detailed original approvals referenced here remain authoritative. | `docs/production/NOWORLD_7STAR_AUTHORITY.md` |
| **6★** | Exact completed/not completed integrated milestones, observable achievement, GitHub commits, true remaining gameplay and content, release/device/user status. Never count raw technical rituals as game completion. | `docs/production/ADVANCEMENT_PROGRESS.json` and the live `docs/roadmap/milestones.json` issues |
| **5★** | Critical evergreen production APIs, source-to-character/action/animation/Unreal binding, content ingestion recipes. Reuse every conversation; do not rebuild or fork it. | `docs/production/CONTENT_BINDING_5STAR.md` and `Scripts/production/bind_native_motion.py` |

**Required first read on every conversation:** This 7★ index, then the 6★ advancement ledger, then the relevant 5★ binding instructions and exact source code. Read original approved numeric/art/character contracts for the specific task, not all historical audits. Submit real changes, push GitHub, and update 6★ with an evidence-linked user-visible *system* delta; keep independent content blockages separately identified.

**Progress rule:** User-noticeable finished integrated gameplay/system outcomes count. Code-only, new scripts, tests, C++ fixes, exporter receipts, imported pieces and compile/package management alone do **not** count a completed game subsystem. Mark them enabling source work, without percentage inflation. No completed milestone until full live integration evidence and owner phone acceptance when applicable.

## Binding authoritative references
- [27 Oct playable-first and Gate12 operational contract](PLAYABLE_FIRST_GATE_RESTART_20261027.md) — the current explicitly overridden Gate semantics and exact original requirements.
- [36 live whole-system acceptance definitions](../roadmap/milestones.json); [roadmap](../roadmap/README.md) — the entire game system scope, below.
- [Original Gate12 full advancement master](ADVANCEMENT_MASTER.md) — older engineering task inventory only; superseded wherever it contradicts v4 or latest user instructions.
- `Content/NoWorld/Animation/SharedMotionCatalog.json` — single packaged gameplay animation bank; existing native fallback preserved.
- `Assets/NoWorld/Luminous/KoikatsuOriginalCards/SOURCE_CARDS_MANIFEST.json` — original-card provenance.
- `docs/production/gate12/evidence/` — real source asset receipts; evidence of source presence is not device acceptance.
- Original approved full-resolution art, v4 combat payloads, World anchors, Gate07 D01–D18 contract files and previous user decisions remain *canonical source material*. Do not overwrite them from incomplete milestone summaries.

## Full-game integrated system coverage — original 36 milestone acceptance requirements

### M01 — Offline mobile startup and free World entry

**Original acceptance:** Native Android app launches offline, provides existing opening/Ruler/World flow without compulsory opening combat; real World entry works on the owner's phone.

**Authority:** [Milestone issue #27](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/27); producing Gate(s): 12.

### M02 — Playable character movement, camera and touch traversal

**Original acceptance:** Approved real character is controllable using phone touch, camera, facing, ground movement, jump, swim and flight when available; anatomy-specific motion is applied in the real World.

**Authority:** [Milestone issue #28](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/28); producing Gate(s): 12.

### M03 — Rendered real World battle and autonomous opponent turns

**Original acceptance:** Real player and opponent rigs visibly alternate decisions/actions, change genuine HP/status, react and resolve win/loss within the actual World, not a test runner.

**Authority:** [Milestone issue #30](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/30); producing Gate(s): 12+04+08.

### M04 — Five-form original Luminous rigs, wardrobes and transformations

**Original acceptance:** Base, Alter, Awakened, Berserker and Grand Convergence use source-faithful real playable models, original wardrobes, facial morphs and transformations on the phone; no fifth ordinary lineage.

**Authority:** [Milestone issue #31](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/31); producing Gate(s): 01-3D+12.

### M05 — Shared authentic animation and expression runtime

**Original acceptance:** Native source-derived animations actually drive real gameplay traversal, combat, facial expression, anatomy/wardrobe and state transitions via a shared semantic rig resolver.

**Authority:** [Milestone issue #32](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/32); producing Gate(s): 12+01-3D.

### M06 — Exact approved five-form v4 combat kits

**Original acceptance:** All 45 approved core active/passive skills, nine Ultimate branches and five Fantasms apply actual v4 effects using existing two executors; all-field AoE and true costs, QTE, statuses work.

**Authority:** [Milestone issue #33](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/33); producing Gate(s): 04+12.

### M07 — Weapons, projectiles, physical hits and combat VFX

**Original acceptance:** Native weapon/magic families, collisions, hit contact, reactions, ability cues, Fantasm presentation and combat effects visibly match source-authentic character actions.

**Authority:** [Milestone issue #34](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/34); producing Gate(s): 12+04.

### M08 — Character identity, Versions and individual Manifestations

**Original acceptance:** Canonical Identity, four conventional Version kits plus exceptional Grand, distinct independently owned Manifestation instances, duplicates and roster selection work without character chips.

**Authority:** [Milestone issue #35](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/35); producing Gate(s): 07+12.

### M09 — Premium native in-World interface and combat HUD

**Original acceptance:** Gate12 integrates polished gacha-grade Unreal native UI based on accepted nine-scope references: World/Ruler navigation, action selection, HP/status, targeting, battle results, authentic character visuals.

**Authority:** [Milestone issue #36](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/36); producing Gate(s): 12.

### M10 — Persistent offline profile and local save/load

**Original acceptance:** World, roster, transformations, progression, wallet, possessions and decisions persist across real app exits/restarts using safe canonical SQLite identities and migrations.

**Authority:** [Milestone issue #37](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/37); producing Gate(s): 12+07.

### M11 — First World terrain, deterministic streaming and navigation

**Original acceptance:** The real coherent 100×100 km first World has functioning Landscape/World Partition/PCG navigation and persistent seeded streaming around the accepted source anchors.

**Authority:** [Milestone issue #38](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/38); producing Gate(s): 02+12.

### M12 — World landmarks, geography, settlements and interiors

**Original acceptance:** Citadel, towns, farmlands, roads, water, forest, caves, interiors and authored locations have original painterly-anime PBR quality, navigation/collision and playable interactions.

**Authority:** [Milestone issue #39](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/39); producing Gate(s): 02+12.

### M13 — Ecology, resources, water and environmental simulation

**Original acceptance:** Persistent biome ecology, gatherable resources, water/terrain effects, wildlife, time and relevant environmental variation function as integrated simulation rather than scenery only.

**Authority:** [Milestone issue #40](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/40); producing Gate(s): 02+12+06.

### M14 — Enemy AI, high-rank combat and encounter balancing

**Original acceptance:** Enemies make differentiated world/battle decisions, use authentic capabilities, varied ranks/telegraphs and relevant counterplay; no arbitrary global level scaling.

**Authority:** [Milestone issue #41](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/41); producing Gate(s): 08+04+12.

### M15 — GP, domains and high-rank progression rules

**Original acceptance:** Exact approved GP strength, one strongest eligible Manifestation per identity, realm links, controlled territories, large-number safe rank math and Grand Convergence rules function in combat and save.

**Authority:** [Milestone issue #42](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/42); producing Gate(s): 04+07+12.

### M16 — Rarity banners, weighted gacha, pity and atomic pulling

**Original acceptance:** Actually eligible accepted character banners, rare/featured odds, 10-pull floor, pity/guarantee, tickets, earned currency, atomic idempotent pulls and distinct duplicates work offline.

**Authority:** [Milestone issue #43](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/43); producing Gate(s): 07+12.

### M17 — Inventory, anatomical storage and equipped gear

**Original acceptance:** Stack/provenance inventory, real anatomy-dependent physical capacity, functional equipment/weapon slots, appearance and item ownership operate consistently during exploration and combat.

**Authority:** [Milestone issue #44](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/44); producing Gate(s): 07+12.

### M18 — Loot, rewards, consumables and earned currency

**Original acceptance:** Real World encounters/resources and quests grant legitimate items/currency/rewards; food/consumables have functional consequences and change retained inventory/wallet.

**Authority:** [Milestone issue #45](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/45); producing Gate(s): 07+12+08.

### M19 — Crafting, invention, enhancement, durability and repairs

**Original acceptance:** Approved original recipes/materials/invention, crafting, equipment improvement, wear/durability and repair consume/produce actual persistent items with gameplay effects.

**Authority:** [Milestone issue #46](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/46); producing Gate(s): 07+12.

### M20 — Settlement trade, markets and regional economics

**Original acceptance:** Authored merchant/market inventories, pricing, regional resources and causal settlement trade actually affect currency, NPCs and persistent World supply.

**Authority:** [Milestone issue #47](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/47); producing Gate(s): 07+06+12.

### M21 — Character growth, affinity, proficiency and statistics

**Original acceptance:** Rank/experience, rarity, skills, proficiency, affinity, derived stats and equipment changes affect real gameplay with consistent persistence and no fabricated ownership.

**Authority:** [Milestone issue #48](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/48); producing Gate(s): 07+04+12.

### M22 — Premium roster, character details and wardrobe UI

**Original acceptance:** Native user-quality roster, character Version details, full 2D/3D presentation, wardrobe, progression and equipment screens integrate real data and preserve accepted visual direction.

**Authority:** [Milestone issue #49](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/49); producing Gate(s): 12.

### M23 — Premium home, gacha, records and territory UI

**Original acceptance:** Gate12 builds refined responsive premium native Home, Records, Gacha, Territory, opening/icons/rarities and coherent mobile navigation based on accepted art plus relevant gacha-quality conventions.

**Authority:** [Milestone issue #50](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/50); producing Gate(s): 12.

### M24 — Native NPC life, relationships and faction systems

**Original acceptance:** Physical NPCs with schedules, ecology, dialogue context, relationships, factions, deaths/absence and causal persistent population/world reactions act within the real World.

**Authority:** [Milestone issue #51](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/51); producing Gate(s): 06+12.

### M25 — Branching quests, story events and consequences

**Original acceptance:** Authentic user-approved lore and playable non-forced open-World quests/dialogue branch on choices, race/species/context and update real characters, factions, rewards and World state.

**Authority:** [Milestone issue #52](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/52); producing Gate(s): 05+12.

### M26 — Chronicle, Codex, knowledge and narrative memory

**Original acceptance:** Player knowledge, Chronicle, Codex and time-based missed-events record actual world/quest events without spoilers or simulated general autobiographical memory.

**Authority:** [Milestone issue #53](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/53); producing Gate(s): 05+12.

### M27 — Bosses, optional encounters and dungeon progression

**Original acceptance:** Optional authored world encounters, real high-rank boss phases, persistent dungeons/events and meaningful victory/retreat/missed-event outcomes are playable without forced opening encounters.

**Authority:** [Milestone issue #54](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/54); producing Gate(s): 08+12.

### M28 — Territories, Ruler powers and changing civilization

**Original acceptance:** Ruler mode, influence/domains, land control, governance/factions and persistent territorial consequences visibly change settlements, GP and game actions.

**Authority:** [Milestone issue #55](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/55); producing Gate(s): 07+06+12.

### M29 — Synchronized source-authentic multi-participant interactions

**Original acceptance:** Real compatible participant rigs and semantically approved synchronized source animations execute in-game with reliable role/clock/contact/privacy/state; architecture supports up to 13 roles where genuinely sourced.

**Authority:** [Milestone issue #56](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/56); producing Gate(s): 12+01-3D.

### M30 — Adaptive music, original audio and game-event SFX

**Original acceptance:** Original licensed/owned soundtrack and approved Japanese vocal cues when sourced, adaptive stems, ambience, authentic hit/interface/world sound events and mixes function in actual gameplay.

**Authority:** [Milestone issue #57](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/57); producing Gate(s): 03+12.

### M31 — Character and environmental visual polish

**Original acceptance:** Original accepted painterly-anime PBR art direction, skin/hair/cloth physics, morphological fidelity, light/VFX readability and atmosphere visibly hold in the actual mobile-rendered game.

**Authority:** [Milestone issue #58](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/58); producing Gate(s): 01-3D+02+12.

### M32 — Persistent events, migration and recovery integrity

**Original acceptance:** Offline save migration, consistent atomic transactions, recovery snapshots and safe resume protect real Manifestations, quests, World changes, items and user data after app restart/crash.

**Authority:** [Milestone issue #59](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/59); producing Gate(s): 12+07.

### M33 — Mobile settings, touch ergonomics and accessibility

**Original acceptance:** Functional touch sizing, scalable UI, portrait/landscape where required, graphics/audio options, language/settings, legibility and accessibility behave correctly in the native phone app.

**Authority:** [Milestone issue #60](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/60); producing Gate(s): 12.

### M34 — Android packaging, performance and device-ready delivery

**Original acceptance:** Current installable Android build packages authentic assets, uses viable memory/rendering/streaming/performance, survives lifecycle transitions and is handed to the owner for direct phone gameplay testing.

**Authority:** [Milestone issue #61](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/61); producing Gate(s): 12.

### M35 — Windows standalone parity and reproducible native packaging

**Original acceptance:** Current standalone Windows release builds from production code with the full World, native UI, correct assets and save compatibility; compilation alone cannot close mobile gameplay acceptance.

**Authority:** [Milestone issue #62](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/62); producing Gate(s): 12.

### M36 — Complete integrated full-game phone acceptance

**Original acceptance:** Owner plays current Android release through open World, all forms, combat, quests, NPCs, economy, gacha, interactions, UI, persistence, visuals and audio and explicitly confirms the complete approved playable game.

**Authority:** [Milestone issue #63](https://github.com/Azrazieliz/NoWorld-False-Horizon/issues/63); producing Gate(s): 12+all.
