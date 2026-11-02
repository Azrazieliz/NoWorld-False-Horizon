# NoWorld: False Horizon — Unreal game source

**Current state:** full-scope existing UE5.8 offline Android/Windows game under Gate12 integration. **Not yet a complete user-accepted Android build.** The original early architecture/vertical-slice README was superseded: this is no longer named Offline-Game or scoped to a disposable prototype.

## Start here (read order)
1. **[7★ complete game authority](docs/production/NOWORLD_7STAR_AUTHORITY.md)** — 175-page v0.40 whole-game master (ten preserved original-text segments) + [v0.41 mature-only addendum](docs/MATURE_CONTENT_V041_PRODUCTION_FREEZE.md) + later explicitly approved changes. Source code generally reflects later work, but bugs and unfinished stubs do not create new creative rules.
2. **[6★ actual integrated progress](docs/production/ADVANCEMENT_PROGRESS.json)** — all original **36** whole-game milestones, **111** historical tasks, concrete source changes, acceptance status and blockers. READ BEFORE WORK to avoid repetition; UPDATE AFTER EVERY material source/gameplay change.
3. **[5★ reusable content factory and instructions](docs/production/CONTENT_BINDING_5STAR.md)** — original Koikatsu cards, Blender, UE native geometry/animation/art import, common source motion library, published assets and reusable automation. Use the existing program first; if unavailable, document the gap and create/index a new reusable 5★ program.
4. **[Live repository navigation and protected PC source map](docs/production/REPO_EXECUTION_MAP.md)** — directory ownership, original PC location, restored animation catalog, known drift and safe organization.
5. **[Mandatory continuation instructions](AGENTS.md)** and [original 36 whole-game milestones](docs/roadmap/milestones.json).

## Current source branches
- **Active GitHub PR:** [Gate12 #89](https://github.com/Azrazieliz/NoWorld-False-Horizon/pull/89), branch `work/gate12-whole-character-shared-motion-20261031`. This is a large draft stacked on earlier Gate12 PRs; do not mistake `main` for completed production or merge/delete old work to simplify navigation.
- **Production integration baseline:** `production/gate12-full-continuation-20261023`.
- **Authorized native PC:** `D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031`. Preserve the original user project, saves, editor changes, keys, source assets and multiple native worktrees. Do not blindly pull a newer GitHub branch into its older checkout.

## Product invariants
- Unreal is the real game runtime: premium portrait Ruler Mode and embodied landscape World Mode operate on the **same canonical persistent world**, offline-first SQLite WorldStore and authentic independent owned Character Manifestations.
- Full physically coherent World, original 3D/2D character assets, shared authentic global animation source, player-controlled World and enemy AI, canonical turn/action combat, progression, gacha, equipment, economy, territory/sovereignty, systemic interactions, narrative, audio and full mobile UX.
- Original approved assets and current source decisions are preserved. No stand-in mannequins, invented character ownership, fake combat damage, per-Version animation libraries, architecture restarts or replacing mature-content systemic interaction with a gallery.
- Gameplay progress means **real integrated player-visible system advancement against 7★**, not test count, code quantity, commits, imports or documentary work. A playable Android APK needs the complete real game source plus native cook/build and owner phone acceptance.

## Animation-catalog recovery
The authoritative `Content/NoWorld/Animation/SharedMotionCatalog.json` was recovered intact from the protected PC on 2026-11-01 ([commit ce190e87](https://github.com/Azrazieliz/NoWorld-False-Horizon/commit/ce190e87b7c9e5d9e7291c253fc975ca95631ab5)). It contains **3,222 indexed original animation assets** already present both in the PC Content tree and tracked GitHub asset paths. **937 combat entries** have existing native source assets but no authored contact markers; their old QA approval flags have been removed from the live catalog (commit `c18b43a6`). Nothing is owner-rejected or owner-accepted before the owner's phone test. Do not mistake the earlier accidentally empty GitHub JSON for absent source content.
