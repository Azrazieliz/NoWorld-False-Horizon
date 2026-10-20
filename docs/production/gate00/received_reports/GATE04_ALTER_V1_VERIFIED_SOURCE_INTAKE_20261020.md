# Gate 00 — Luminous Valentine Alter v1 exact-source intake and Android availability
**Date:** 2026-10-20. **Status:** SOURCE VERIFIED · USER-APPROVED DESIGN · NATIVE GAMEPLAY NOT ACCEPTED.

## Received and independently checked
The user attached `Gate04_Luminous_Alter_APPROVED_Handoffs_20261019.zip` to the current Game content Creation project conversation on 20 October. Its bytes were opened directly; ZIP CRC integrity and **all 17 entries** in its `SHA256SUMS.txt` passed. This replaces earlier "Alter exact numerical package missing" notices; never rewrite the earlier historical evidence.
- Complete original ZIP SHA-256: `063be65715a86bd994ae906f144242b544ca635844cc33ab7ec87108e472e30c`; size **3,252,143 bytes**.
- Exact current approved design member: `Luminous_Alter_Offensive_Kit_v1_APPROVED_DESIGN.json`; size **35,551 bytes**; SHA-256 `2c6474548541df55b036eaa5a605805f64bba6fe212d4c91c4013c7fdd1a254c`.
- Approval record: `Gate04_Alter_User_Approval_Record.json`; SHA-256 `ce23ead657388b11c89dc08722756656ce27de84dd2ee8d09b8d894bafc9f2c4`.
- C++ source mapping draft: `Frozen_UE_Source_Projection_v1.json`; SHA-256 `a401b0a4f076265420f8c528bacf0cc2c46a63d1233ffac8d7ea91255ba144e9`.
- Provenance: current conversation upload file ID `file_0000000064008211b57f3ce2f570b0f1`, Library file ID `libfile_34e1ff025a70819193e7444e40b9e855`. Accessible in the Game content Creation project with Files and `files.materialize`; **do not** infer this is a Windows PC path or copy private Koikatsu cards into the public repository.
- Verified design: every named mechanically frozen property from approval record matches the original `PROPOSAL` snapshot. `reference_tests_result.json` reports 38 source-reference passes; **not Unreal or Android test passes**.

## Native source contract (EXACT original JSON is authoritative)
Same Luminous Character Identity, separate Alter Version, Origin MR, Hero Rank Lv1 HP 7650 / ATK 4830 / DEF 1040 / SPD 151, Crit Rate 37%, Crit Damage Bonus +132%. Exactly 3 actives, 6 passives, separate Black Mass dual-branch Ultimate and Cathedral World Fantasm. Never inherit Base v6 Day/Night kit into Alter. No accepted Alter Registry v2 identity, Manifestation grant, ownership, weapon, installed Unreal actor or playable character is implied.

| Core active | Target | Core exact numeric anchors |
| --- | --- | --- |
| A1 Vesperblade — Litany of Thorns | ST | Sword contacts 48%, 64%, 105% ATK; nonlethal self-HP tithe 2% Max HP; after 3 legal seals conditional +75% ATK finisher |
| A2 Black Eucharist — Rain of Thorns | 4 enemies max | 98% + 112% ATK each, primary-only bonus 100% ATK; nonlethal self-HP tithe 4% Max HP |
| A3 Martyr's Communion — Scarlet Apostasy | Self | nonlethal 8% Max HP cost; 2 owner turns/10s +26% ATK, +12% SPD, +18 percentage points Crit Rate; stance +22% Bleed potency and +12% DEF penetration only vs Bleeding; once-per-stance third-hit 50% ATK |
| Ultimate Black Mass, Rite of Ruin | AoE up to 4 | 100% Ultimate charge +25 Red Tithe +5% Max HP cost; three physical strikes 145%, 145%, 80% ATK / enemy |
| Ultimate Black Mass, Rite of the Hollow Crown | ST | 150% Ultimate charge +40 Red Tithe; six authored strikes 55%, 65%, 75%, 90%, 105%, 135% ATK, conditional additional 130% ATK after 3 legal seals |

