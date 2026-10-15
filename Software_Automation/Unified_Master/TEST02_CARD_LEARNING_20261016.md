# NoWorld SM01 — Test 02 native-card comparison (read-only)

## Evidence
- User-made card: `Koikatu_F_20261016001304793_Chika Haruno.png` SHA256 `62c575249f014ed9a479bb4753700a3e4a14fb520d91f03b801dda6c746d6419`
- Automated comparison card: `SM01_AUTOTEST_nw0ec8f513e10c42.png` SHA256 `97f031d6864d4419d26bd809578eb48137ab8c7580bf666bdb855fa6b521dc01`
- Full machine diff: `D:\UnrealProjects\NW_Blender_Private_Review_20261015\TEST02_USER_VS_AUTOMATION_DIFF_20261016.json`
- Target settings recipe: `D:\UnrealProjects\NW_Blender_Private_Review_20261015\TEST02_USER_LEARNED_RECIPE_20261016.json`
- Neither card was edited or overwritten.

## Findings
- Same installed Koikatsu environment. A new modeling library was NOT required to improve this design.
- Hair front slot 0 native item 2 -> 19, with warmer ivory base/root/tip colors; hair kind 0 -> 2.
- Iris item 10 -> 6, warmer amber RGB approximately (1.000,.705,.411) rather than (.980,.530,.080); eyeshadow ID 2 -> 9.
- Face: exactly 6 of 52 native shape values changed (indices 8,11,14,15,16,31).
- Body: exactly 11 of 44 native shape values changed (indices 4,5,6,20,21,22,24,25,38,39,40).
- Cheek blush absent (ID 0, opacity 0) in both saved cards.
- User active coordinate has 6 serialized accessories; automated card active coordinate has 0 serialized accessories across all seven saved coordinate slots.
- User accessory 0: type120 / ID34, headtop; position (.6,-2.4,-1.7), rotation (25,5,359), scale (.83,.77,1.75), second correction scale (.84,1.4,1).
- User accessory 2: type124 / ID20, bust; position (.1,1.1,.6), scale (1.3,1,.9), second scale (1,.01,1).
- User accessory 3: type120 / ID9, neck; position (.1,-.7,.8), rotation (336,0,0).
- User cuffs: type120 / IDs 15 and 14, parent wrist_R and wrist_L, native unit scale.
- User fishnets: category110 (co_panst) ID9 used as an accessory parented to the waist, rather than altering the saved pantyhose clothing part.
- Both cards still store clothing-top ID2 in coordinate0. A card thumbnail showing a custom bodysuit is therefore NOT proof that the custom outfit was serialized. Native save validation must check clothing and accessory data.
- User card contains KKABMX extension data absent from automated card. This confirms an additional data category, not necessarily that all such data was newly edited.

## Root causes corrected in isolated source
1. **Save routine omitted `ChaControl.AssignCoordinate(currentType,c.nowCoordinate)`.** Native game IL confirms this copies all active outfit+accessory data into stored `chaFile.coordinate[]`; called before `SaveCharaFile` in patched implementation. Native full-card roundtrip compares 9 clothing parts and each accessory's identity, parent and both correction matrices. Not yet installed in currently running game.
2. **Placement supported only primary position/rotation/scale.** Patched to support pos2/rot2/scale2; native readbacks report both matrices.
3. **Placement rejected legitimate rotations 336 or 359 and secondary scale .01.** Patched range to cover 360 degrees and 1% scales.
4. **Placement treated no-op native setters as errors.** Patched to skip axes already at target.
5. **Asset application required mod GUID even for vanilla assets.** Patched explicit `apply_accessory_native` mode accepting known category/effective ID.
6. **Builder lacked replay from an existing card.** Added read-only native card diff and recipe extraction; builder supports native asset IDs and six transform groups.

## Acceptance boundaries
- Game running untouched; original cards intact.
- Offline Unity3.5 source compilation PASS.
- Three Python scripts syntax check PASS.
- Read-only native comparison and recipe extraction PASS on actual cards.
- Patched runtime was NOT loaded into live game; card persistence after fix is **not yet runtime-tested**.
- Full image-to-3D fidelity and Blender export still unverified. Do not claim Test 02 visually accepted.
