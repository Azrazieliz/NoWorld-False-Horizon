# Gate 01 — General Manifestation / Version Taxonomy (proposal v0.1)
Date: 2026-10-16. Status: production authoring guidance; **not** a Registry v2 approval, new content ID, rarity assignment or change to the frozen Foundation.

## Ontology boundary
**Identity** is the persistent canonical character. **Version** is an immutable content definition (Base, Alter, Awakened, etc.) attached to one Identity. **Owned Manifestation** is the independently developed runtime instance owned by a Ruler and may reference a Version. A **skin** is cosmetic styling without a new gameplay/ontological form. **Clothed / Underwear / Nude** are presentation states, not Versions or owned instances. Never collapse these layers or generate synthetic ownership.

## Semantic families (descriptive authoring tags, never automatic gameplay consequences)
| Form | General meaning | Authoring discriminator |
|---|---|---|
| Base | Canonical starting interpretation | Identity anchor: face, signature traits, body language and power baseline |
| Alter | Counterfactual or divergent expression of the same Identity | Meaningful difference in path, values, power source, personality expression or presentation; not merely a recolor or outfit |
| Awakened | Latent/internal potential becomes actively realized | Same person, demonstrable activation of existing potential; state and power grammar visibly mature |
| Ascended | Earned higher plane of mastery or being | Elevation beyond earlier constraints with coherent cost and mechanism |
| Evolved | Persistent structural development with new function | Explain developmental cause, new anatomy/equipment/abilities and what remains recognizable |
| Corrupted | Outside force or self-inflicted distortion compromises original nature | Define contaminating mechanism, visible consequences, agency and potential reversibility |
| Purified / Redeemed | Corruption or constraint removed, or path of recovery | Show repaired or transformed source and distinguish restoration from stronger awakening |
| Fallen | Loss of status, code, ideals or former power through narrative reversal | Psychological and social descent; no requirement for corruption |
| Berserk / Unbound | Inhibited control or limits released | Specify whether temporary combat state or independently authored persistent Version |
| Possessed / Symbiotic | Distinct agency shares or dominates the body | Preserve agency boundaries, host traits and source of foreign influence |
| Divine / Demonic / Mythic | Specific cosmological affiliation or metamorphosis | Require world-lore cause rather than aesthetics alone |
| Elemental / Aspect | Emphasis on one internally coherent affinity or domain | Meaningfully changed fighting grammar, not a color filter |
| Temporal / Historical | Same Identity at another stage or alternate chronology | Define point of divergence; protect continuity and version lineage |
| Reborn / Reincarnated | Identity persists through a new vessel/life | Explicit identity continuity rules, memories, anatomy and independent personhood |
| Fusion / Convergence | Synthesis of compatible histories/forms/actors | Define contributors, continuity, reversibility and identity consequences; Grand Convergence remains governed by its existing game-specific rules |
| Absolute | Character-specific post-MR pinnacle classification, where separately approved | Never infer from appearance or treat as a universal manifestation family; Gate 10 rarity authority applies |

A character can have multiple descriptive tags (e.g. Alter + Corrupted), but exactly one primary narrative rationale should explain the new Version. **Awakened**, **Ascended**, **Alter** are not automatic rarity tiers and do not imply fixed stat bonuses. Forms can be unplayable narrative states unless explicitly commissioned.

## Decision test
1. Define the base Identity invariant: face signature, recognizable identifiers, personality continuity, lore, silhouette and power motifs.
2. Write the *cause* and *counterfactual*: what triggered change, whether permanent, reversible, voluntary, and at what historical point.
3. Compare five axes: ontology/anatomy, psychology/alignment, power mechanism, visual language and narrative progression.
4. If all meaningful changes are wardrobe/hair/palette, author **Skin**, not Version. If a persistent transformation affects gameplay, physical makeup or lore state, propose a **Version**. A temporary visual effect may be a **state** inside a Version.
5. Select primary family, optional descriptors and an explicit diff table against Base. Preserve recognizability without forbidding dramatic changes.
6. Produce one exact premium hand-drawn 2D reference sheet per approved Version: front/side/back, facial expressions, palette, anatomy, clothing components and weapon/power details, plus an independent signature illustration. Approval/freeze of this exact 2D design precedes *new* 3D construction. User-provided 3D-first models are **reverse-reference exceptions**: inspect the actual model, reconstruct its faithful 2D sheet, present discrepancies for approval before changing 3D.
7. Use the consolidated Koikatsu Party -> KKBP -> Blender -> Unreal automation; automatically reuse verified tool mappings, settings, offsets, scripts, failure fixes and saved procedures. Never regress or remake approved source. Preserve cel-shaded anime/manhwa game direction and inspect face/material fidelity after export.
8. Per approved version produce independently loadable CLOTHED/UNDERWEAR/NUDE source cards as applicable; changing a Maker viewport toggle alone is not sufficient. Validate genuinely embedded outfit coordinates, test all cards independently, store hashes and source provenance; preserve shared identity and rig where viable.
9. Prepare version-specific kit/abilities for Gate 04, rig/motion/emotes for Gate 09, canonical registry intake for Gate 11 and integration/device testing for Gate 12. Coordinate Gate 10 rarity and presentation design, Gate 00 approvals. None of these are self-approved through Gate 01.
10. Separate provisional design from verified assets, freeze, Registry acceptance, playable bindings and actual Android verification; mark each independently.

## Form proposal minimum schema
```yaml
identity_ref: existing_canonical_identity
version_ref: null # assigned by Registry authority, never invented
primary_family: alter
secondary_descriptors: []
trigger: null
continuity_invariants: []
base_vs_variant_changes: {face: null, anatomy: null, psychology: null, costume: null, power: null, lore: null}
reversibility: unknown
state_or_version_rationale: null
source_3d_paths: []
approved_2d_reference_sha256: null
separate_artwork_sha256: null
clothed_underwear_nude_card_hashes: {}
gate04_kit_status: pending
gate09_animation_status: pending
gate10_rarity_status: pending
gate11_registry_status: not_submitted
gate12_android_status: not_tested
```

## Preserve three Gate 01 entry workflows
Original Character Creation; Feminisation (reconstruct rather than tweak a male base); Existing Character Adaptation (2D target before adapting KK model). This taxonomy applies to all three, never substitutes them.
