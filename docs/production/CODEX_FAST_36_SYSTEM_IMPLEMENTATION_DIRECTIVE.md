# NoWorld — Codex accelerated 36-system implementation directive
**Effective:** 2026-11-05. **Priority:** current owner's explicit execution instruction, superseding older assistant-defined verification gates. **Scope:** complete the existing game; this is not a redesign, a reset or a new feature request.

## The non-negotiable working rule
**Implement first. No gameplay verification until the owner installs the completed Android game.** Codex must deliver source implementation across **the existing M01–M36 scope** at maximum practical throughput, preserving approved functionality and original assets. When real implementation of an individual system is finished, mark it **`IMPLEMENTED_FROZEN_AWAITING_PHONE_TEST`** and **move to the next unfinished system**. Do not revisit a frozen system for audits, tests, speculative repairs, optimization, cosmetic refinements, repeated QA, or approval questions. Reopen it **only if a real integration blocker prevents another required system or APK packaging, or the owner reports a defect after the phone test**. An actual missing implementation is not a completed system; do not close empty scaffolds, partial links, imports alone or placeholders.

This is a *workflow state*, not a claim that the owner has tested/accepted the system. Preserve the existing 6★ `owner_accepted:false`, `device_verified:false` and historic milestone acceptance fields until the owner actually supplies a verdict.

## Work now; do not create new prerequisites
1. Read once per workstream: `AGENTS.md`, the 7★ authority index and relevant exact v0.40/v0.41 clauses, current 6★ ledger, 5★ production tools and execution map. **Only inspect touched implementation areas thereafter.** Use existing master/issue acceptance as specification. No prolonged whole-repository audit or planning loop.
2. **Write actual Unreal Engine 5.8 C++ gameplay integration, Blueprints/native assets, UI, data bindings and existing source-factory products.** Preserve identifiers, WorldStore, original character/market/ecology sources, complete original motion bank (3,222 indexed entries, 937 combat), approved game rules and native architecture. Reuse systems, no duplicates, stand-ins or fabricated action contacts. Adapt source-faithful reusable functions rather than manually reproducing each variant.
3. **Execute independent systems in parallel** via separate worktrees/branches where practical (World, combat/characters, gameplay/economy/progression, NPC/quests, UI/persistence, assets/audio/platform). Avoid having two agents edit the same file. Integrate changes frequently into the established PR #89 branch, resolving dependencies directly. The existing native PC worker is an execution resource, not the source of truth.
4. **No tests or verification loop:** do not run unit, automation, editor gameplay, visual/manual checks, recurring native compile checks, benchmarking, static QA campaigns, source audits, performance validation or acceptance gates between implementations. Do not block on CI. A source implementation commit plus its exact integration points is sufficient to move forward provisionally.
5. After **each material implemented slice**, commit actual code/assets to GitHub, and update the **6★ ledger** with affected M-ID(s), exact commit, the implemented runtime entry point, the remaining missing functionality, and `IMPLEMENTED_FROZEN_AWAITING_PHONE_TEST` **only when implementation is genuinely complete**. Batch related updates; do not spend an entire execution pass editing status instead of shipping code. Keep incomplete work `IN_PROGRESS` or `BLOCKED_BY_MISSING_SOURCE`, with a concrete blocker. Never state owner-tested/accepted when not.
6. **Do not repeatedly build or package.** Once all implementable M01–M35 source systems are complete as scoped, perform **one current Android ASTC ARM64 BuildCookRun** from the combined implementation and fix only real blockers that prevent generating/installing the APK (compiler/UHT/linker/cook/package errors, missing native files, signature or manifest faults). Packaging is necessary production output, **not gameplay validation**. Deliver the actual fresh APK and source-build identification for the owner to test. Preserve any preexisting independent Android build already underway but do not treat its older code snapshot as the final all-systems APK.
7. **M36 is the owner's full-game phone acceptance milestone**, not a Codex-authored gameplay feature. Leave its owner verdict open until the user tests the APK. Do not use M36 to block APK construction; its implementation work is the integrated game from M01–M35.
8. After owner phone feedback, reopen **only** systems exhibiting a reported defect or necessary cross-system fault, repair those targeted issues and provide an updated APK. No broad rework absent evidence or explicit instruction.

## Coverage — all 36 canonical milestones (do not omit later systems)
| ID | Existing system | ID | Existing system |
|---|---|---|---|
| M01 | Offline mobile startup and free World entry | M19 | Crafting, invention, enhancement, durability and repairs |
| M02 | Playable movement, camera and touch traversal | M20 | Settlement trade, markets and regional economics |
| M03 | Real World battle and autonomous opponent turns | M21 | Character growth, affinity, proficiency and statistics |
| M04 | Five-form original Luminous rigs/transformations | M22 | Roster, character details and wardrobe UI |
| M05 | Shared authentic animation/expression runtime | M23 | Home, gacha, records and territory UI |
| M06 | Approved five-form v4 combat kits | M24 | NPC life, relationships and factions |
| M07 | Weapons, projectiles, physical hits and VFX | M25 | Quests, story events and consequences |
| M08 | Identity, Versions and Manifestations | M26 | Chronicle, Codex and narrative memory |
| M09 | Native in-World interface and combat HUD | M27 | Bosses, optional encounters and dungeons |
| M10 | Offline profile and save/load | M28 | Territories, Ruler powers and civilization |
| M11 | First World terrain, streaming and navigation | M29 | Source-authentic multi-participant interactions |
| M12 | World geography, settlements and interiors | M30 | Adaptive music and original audio/SFX |
| M13 | Ecology, resources, water and environment | M31 | Character/environmental visual polish |
| M14 | Enemy AI, high-rank combat and balance | M32 | Persistent events, migration and recovery |
| M15 | GP, domains and high-rank progression | M33 | Mobile settings, touch and accessibility |
| M16 | Rarity banners, gacha and pity | M34 | Android packaging and device-ready delivery |
| M17 | Inventory, anatomical storage and gear | M35 | Windows standalone parity and packaging |
| M18 | Loot, rewards, consumables and currency | M36 | **Owner phone acceptance — reserved for user** |

The authoritative detailed requirements are still in the 7★ source/issue contracts and must be implemented, not reduced to the brief labels above.

## Fast execution priority, not a permission to stop early
**First remove critical gameplay dependency gaps**: World control/terrain/streaming/original vegetation and all 12 market sections (M01/M02/M11/M12/M13), shared original characters/motion/combat/contact (M03–M08/M14), real UI/save/persistence (M09/M10/M32/M33). In parallel implement every remaining gameplay, economy, social, narrative, media and platform system (M15–M31/M34/M35). Do not declare the project complete after only the first three groups.

**Meaning of done:** exact required implementation exists and is connected to its real runtime; pending owner review is NOT implementation unfinished. **Meaning of not done:** required gameplay behavior/source/binding is still absent, simulated by a fixture, or represented only by a spec/report/test.

**User instruction precedence:** this rapid implementation-and-phone-first policy supersedes older requirements to run automated tests, gain native gameplay acceptance, or seek repeated approval before advancing. It does **not** authorize fabricating finished systems, source assets, combat timing, phone-test results or an APK.
