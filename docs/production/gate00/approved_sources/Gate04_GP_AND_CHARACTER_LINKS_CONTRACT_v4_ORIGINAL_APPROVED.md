# Gate 04 — Account GP, Character GP and Relationship Links (v4 proposal)

**Status:** Proposed system/content contract; not a frozen engine change, not installed in the Registry. **Preserved authority:** `docs/RULER_PROGRESSION.md`, `docs/EXISTENCE_POWER_RANK.md`, `docs/CHARACTER_PROGRESSION.md`, `docs/TERRITORY_PROJECTS.md`, `docs/COMBAT_MATH.md` at Foundation `3f54f07cacbf193e160d0be15a7be5a7bc730c3b`.

## 1. Three different meanings

- **Character GP** is a magnitude-sensitive estimate of one *currently available Manifestation's* current effective combat capability. Character GP does not replace Rank, Level, Rarity, skill, gear, Factors, counterplay or knowledge.
- **Ruler / Account GP** represents the player's current **total effective power** across protagonist, unique Character Identities, genuine controlled Territories, Domain Cores, independent military resources and proven inter-character bonds.
- **Ruler / Overlord status** is an authoritative *sovereignty-state determination*, not a hidden threshold of GP. Overlord needs both sufficient real personal/existential Authority and actual high-order territorial sovereignty. A ruler can have much higher GP than another without either automatically changing title. GP can influence descriptive Overlord standing and outside observers' threat estimates.

## 2. Exact account aggregation

For each `CharacterIdentityId`, select the **highest currently available Manifestation GP** among legitimately owned copies and Versions. Exclude retired, permanently lost or Convergence-consumed source copies. **Do not sum Base + Alter + Awakened + Berserker + Grand for the same Identity.** The player may have multiple copies for development, but only the single strongest available copy represents the Identity's Account GP contribution. Combat party identity exclusivity remains unchanged.

```
AccountGP = ProtagonistCurrentPersonalGP
          + Sum( MAX(AvailableManifestationGP) per unique CharacterIdentityId )
          + Sum(EffectiveControlledTerritoryStructuralGP + DistinctDomainCoreGP)
          + Sum(IndependentNonRosterAssetGP)
          + Sum(VerifiedDistinctIdentityLinkBonusGP)
          + OtherUniqueEffectivePowerGP
```

Components must carry stable authoritative **source IDs** and an ownership/contribution ledger. This avoids double counting an army unit that is already counted as a companion, or military infrastructure also represented as a Domain asset. Domain power is not an artificial acreage value. Territory GP depends on actual effective control, defenses, resources, infrastructure, populations where relevant, Domain Authority and autonomous assets. During contested control, include only the Ruler's actual effective capacity; do not grant two claimants full GP for the same source. A recovered Territory retains historical identity under existing reclamation rules.

**Worked reference scenario at Hero Level 1** (not a real account): Luminous Base GP 37,485 and Alter GP 51,404 -> character contribution is **51,404**, not **88,889**. Add current Ruler GP 45,000; Territory infrastructure 60,000; Domain Core 25,000; one independently owned companion 18,000; and two earned links adding 2,700 -> **202,104 demonstration Account GP**. No title is inferred.

## 3. Candidate character GP (explicit, numerical)

Character stats are read from actual *current resolved* `FOGCombatStats`, not from separate universal RPG templates. No need for new fabricated Bleed/Burn/Magic resistance fields. Only the existing 16 native stats appear in source definitions; relevant additional effects appear in the actual six-action benchmark, rather than invented stat fields.

```
StatGP = (0.30*MaxHP + 1.30*ATK + 0.90*DEF + 6*SPD)
       * (1 + 0.20*min(CritRate,100%)*max(CritDamageBonus,0))
       * (0.70 + 0.30*min(Hit,180%))
       * (1 + 0.15*min(DEFpenetration,100%))
       * (1 + 0.12*min(Dodge,100%) + 0.10*BlockRate*BlockReduction)

SixActionGP = 0.40*SixActionSTDamage
            + 0.22*SixActionAoETotalDamage
            + 0.30*SixActionEffectiveHealing
            + 0.15*SixActionActualShieldAbsorption

ManifestationGP = round_half_up(StatGP + SixActionGP)
```

Percentages are fractions of 1 inside the actual formula. These weights are **initial illustrative coefficients requiring one shared balance calibration**; they are not pre-existing frozen rules. Do not multiply by Rank, Rarity, Class or Level again: their real capabilities already influence resolved stats, six-action outcomes and world power. Retain `FOGLargeNumber` and deterministic decimal/basis-point arithmetic in Unreal for extreme Rank magnitudes. The supplied Python uses Decimal for its reference aggregation; the HTML interface uses floating point *only for readable approximate review*.

