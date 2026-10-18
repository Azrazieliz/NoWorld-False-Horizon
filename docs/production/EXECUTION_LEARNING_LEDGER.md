# P0 ? Execution, Time Protection and Permanent Learning Ledger

Cross-Gate standing project policy, effective 2026-10-18. READ BEFORE PRODUCTION WORK. The user's private reason for time scarcity is intentionally not recorded in public Git.

## Execution contract

1. **Time protection:** Optimize verified output per minute and minimize the user's required attention. Keep commentary to actual milestones and blockers. Do not request reapproval for routine authorized work.
2. **Preserve source:** At task start inspect latest SHA, dirty worktree, approved references, source of truth, completed code, and known failures. Do not overwrite frozen Foundation, Registry, Koikatsu source cards, saves or accepted UI/art.
3. **Budget diagnosis:** Default to ~5 minutes focused read-only discovery before choosing a known-good method. If ~15 minutes elapse without new evidence or product progress, change method and identify blocker; do not continue trial-and-error. Do not interrupt legitimate in-flight builds. Quality remains the acceptance standard.
4. **Every correction must generalize:** Record symptom, systemic root cause, source/automation method correction, ALL relevant affected features, actual before/after proof and remaining limitations. Apply the change to all relevant workflows. Never rerun an already rejected technique disguised by new filenames.
5. **Actual output proves acceptance:** Compilation, tests, synthetic fixtures, screenshots from the wrong app or legacy commit, and automated clicks do not by themselves prove visual fidelity, character art or Android quality. Inspect genuine authored files and current native screenshots against approved references.
6. **No counterfeit data:** Never invent owned Manifestations, canonical identities, progress, record entries, banners or character artwork to make an empty UI look populated. QA-only objects never appear as genuine player content.
7. **Testing proportional to changes:** Reuse tested foundations, batch related patches, one meaningful incremental build, test relevant regressions, then visually inspect rendered change. Do not waste time on repeated known-passing suites or disposable prototypes.
8. **Frozen scope:** Ruler always portrait, World always landscape. Approved Gacha and Gate10 art, character identity/wardrobe/Privacy, source cards, saves and Foundation remain authoritative.
9. **Crash-safe results:** At real milestone, commit substantial changes and concise checkpoint with SHA, actual artifact, visual/native evidence, current blocker and exact next task. Never claim DONE if user-standard output remains missing.
10. **Gate across contexts:** Future Gates using this repository must read this document and propagate new generalizable corrections. Conversation-specific memory is not a substitute for this source file.

## Observed failures and permanent method guards

| User-visible failure / systemic cause | Reusable corrected method | Actual evidence and acceptance |
| --- | --- | --- |
| Geometric, under-styled UI repeatedly treated as complete because widgets and tests passed | Compare per-screen *native screenshots* against approved Gate10 references; check art hierarchy, typography, actual width, source and orientation before acceptance | Real 360x720 Characters proof in docs/production/gate12/evidence/20261018_fullpage_characters; full UI still OPEN |
| Home duplicated Characters/Gacha, leaked Ruler UUID and equated Ruler to a Manifestation | Home action/navigation single responsibility; keep store IDs private; characters only represent canonically owned Manifestations | Gate12 source commit 16e4e974; native 23-step journey; artistic acceptance remains OPEN |
| White strip, glyph overflow and bright default scroll rail | Inspect source raster transparency and Slate defaults; bound each native cell; use verified Unicode fonts and dark scroll styling | Gate12 commit 5374ac84 with native screenshot, Android proof still OPEN |
| Characters restricted to one-third; 3D QA viewer mislocated | Dedicated full portrait destination; exactly 3 tall owned portrait cards per row; 2D/3D toggle INSIDE actual owned character detail, not global debug menu | Gate12 commit 5374ac84; real-owned character demonstration still OPEN |
| Repeated build loops without product improvement; premature 23/23 claims | Execute closest existing integration, make smallest systemic fix, real render/functional proof, then stop or change failed method. Never count same receipt as different source | Enforced by requiring exact screenshot/receipt path and source SHA at each future production checkpoint |

### 2026-10-18 ? Weapon-family correction

- Rejected failure: one fixed sword icon in World and one generic attack pose implicitly represented all characters/weapons/supernatural kits. Root cause: diagnostic presentation was promoted into a universal content rule.
- Corrected method: neutral action icon and read-only authored equipped-item motion profile, rig-compatible bound animation only; missing/conflicting/broken profiles do not select a guessed firearm pose. Open-ended category names allow natural anatomy, sling, melee, casting and technology.
- Propagation: source combat action presentation and World HUD now share the same assumption-free rule. Dedicated regression OfflineGame.Gate12.Combat.MotionProfiles.NoUniversalRifleOrSword passed 1/1 under UE5.8.3. Actual clip assets require Gate09; equipment metadata contract needs Gate07; targeting/hits require Gate04. Never claim authored animation from a parser or diagnostic.
- UE5.8 compile failure was corrected at the checked-format API, not rerun unchanged. See Gate12 weapon motion dependency handoff.

### 2026-10-18 ? Generalizable weapon/ability presentation correction

- Actual failure: Narrow sword-only interpretation, diagnostic gun/reach fallback, equipment-only resolution that omits innate abilities, and Ultimate using Skill clip family.
- Method: Read explicit authored equipment style or, if absent, explicit canonical entity innate style. Conflicts fail closed. Choose ONLY skeleton-compatible authored clips or no motion. Distinct Basic/Skill/Ultimate clip and semantic SFX keys; never play mismatched audio if its skeleton-compatible action clip is unbound.
- Propagate to fists, sling, bow, technology, magic, natural arms, nonhuman anatomy and multiweapon compositions, plus content contracts Gate04/07/09.
- Evidence: Source/static checks ONLY, user explicitly deferred application builds. No visual or Android acceptance yet. Contract in docs/production/gate12/GATE12_WEAPON_MOTION_DEPENDENCIES_20261018.md.
- Never repeat: Do not synthesize a firearm/sword stance, physical weapon, ownership or special effects when no real authored assets exist.

## Checkpoint record format

Requirement | source/artifact actually changed | real native visual/function PASS/FAIL/UNVERIFIED | evidence path+SHA | remaining blocker / next executable action | time spent.

Only add a new record when a real failure and corrected method are identified. Product work, not ledger expansion, is the objective.
