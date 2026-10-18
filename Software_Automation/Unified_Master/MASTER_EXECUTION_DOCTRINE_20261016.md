# NoWorld SM01/SM02/GAEA/UE — MANDATORY LEARNED PRODUCTION METHOD
**Effective 2026-10-16.** Owner: shared Software Mastery, consumed by Gates 01–12.

**2026-10-18 PRIORITY — Gate01/Test06:** Read `TEST06_8OF10_GATE_SCORE_CONTINUITY_20261018.md` and K042–K044 before any further Koikatsu character test. The White Gold Dancer visible result received **user 8/10** after its earlier native no-approval receipt was written. Its exact original scripts and four-view QA are the preserved highest user-rated benchmark; the pre-Test06 feature-gate code at `521d725` remains the baseline. Do not substitute the failed witch, claim user 8/10 was computed by numeric gate scoring, or run further unsolicited calibrations. This requirement is strictly about Koikatsu automation continuity.
**Reference authority:** `MASTER_LEARNED_METHODS_V1.json` (26 initial lessons). All prior older narrative handoffs are subordinate when they conflict.
**Purpose:** Let the commissioned Gates automate real production requests through repeatable tools. This test series assesses the assistant's capabilities; the sample characters are disposable QA, **not** required canonical game characters.

**2026-10-17 addition:** K035–K037 require one-session Koikatsu operation: recognize Test4's predominantly black fitted military commander outfit and proper XYZ/scale (user rated 6.7/10); plan all plausible matching existing mods before ONE Maker launch, run native edits in a single Python batch, track actual latency, and never repeatedly restart Maker for one new candidate. The 13/13 new batch checks are offline only. See `TEST04_LESSONS_ONE_SESSION_GATE01_20261017.md`.

## One-time knowledge acquisition; automatic reuse thereafter
1. At the beginning of EVERY task, run `gate_start.py --gate NN` or use `gatectl.py`/`pipeline_runner.py` (these already load `mastery_preflight.ensure_master()` automatically).
2. Use the returned `known_methods` to choose the correct already-tested native API, parameters, asset IDs and validation. **Do not rediscover a previously resolved issue or reimplement a verified tool.**
3. Resolve ownership first: Gate00 approves canon; Gate11 controls registry; Gate12 owns UE runtime; domain Gates own their production sources. A software fixture is neither asset acceptance nor merge permission.
4. Apply the user's explicit current goal/threshold. For disposable benchmarking, stop when the user says the capability is sufficiently assessed, even if image likeness is not pixel perfect. Do **not** waste hours perfecting temporary cosmetics, environmental wetness or scenery.
5. Preserve existing correct state and assets. Compare with prior passing readbacks and renders. If a later modification breaks a previously correct property (hair color, sleeve color, blouse, parent), immediately restore that known-good state or fail the candidate, not keep going.
6. After ANY new discovery or correction: record the precise action, parameters, evidence, preconditions, tested outcome and regression test in `MASTER_LEARNED_METHODS_V1.json`, add/update relevant code and tests, run all read-only regression checks and native tests when approved; commit to isolated `software/mastery-integrated`. **Do not trust chat-only memory.**
7. Only then use that technique by default in later tests across all relevant Gates.