### Six-action benchmark definition

Use one published, immutable benchmark scenario set per Rank band, with appropriate target-defense references and target counts. Resolve actual six legal owner actions per tested kit using the same turn/World shared `FOGCombatMath` rules, seeded Crit/Hit/Block and true effect timing. Include sustained and opening damage, support, field effects and passive follow-ups. Separate ST and AoE to avoid valuing nominal AoE magnitude as if all enemies were always present. Normalize by six legal actions / actual action-value time. Ordinary damage from a non-lethal absorption shield should not be recorded as HP damage or vampirism. A healing cast into a fully healthy team has zero **effective** healing. A protection that never absorbs an attack does not contribute as effective shield absorption.

**Current browser/JSON status:** Native benchmark fixtures do not yet exist. It uses explicitly authored six-action magnitudes as an approximation, *does not claim exact simulation*, and omits the shield absorption term until a real resolver supplies it. No numerical GP should enter canonical saves based on these approximations alone.

## 4. Links / bonds between Characters

Links are earned relationship milestones supported by actual game history, not just a gallery ownership checkbox. Examples of **types**, not canonical relationships: Shared Campaign (actually fought together), Oath / contract, faction history, documented narrative bond, friendship, rivalry, paired achievements, master–disciple or Ruler relationship. They can create both **Account GP** and separate **real gameplay bonuses** if designed, rather than merely displaying +GP.

Each author-defined link requires two **distinct identities**, an unlock predicate, an earned stage and at least one actual mechanical consequence. Its **account-only** bonus is computed once using the *raw strongest available character GP* of its two distinct endpoints (or Ruler personal GP):

```
LinkBonusGP = EarnedLinkRate * min(RawSelectedIdentityGP_A, RawSelectedIdentityGP_B)
```

Example milestone proposal: Recognized 2%, Proven 5%, Legendary 10%; select highest earned stage, never stack the same link at three stages, and never calculate link bonuses on already link-bonused GP. No global bond is created merely by owning two random characters. A pair whose members are two Luminous copies is *one Identity* and adds **zero identity-collection link GP**. A linked but unavailable or dead companion contributes according to the actual permitted ownership/availability rules; there is no phantom temporary bonus.

Group links involving three or more distinct Identities can exist as **one authored set** but must publish contribution IDs, verified participants, explicit rate/requirements and no overlapping double-credit. Link bonuses may be earned in quest/faction/world mode with meaningful narrative consequence. An independent team's chemistry may also grant buffed stats or a coordinated QTE, which then affects the actual skill-resolver-derived GP as appropriate; do not count the same bonus twice in both a flat and derived channel without an explicit author rule.

## 5. Grand Convergence acquisition and GP

User-specified Luminous requirement: **five** individually acquired, **maximally reinforced** Manifestations across Base, Alter, Awakened and Berserker, plus completion of Convergence conditions. The fifth copy's distinct lineage and the full bespoke proof set are unresolved; do **not** invent a mandatory fifth Version. After real Convergence, selected source Manifestations cease separate deployment and are preserved as provenance of **one Grand**. Only Grand's current *own* GP counts for the Identity, with no extra +GP for consumed sources. It can legitimately exceed lesser Luminous forms by 10x or more; some unrelated endgame characters can still exceed Luminous Grand.

## 6. Implementation boundary and verifiable behavior

Implementation uses a deterministic, non-recursive dependency graph:

1. Read actual personal/Ruler and Manifestation resolved states and effective Rank.
2. Evaluate each manifestation and its registered effect/benchmark outcomes; calculate raw GP, cache by stat/equipment/learned-skill/progression generation.
3. Resolve `bestAvailableManifestationByIdentity` using stable `(GP desc, acquisition time asc, stable ManifestationId)` ordering. Never force Convergence or create imaginary copies.
4. Sum controlled Territory/Domain/unique independent assets by stable source ID without overlap.
5. Evaluate valid inter-Identity link achievements from their base, unlinked totals; apply authored rate exactly once.
6. Publish `AccountGP` through `FOGLargeNumber`, and separately run the Ruler/Overlord sovereignty-state resolver against actual qualifying conditions. Persist history/Chronicle for gains/losses with real provenance.

Tests must reject duplicate ownership IDs, double-counted stationed companions, unusable source copies, link recursion, two links that represent the same relationship, arbitrary status promotion from GP alone, loss of unambiguously real power through display-rounding, and Convergence source copies remaining double-counted after merge. The included `gp_reference_v4.py` implements the prototype duplicate guards and the identity-max/link calculation.

**Approval:** All parameters in this file are proposed to Gate 00. No committed gameplay architecture is altered by this package.