The source contains **six** full event-driven passives: P1 Blood's Accounting (Red Tithe and damage-only lifesteal), P2 Anatomy of Ruin (Severing Wound + Rupture Seal), P3 Ecstasy of Attrition (Martyr Momentum), P4 Eclipse Harrier (perfect dodge/switch/riposte), P5 The Unfinished Death (conditional self-revival), P6 Debts the World Cannot Collect (observed family/World Negation accounting). Exact effects, timing and condition clauses must be read from approved source, **not extrapolated from this summary**.

Shared constraints: Red Tithe 0–100 (initial 20); eligible A1/A2/A3 cost grants +8/+15/+20 once per actual action; 18% direct physical lifesteal, 9% Severing Wound tick lifesteal and **16% Max HP per owner-action total heal cap**; Severing Wound is Bleed-derived physical DoT at 16% ATK/stack/tick (2 stacks); Rupture Seals are non-Bleed owner/target marks (3 max). Self-revival may happen only **once per encounter**, with a valid soul, **75 Red Tithe paid before succession**, at 18% Max HP. Authored true-damage families and generic elemental resistance must NOT be fabricated. Preserve seeded/frozen combat hit/crit/DEF/rank rules, per-action dedup, both existing executors and persistent Manifestation/Version state.

`Cathedral of the Unwritten Death`: 14m radius, 3 owner turns/16s, activation 60 Red Tithe, **four individually contestable** World Negation effects, including success/failure:
1. **Blood Reversal — Denied Recovery**: costs 15 Red Tithe; denies one eligible temporary heal event after two source-owned Severing Wounds; max two successful negations per field, event cooldown 5s.
2. **Thorn Unbinding — Broken Continuation**: costs 20 Red Tithe; contests one eligible regeneration or summon-maintenance tether attached to 3 Rupture Seals; consume marks ONLY on success, attempt pays cost regardless; once/target/field.
3. **Mirror Debt — No Returned Wound**: costs 20 Red Tithe; contests only an eligible reflected-damage *copy*, not the underlying real hit; one success per field.
4. **Boundary Decree — The Unwritten Margin**: zero individual cost; real intersecting World Fantasm/Domain boundary resolved per sector by compatible Authority/priority/strength; winning sectors advance, losing sectors retract; no global Domain erasure.
A failed contest does not confer negation. The original JSON also specifies additional source-owned limits (e.g., rule-category refund), status expiries, QTE, progression equations, field render cues and 16 native stat field mapping.

## Updated actual owner actions
- **Gate 12 / P0:** use exact original approved JSON from Files/Library, validate source SHA before native ingestion, resolve legitimate additive Registry intake via Gate 00 (retired Gate11 signoff is not a dependency); integrate skill/effect handlers into the **two existing executors**; preserve original ID conventions and all protected sources. Do not substitute the pre-approval `PROPOSAL` JSON or author fake canonical IDs. Only after source-grade + compatible native bindings may Gate00 accept a new Version.
- **Gate 09 / P0:** produce Alter-specific A1/A2/A3, both Ultimates, Cathedrals' four unique success/failure World Negation VFX/SFX/geometry and true reaction/hit windows. Original Alter rig must be independently verified; Base 975 bones do not establish equivalence.
- **Gate 07:** authentic sourced item/equipped weapon/anatomy; none automatically granted. All 18 current Gate07 user decisions remain approved.
- **Gate 02/Gate 12 World and shared motion scope:** no change; Gate02 anchors only, Gate12 authors procedural remainder; Gate09 common motion variations, Gate12 maps physics/ability profiles.

## Android device (NEW; separate from gameplay qualification)
At a live authorized connected Windows PC check on 20 October, `adb devices -l` identified **one authorized physical Samsung SM-S948B** running Android 17. `adb shell pm list packages -3` showed no package matching `offline|noworld|koikatsu|gate12`. The newest discovered local app APK at Gate12's workspace was dated **2026-10-11 23:57:22** (old, predates the four native clips imported on 20 Oct). **Do NOT install this stale APK and pretend to validate those four clips.** Actual current-version native character AnimBP binding, cook/build/install, visual playback evidence and clip deformation/performance must precede any `ANDROID_PLAYABLE` PASS. The phone is now available but can disconnect; recheck at test time.

## Immutable boundaries
Historical original 31 Registry v2 entries, Base v6, user-authored Alter three wardrobe cards, frozen Foundation `3f54f07`, PR #16 and player saves remain untouched. No new Awakened/Berserker/Grand mechanics or automatic Manifestations. This receipt is source intake and live-device discovery, not production art or gameplay acceptance.
