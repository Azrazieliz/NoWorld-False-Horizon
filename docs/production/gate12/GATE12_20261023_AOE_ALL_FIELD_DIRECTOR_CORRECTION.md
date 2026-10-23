# Gate12 AoE directive — 23 October 2026

## Director's newest combat targeting rule

An attack/ability whose **approved gameplay target scope is area-of-effect** applies to **every eligible character currently active in the battle field**, with ally/enemy allegiance determined by the skill. A caller may set a primary/center target where the existing skill has a primary-specific bonus, but may NOT manually omit otherwise eligible affected actors. **No arbitrary three- or four-target cap.** Reservists, removed and defeated actors are not currently fighting in the field. Single-target attacks, self-only abilities, revival preparations and Covenant preparations retain their approved individual legality rules.

This **2026-10-23 instruction supersedes previous source-specific three/four-target ceilings**, without rewriting other approved kit mathematics, form identity, ownership, cooldown, cost, revival or aftermath.

## Changes

- Source: `Source/OfflineGame/Private/Combat/OGTurnBattleAreaTargets.cpp` and declaration in `Public/Combat/OGTurnBattle.h`. `ExpandApprovedFieldTargets` recognizes existing approved Luminous area types: Base A2 all active enemies, Base A3 ordinary benediction all active allies, Base Night Ultimate all active enemies, Base Day Ultimate ordinary all active allies, Alter A2 all active enemies, Alter Ruin Ultimate all active enemies. Do not generalize unnamed/unknown future skill types without their accepted target scopes.
- The **authoritative** `FOGTurnBattle::ApplyResolvedAction` auto-expands before the already-existing snapshot/rollback, preflight, timeline, and ability execution. UI selection cannot reduce an AoE. Chosen primary stays first only when eligible for the skill.
- Preflight/approved Base and Alter executors now use current field size instead of arbitrary ceiling 3 or 4, while retaining target-allegiance requirements and per-target effects. Unaltered special cases: Covenant, Day revival preparation, Alter A3, Harvest single target.
- World HUD now picks a primary target rather than a limited list of four. Native World action bridge requests identical expansion *before* HP snapshots/reaction visual requests. The executor expands again authoritatively so a caller bypassing HUD receives the same all-field behavior.
- Preserve original authored content JSON, approved artistic files, motion catalog and user-owned `Config/DefaultEditor.ini`. This corrects gameplay policy; it is NOT approval of any QA-gated animation.
- Focused non-destructive automation test `OfflineGame.Combat.Luminous.AoEAlwaysAllFieldParticipants` builds an actual native battle formation with five active enemies plus a reserve, three active allies, and verifies generated targets and retained primary across Base/Alter approved area types.

## Acceptance boundary

**Actual verified:** UE5.8.3 OfflineGameEditor Win64 Development build `Result: Succeeded`, exit code 0. The focused native Unreal automation test was executed after correcting the **test fixture**: an earlier supposed reserve had been alone in its own lane and was correctly promoted to active by the frozen succession rules. The corrected reserve follows a living active member of the same lane. The corrected test passed:
`Saved/Logs/Gate12AllFieldAoENativeTest_CORRECTED_20261023.log` — `Test Completed. Result={Success} Name={AoEAlwaysAllFieldParticipants}` at 18:47:56 Paris time. Original failed test log retained unmodified.

**Graphical review status:** The real UE renderer was launched using `/Game/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_StreamingRuntime -game` on the user-unlocked Windows computer. The log `Saved/Logs/Gate12AoEUnlockedLive20261023.log` proves genuine map `Load map complete`, original Gate02 513-grid terrain, 14,753-cell water source/14 render sections, 10,454 authored placements/599 tiles and native game services initialized without diagnostic fixture. However Windows automatically re-locked while heavy assets were loading; resulting screen captures show the Windows lock screen instead of gameplay, and no real combat actor could be visually accepted. The actual 8 GB workstation also logs static-mesh cold compilation memory pressure. The game process launched during this inspection was closed without touching existing user applications.

**Scope limits:** All-field targeting correction currently covers precisely the six already implemented/approved Base/Alter area categories. Other character abilities require their own approved target semantics before game-wide registration. The 3,201 QA-gated motions remain unapproved for gameplay; no Combat.* approved native clip was fabricated. No new enemy, canonical Registry entry, source art or entire 100×100 km World was generated. AoE **mechanical** targeting is verified, but damage/VFX/presentation end-to-end, character/first-World completion and Android hardware approval remain open.
