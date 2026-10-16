# P0 — Gate 10 frozen UI mismatch and sword-combat presentation correction
Date: 2026-10-16. Status: **NOT ACCEPTED — VISUAL PRODUCTION HOLD**.
Authority: user-corrected Gate10 final handoff, original `Gate10_FINAL_Gate00_Handoff_2026-10-13.zip` SHA256 `b92074ca762d4db3228ef237747e664ea3abc9c2da48941a20716ae84c2e7f49`, and later direct user instructions removing character chips and requiring swords rather than firearms.

## Directly inspected source versus actual real Win64 captures

**Opening — NOT ACCEPTED.** Current actual game `docs/production/gate12/evidence/issue25/Win64_StartingWorld_Opening_NoCook_20261016.png` is a static flat blue/grey panel with buttons over generic mountains and moon. Gate10 approved **opening direction 04, Vacant Throne**, in `Gate10_Opening_10_Concepts_2026-10-12.zip/variants/04_vacant_throne.png` and subsequent final written freeze: ominous gothic throne, bespoke title/premium menus, HSR-like character/scene ambient motion, separate responsive portrait/landscape compositions, valid Continue/New Game/Settings/loading/recovery/offline; privacy and reduced-motion alternatives. The concept image is *reference only*, not an acceptable baked screenshot as the runtime UI. The actual opening must be editable native UMG + authored animation and real data.

**World HUD — NOT ACCEPTED.** Actual `docs/production/gate12/evidence/issue25/Real3DPreviewAndWorld_20261016/world.png`: blocky gray controls, centered orange diagnostic humanoid, unstyled minimap/bar/action controls. Frozen `Gate10_Continuation_Validated_2026-10-12.zip/01_FROZEN/WorldHUD/world_hud_freeze.png`: ornate thin angular dark/gold anime HUD, anchored minimap upper left, world/destination and party elements, lower-left joystick, lower-right sword-oriented combat cluster, lower HP/resource styling, fully diegetic world visibility. The frozen image conveys layout and art direction; **do not use screenshot as UI**. Reimplement editable UMG with native live data, touch-safe areas, viewport/orientation adaptation; no fake party/characters.

**Ruler Home, Records, Gacha, Territory, Character Roster — NOT VISUALLY ACCEPTED.** Current `UOGGate12NativeRulerWidget` is a working canonical data shell with real five-page navigation; actual screenshots show functional layouts but not the approved dark high-detail premium ornate surfaces, backgrounds, illustrations or visual density. Gate10 original frozen refs are inside the final handoff:
- `01_FROZEN/RulerHome/approved_visual.png`
- `01_FROZEN/Records/approved_visual_with_placeholder_text.png` (placeholder data must NEVER become canonical)
- `01_FROZEN/Gacha/user_approved_reference.png`
- Three selected Territory representations are equivalent as user said; do not ask user to choose again.
- Approved roster v0.2: portrait three identical tall prismatic cards per row, artwork dominant, upper-left thin-rim rarity insignia, real Identity/Version/owned Manifestations; no chips. Thin badge and border design R yellow SR pink SSR red UR blue LR white/grey gold insignia MR matte black platinum insignia; no rarity VFX; bespoke per-character Absolute.
- Exact selected Android icon original source `818455.png`, not a newly invented title image.

**Sword combat presentation — NOT ACCEPTED.** User explicitly stated that these characters use **swords, not firearms**. Frozen World HUD's large main action is a sword/blade emblem. The current runtime uses a generic `foundation.primary_attack` action and an unarmed diagnostic humanoid; its neutral control label “Attack” and circular icon do **not** demonstrate a real sword, held weapon, animation, hitboxes or visual effects. Source grep for literal `gun/rifle/pistol/firearm` in runtime C++ did NOT reveal actual firearm equip code; however that **does not** prove that the game implements swords. All eventual approved weapon meshes, animation montages, combat VFX, sound and loadout UI must be sword/melee-relevant to their accepted character kits; no firearm defaults/assets or firearm-oriented combat visuals. Reticle/camera lock can remain a melee target-lock if approved by Gate10; do not misclassify a target reticle itself as a gun.

## STOP erroneous gate acceptance semantics
- Prior tests 16/16 real Ruler/World flow and 150/150 native mechanics do **not** prove *any* Gate10 visual match. Do not label opening, World HUD, five native Ruler pages, icon or character presentation FINAL/APPROVED from a passing build, screenshots, or template checks.
- Existing QA visual source (45-part Luminous test actor) does not replace actual approved final character or sword equip.
- Frozen Foundation worktree, original saves and APKs, Registry v2, PR#16 untouched. No Android cook/package/install.
- No redesign of accepted frozen Gate10 layouts and no request for further design approval.

## Production implementation and acceptance order
1. Import/recover the exact supplied Gate10 source references into a versioned *reference-only* read-only visual intake; verify hashes and separate approved/frozen from older rejected or unapproved concepts. Use **opening direction 04** + final written freeze, not the original ZIP's own “ten unapproved” status.
2. Build **real native UMG opening** scene with approved premium throne atmosphere and runtime animation/responsiveness, preserve functional New/Continue/Settings/Recovery and lossless save behavior, honest disabled states.
3. Replace the prototype World **visual HUD** with native editable Gate10-styled layered Slate/UMG widgets and data-driven controls; preserve proven input and World transition. Main combat action must visibly be sword/melee based. Hold unresolved equip/animation content pending the actual approved asset.
4. Recreate approved Ruler Home/Records/Gacha/Territory/roster structures with their actual visual hierarchy, not screenshot textures, ensure portrait three-card invariant, rarity grammar, true state bindings and all five nav actions.
5. Obtain native game screenshots side by side with frozen references **at matching aspect ratios**; gate visual acceptance screen-by-screen, distinguish source-complete, runtime-functional, visually-matched and device-verified. Test portrait and landscape, Privacy/SFW, reduced-motion, loading/recovery.
6. Gate09 and Gate01/07 author or provide **authentic sword assets, equip/attack locomotion and animation** for user-approved characters. Gate12 binds genuine approved sword content and validates real hit detection and action presentation; does not fake source actor or authorship.

## Current blocked actions
Actual finished visual UMG is missing. Genuine character equip and sword animations are missing. No production UI art import, nine-screen final visual capture or user-visible Android acceptance exists. Report those as unresolved until implemented. This document records the exact failure and is not itself a visual implementation.
