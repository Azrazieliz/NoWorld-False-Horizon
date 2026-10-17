# Gate01 / Software Mastery — Test 4 learning, Test 5 startup contract
**Recorded:** 2026-10-17.
**Purpose:** Benchmark a reusable direct-request automation system for Gates 01–12. This is NOT canonical character production.

## User acceptance and performance
- **Test4 resemblance rated 6.7/10 by the user** (their judgment, not a standardized computer-vision metric). “Slightly looks like the right character”; better than earlier attempts; still barely average.
- **Speed is unacceptable.** User observed approximately 25 minutes elapsed with perhaps 1 minute of actual character modification (**~4% productive operation time by that estimate**). Do not represent this as accurate instrumented telemetry; the old pipeline did not independently log idle vs productive time.
- Reference has Jeanne Alter-like military commander outfit **mostly BLACK**, fitted military cut, burgundy/red trim and inner cape, intricate armor, silver layered hair, gold eyes, a black engraved forehead crest, boots, sword. Incorrect color/material dominance or generic knight armor harms identity. Body size, accessory XYZ placement and scale are meaningful anchors.
- Initial wrong purple cape blocked front suit; wrong shoe coordinate meant armored boots disappeared; beige hair texture and white crown atlas caused color mismatch despite palette setters. Real UV-preserving source texture override was needed, but further polishing was slow.
- **Test4 STOPPED by user.** No further creative edits, no native card promotion or Blender export without new request.

## Binding Test5 workflow
1. Before any Koikatsu launch, parse image into prioritized **silhouette/face/body/outfit/hair/cape/gear/ornament** features. Include color DOMINANCE, shape, scale and placement, not just accessory presence or franchise association.
2. Query the entire installed **indexed asset catalog** in one pass. Candidate assets from dozens of installed packages can be ranked without booting Maker. Do not slowly search a single mod and restart after each choice.
3. **Pre-stage a broad set of relevant source mod packages once**, with exact hash checks and disk/memory budget, BEFORE starting the sandbox game. Never silently preload the full ~38 GB library as if memory were infinite; keep full catalog searchable and stage all plausible benchmark-relevant packages. If game is already running, do not copy archives into the active mod folder and assume they load; use currently loaded alternatives or postpone unavailable candidates until the next planned cold start.
4. **Exactly one planned Maker launch per benchmark**, no ad-hoc stop/restart for every missing asset. The pipeline itself NEVER launches or terminates a user-controlled Steam process.
5. Execute a **single Python native batch** using `kk_one_session.py batch`; no Python subprocess per operator. Use a neutral base, set face and body proportions deliberately, try several existing clothing/hair/accessory combinations, then reposition/resize precisely using XYZ/rotation/scale/second transforms.
6. Finalize colors and actual UV/material texture overrides **AFTER** asset selection, parenting and XYZ transforms (source material may reset on object recreation). Assert hair/cape/sleeve colors before acceptance to prevent regressions.
7. Capture one front/face/side set under controlled camera, identify a SMALL list of large errors, fix those using the fast native batch while keeping Maker open. Don't spend 25 minutes solving cosmetic 1% differences. Use Blender only when native asset fit demonstrably fails for key geometry.
8. Native card save+independent coordinate reload is optional in a disposable benchmark and should occur ONLY when the test requires an output. No unnecessary snapshots, giant intermediate files, repeated image transfer or native exports.
9. Measure **wall-clock from reference intake / pre-plan** separately from source selection, archive staging, runtime startup, native edits and visual assessment. Report every detected restart and command latency. Performance must be part of QA, not an anecdote.
10. After Test5, record any genuinely new API/source finding into the permanent `MASTER_LEARNED_METHODS_V1.json`, then add code and a failing-on-regression test; this is mandatory across Gates, not just for Koikatsu.

## New reusable implementation
- `kk_one_session.py plan --keyword "..." --guid "..." --out <new plan.json>` reads readonly catalog and calculates a bounded complete startup mod pool.
- `kk_one_session.py stage --plan <plan.json> --receipt <new stage.json>` deploys exact source copies ONCE **only while no Koikatsu game is running**. Original mods remain immutable. Rejects missing GUID, changed selected packages and unbounded staging cost.
- `kk_one_session.py batch --recipe <JSON list> --receipt <new timing.json>` sends multiple native APIs directly from ONE persistent Python invocation, refuses unauthorized destructive/save actions by default, never restarts Maker and records per-operation latency.
- `test_kk_one_session.py` verifies source index search, mandatory pools, disk limit, dedup, API validation, geometry/morph-batch TSV, receipts and no live app operations using a dummy submitter.
- **Current result is an offline implementation and regression proof, not a measured end-to-end Test5 speed improvement.** Require actual timed live Test5 results before claiming efficiency success.

## Regressions to prevent
- Native appearance dominated by wrong color after source asset transforms.
- Headgear floating above face; cape covering body; belt/sword attachment in wrong hand; wrong footwear coordinate.
- Body not sized appropriately because hidden by costume and never measured.
- Multiple Maker launches / restart loops in one benchmark.
- Asset discovery performed via visual UI while full indexed library already contains searchable candidates.
- Excessive planning/handoffs replacing practical edits; stopped benchmark continued after user STOP.