## Normalized operational quality gates (cheap first)
- **Reference:** use actual native/full-res user upload when it is available, display beside Maker with zoom/crops if access permits; never send 144p thumbnails or consume minutes transferring an image over base64. If only vision-in-chat is available, extract attributes directly without pretending a local viewer shows the original.
- **Identity:** face shape/iris/makeup/expression, anatomy/bust/hip/thigh ratios, layered hair silhouette and major outfit are first. Default Chika Haruno or default school layers are NOT implicit foundations. Each value should be intentionally set, with named slider mapping.
- **Asset choice:** query indexed installed catalog; rank several similar local assets by actual silhouette; rapidly sample candidates; don't model a whole shirt in Blender while better Koikatsu tee already exists. **Same assets + correct XYZ often suffices** (user Test2).
- **Placement:** select correct parent bone (headtop/neck/wrist/bust/waist), then position/rotation/scale. Include SECOND correction pos2/rot2/scale2. Relative scale 1.0=100%, 0.01=1%, angles up to 360; verify actual coordinate readback and rendered location.
- **Order dependencies:** save/editing should not reset material overrides. If asset/renderer changes recreate mesh, apply native color + material overrides LAST (hair root/tip, _Color, _Color2/3, shadow/line; garment diffuse and sleeve palettes). Re-render and inspect. Test3 braid orange->white/grey and sleeves white->navy were regressions.
- **Character wardrobe:** actual CLOTHED/UNDERWEAR/NUDE designs and material/mesh assets, not simply hidden/masked objects; privacy display is independent of actual selections.
- **Persistence:** `AssignCoordinate` from live `nowCoordinate` to stored card coordinate BEFORE SaveCharaFile. Reload the saved card, compare all clothing IDs, accessories and both transform sets. Test2 automated card lost all accessories despite apparent UI editing; Test3 patched saving verified all 9 clothing parts and 20 accessory slots.
- **Export/Blender:** export with KKBP 5.6 and dependencies correctly active; require real PMX and source maps, bones, skinned meshes, shape keys. Blender must load/assign original KKBP texture images (albedo, normals/masks, shader parity), preserve detail. **Zero images imported is a FAILED MATERIAL IMPORT** even when an editable .blend and render exist. Render + inspector should be compared to KK reference before accepting, no flat-color replacements.
- **Source quality:** preserve native rig, editable morphs, UV maps and normals; no invisible geometry or double-sized generated wings. Animation must use actual skeletal movement, not moving camera props. Unreal must have native tests and device rendering where appropriate.
- **Proof statuses distinct:** API_ROUTE_PASS / native setter/ native card save / editable skinned Blender / visual acceptable / Android runtime acceptable / Gate00 canonical approved are not interchangeable.
- **Timing:** batch direct API calls (~subsecond each demonstrated), avoid unnecessary GUI restarts; use immutable source hashes and only one final deliverable for user when requested. User calls out minutes of idle surveying and needless GUI navigation as failure.

## Test1 — silver hair, lilac eyes, white-gold outfit
Failed badly (default face, ineffective hair layering, wrong outfit, no meaningful faithful 3D). Lesson: asset matching and face/body anchors cannot be bypassed; do not count technical procedure as reference accuracy.

## Test2 — ivory bunny and direct user correction
The user did NOT switch to a different mod set. They used the **same setup**, changing XYZ, rotations, scales, colors, face/body, layered hair and added fishnets. Comparing native saved cards revealed 6 changed face sliders, 11 body sliders, 4 hair parts and 6 accessory slots. Concrete data at `TEST02_CARD_LEARNING_20261016.md` and two private QA JSONs.
- Bunny ears: parent `a_n_headtop`, pos `(.6,-2.4,-1.7)`, rot `(25,5,359)`, scale `(.83,.77,1.75)`, scale2 `(.84,1.40,1)`.
- Neck detail pos `(.1,-.7,.8)`, rot `(336,0,0)`. Chest detail primary scale `(1.3,1,.9)`, secondary scale `(1,.01,1)`.
- Wrist cuffs: native wrist_R and wrist_L bones. Fishnets: category110 native ID9 waist attachment.
- Main defect: **automated card had ZERO serialized accessories** despite live changes. Correct `AssignCoordinate` before native card save. Test3 subsequently demonstrated runtime 9/9 clothing+20/20 accessory saved roundtrip, but **exact Test2 user recipe replay fidelity remains unverified**.
- Cosmetic defaults, face identity, bust/hips and layered hairstyle must be deliberate.

