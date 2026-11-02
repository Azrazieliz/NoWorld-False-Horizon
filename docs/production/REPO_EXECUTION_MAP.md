# NoWorld — non-destructive repository execution map
**Updated 2026-11-01. Importance: 4★ operational map, never a replacement for 7★ canon.**
**Latest source correction:** GitHub is the durable home; the PC is a temporary native Unreal/Koikatsu/Blender worker. Live animation catalog has been normalized without any pre-phone QA approval boolean; all 3,222 original native references are preserved. Historical approval counts later in this map refer to an older snapshot, not the active eligibility or owner review state. Only an explicit owner response after a phone test can be a rejection verdict.
**Active integration:** `work/gate12-whole-character-shared-motion-20261031` (PR #89).
**Production baseline:** `production/gate12-full-continuation-20261023`.
**User runs ChatGPT on Android only.** Native PC is an authorized build/source worker, not a disposable clone.

## Mandatory first reads and write discipline
1. **7★** `docs/production/NOWORLD_7STAR_AUTHORITY.md`, complete v0.40 archive `docs/production/authority/v040_source_text/part_01.md` through `part_10.md`, mature-only v0.41 addendum `docs/MATURE_CONTENT_V041_PRODUCTION_FREEZE.md`, and later explicit approved changes; inspect original detailed source only for the targeted system.
2. **6★** `docs/production/ADVANCEMENT_PROGRESS.json`: read BEFORE EVERY source operation to avoid repeating work; update after EVERY meaningful source/content system change, giving exact Git SHA, 7★ milestone IDs, concrete implemented behavior and remaining observable gameplay. Never grant a milestone merely because source exists.
3. **5★** `docs/production/CONTENT_BINDING_5STAR.md`: use the existing indexed Python/Unreal/Koikatsu/source-to-action programs BEFORE designing new automation. If the task cannot be done by them, document the actual missing capability, implement a *reusable* 5★ program and add that program to the 5★ index in the same change.
4. Run `python Scripts/production/record_noworld_progress.py --resume` at the next session in a current checkout. Use `--commit --system --result --next --reuse-5star` or `--new-5star-program --why-new` after actual source commits. It will refuse a missing Git commit and never change owner acceptance.

## Frozen source folder map: move nothing that can break identity or package paths
| Domain | Canonical location | Rule |
|---|---|---|
| Design authority | `docs/production/NOWORLD_7STAR_AUTHORITY.md`, `docs/production/authority/`, `docs/MATURE_CONTENT_V041_PRODUCTION_FREEZE.md` | v0.40 whole game; v0.41 mature-only; later explicit accepted changes |
| Whole-game progress | `docs/production/ADVANCEMENT_PROGRESS.json`, `docs/roadmap/milestones.json` | 36 milestone IDs; original 111 tasks; do not re-audit from scratch |
| 5★ production automation | `Scripts/production/`, `Scripts/Gate12Koikatsu/`, `Tools/Koikatsu/` and `ContentFactory/` | Reuse rather than copy; programmatically generate native assets and secure receipts |
| Canonical gameplay | `Source/OfflineGame/{Public,Private}/` | Retain existing C++ class names/includes, UE reflection, sources of authoritative world/save/battle |
| Native game assets | `Content/NoWorld/` | Preserve stable `/Game/NoWorld/...` object paths and references; do NOT bulk-move UE .uasset files outside Unreal |
| Shared source motions | `Content/NoWorld/Animation/SharedMotionCatalog.json` and runtime `Source/OfflineGame/*/Animation/` | ONE canonical bank, globally reusable anatomy-compatible derivatives |
| Real characters | `Content/NoWorld/Characters/Luminous/` and authenticated original cards `Assets/NoWorld/` | Original source, five form kits, immutable exact-card provenance, Git LFS |
| Persistence and business systems | `Source/OfflineGame/Private/Persistence/`, `Gacha/`, `Combat/`, `World/` | Keep one SQLite WorldStore and the existing authoritative gameplay systems |
| Systemic interactions | `Source/OfflineGame/*/Interaction/`, `UI/OGNativeInteractionWidget*` | v0.41 foundation is already source-implemented; complete missing links without replacing it |
| Native UI | `Source/OfflineGame/Private/UI/` and `Source/OfflineGame/Public/UI/` | Existing premium Ruler and landscape HUD implementation, not independent mock UI |
| Factory output recipes | `ContentFactory/Jobs/` (input), `Content/NoWorld/ContentFactory/` (approved output) | Never copy private raw source assets to public repo; generated .uasset via LFS |
| Original studio assets | `Assets/`, PC `SourceArt/`, `SourceAssets/`, other protected PC staging | Original protected sources retained; separate from cooked assets |
| Source evidence | `docs/production/gate12/evidence/` | Supporting receipts, never device acceptance |
| Windows packaging | `Scripts/android_unreal_package.ps1`, `Build/`, `Config/`, `.github/workflows/` | Build only when full requested integration is ready; preserve keystores/saves |

## Current environment and recovered animation source
- Connected Windows device: `LAPTOP-1LI4VRCJ`. Protected native project: `D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031`, local checkout at `d4e68de1` during the 2026-11-01 inventory. **Do not git pull/checkout/reset in that working project** without checking preservation rules. The active GitHub branch is much newer.
- Local original animation catalog: **3,222** source entries / **1,077** distinct source clip IDs / **1,075** semantic action IDs. Exactly 918 source entries already flagged gameplay-approved (902 AdultInteraction, 12 Traversal, 4 Interaction); 937 combat entries flagged unapproved and none had a contact timestamp or contact notify. Source references were checked against actual files: 3,222 native Unreal assets present on PC; all 3,222 corresponding paths tracked in the restored GitHub tree.
- Source catalog restored intact via [GitHub commit ce190e87](https://github.com/Azrazieliz/NoWorld-False-Horizon/commit/ce190e87b7c9e5d9e7291c253fc975ca95631ab5) from protected PC original, after an earlier 68-byte GitHub replacement. Never initialize an empty catalog over indexed source again. No native playback approval or contact was invented.
- Native project tracked working tree was unchanged; **six untracked Grand/source patch .py files** plus **three .pyc caches** were found. These are PC-only one-off migration/repair scripts and might be non-idempotent. Leave them intact. They must not be confused with a reusable clean 5★ factory. Full discovery of legacy external staging drives has not been performed.
- Existing old Gate09 source and separate Grand import staging at `D:\UnrealProjects\Gate09_Protected_Production_20261016` and `D:\UnrealProjects\G12_LuminousNativeImport_20261031` are protected original caches, not automatic publish folders.

## Verified narrow static source checks (not Android acceptance)
- `OGRulerGachaAccessService` uses canonical territory control and a strictly longer-than-one-in-game-month TimeDomain qualification.
- `OGTurnBattleAreaTargets` explicitly expands eligible active field targets for authored AoE instead of using a fixed six-target cut.
- `OGSystemicInteractionRuntime` provides canonical Ruler role, variable participants and staged source action graphs; `OGFoundationInteractionHost` connects to actual WorldStore and existing native UI. It also contains a **diagnostic fallback graph path during combat** when no source definition is selected. That is a source drift/risk against 'no stand-ins' and must not be advertised as finished native content.
- Combat motion catalog has real assets but no approved combat/contact mapping. Retain existing authentic animation source; complete authored timing, permissions and runtime links without a new bank.
- v0.41 addendum explicitly states it does not reopen eligibility. Creative Director separately says the old 'single restriction' was removed. That **exact conflict is open** and must not be silently resolved by inventing a policy.
- Source-only content factory is not yet compiled/executed. GitHub workflows need an authorized running native Windows worker; no claim of a fully automatic installed pipeline.

## Safe organization rules
- This map is an **index**, not a destructive move. Broad renames of Unreal UAssets, original cards, C++ reflected classes, sqlite schemas, world anchors or production Windows directories can sever live references.
- Prefer file-aware indices, known program entry points and source-compatible wrappers to moving legacy roots. All new factory scripts go under the indexed production root and are registered at 5★.
- Review original existing source and 6★ status before changing any system; reuse original AI, World, character, interaction, gacha, save and UI systems.
- For every reported result clearly classify: `PLAYER_VISIBLE_ACCEPTED`, `SOURCE_LINKED_UNVERIFIED`, `CONTENT_PENDING`, `DESIGN_CONFLICT`, `NATIVE_BUILD_PENDING`.
- No new tests, build or phone-install claim until the user-requested full-game implementation is complete.