## Test3 — orange braid, blue eyes, white tee/thigh-highs
**User states overall capability benchmark sufficiently assessed.** It is NOT an approved canonical 3D reconstruction. User had to increase bust drastically; this is a failure in body targeting. Native test used shape controls, orange hair, braid, iris, tee/stockings. Fix mistakes systematically:
- The UI catalog lists 47 body controls but existing native wrapper accepts indices 0..43. Do not invent normalization bounds for capabilities not actually tested. Document extra controls as gap.
- Accessory transform or replace can reset some material colors: always final-color after shape/placement. Regression observed orange braid -> white -> grey; white sleeves -> navy.
- Native KKBP PMX -> Blender confirmed 2 armatures, 454 shape keys, 237845 vertices; a successful editable Blender is **not** a high-quality visual result.
- Blender MMD imported shader groups but loaded **0 image textures**, producing flat/incorrect render. KKBP export included real face/hair/shirt/sock PNG CM/DM/MT/etc. This remains the TOP next SM02 source-parity defect to fix, not a texture-less 'polish'.
- Character wetness is environment-dependent, so omit unless inherent to a character design or user specifically asks for wet variant.
- Wasted time modeling wrong shirt-dress in Blender while native `deadshark.Oversize Tee` was in the available catalog. Preserve expensive rendered detail and choose simpler software route.
- The user ended Test3 refinement. **No more character operations unless explicitly commissioned**.

## Gates 01–12 — who reuses the shared method?
| Gate | Native source owned by Gate | Reusable master action |
|---|---|---|
| 01 | Characters/models, wardrobe, rigged geometry | SM01 catalog + native recipe/card data, KKBP, SM02 mesh/material preservation |
| 02 | Worlds/terrain/architecture/ecology | Gaea known-good graph/erosion, tiled exports, Blender geometry, UE world QA |
| 03 | Music, adaptive score, editable audio source | Distinguish musical brief from actual rendered MIDI/stems/lossless masters |
| 04 | Kits/skills/stats/combat | Source-contract validation, typed effects and reliable deterministic fixtures |
| 05 | Lore/story/quests | Editable graph and causality states, spoiler separation, testable quest data |
| 06 | NPCs/factions/civilizations | Persistent identity simulation, condition tests and schema QA |
| 07 | Equipment/items/loot/economy | Native items with anatomical slots, Registry/UE compatibility checks |
| 08 | Encounters/dungeons/events | World-conditioned persistent encounters, no forced routes |
| 09 | Animation/VFX/cinematics/SFX/voice | Blender rig/clip source QA, real motions; Gate12 native retarget and physics |
| 10 | UI/UX/reference assets | Approved visual source, layout variants, app visual testing |
| 11 | Registry integration | Registry v2 intake/schema acceptance, no invented owned IDs or packages |
| 12 | UE runtime/Android | Native engine authoring, saved asset reload, animation/material parity, device QA |

**Tooling truth:** `gatectl.py` currently implements only Gate00, Gate01, Gate02, Gate09, Gate12 owner IDs; other Gates can read this methodology via `gate_start.py`, but direct execution routes are not implemented. DO NOT imply Gate03–08/10/11 can automatically run all jobs already. No new canon is commissioned here.

## How next Gate specialist starts
```powershell
$root='D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Unified_Master'
$py='D:\Tools\Python311\python.exe'
& $py "$root\mastery_preflight.py"
& $py "$root\gate_start.py" --gate 01  # change to own gate; structured learned procedures
& $py "$root\mastery_regression_tests.py"
& $py "$root\runner_regression_tests.py"
# Standard native test-only jobs/plan THEN load knowledge automatically:
& $py "$root\gatectl.py" --job '<new job spec>' --preflight
```
Failure of any guard means investigate the regression, NOT bypass master startup.

### Known open gaps (NOT accepted; never silently 'fixed' in narrative)
- Full reference image reliably available to local KK viewer without slow transfer: partially supported, not end-to-end verified.
- Robust reference -> optimal existing KK asset selection and coherent full anatomy/face: no acceptance, user corrections still required.
- Full neutral-default base plus true three-way wardrobe output acceptance: missing.
- Native controls for body morph indices 44–46 and non-normalized values: unverified.
- Koikatsu native no-regression readbacks for material colors across export: missing automation.
- Blender accurate native texture hookups, geometry/material retention and reference visuals: failed on Test3; highest SM02 priority.
- Reliable natural movement, physical foot contact, swim/fly sustain, anime expressive choreography: outstanding Gates09/12.
- Gaea arbitrary-layout graph automation; UE Landscape and World Partition: outstanding Gates02/12.
- Unreal material/animation Android production acceptance: outstanding Gate12.

Do not waste time polishing or shipping the three disposable character tests. The actual deliverable is a general reusable pipeline with continually improving success, speed, and no rediscovery of old lessons.
