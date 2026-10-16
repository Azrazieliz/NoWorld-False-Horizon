# Gate 01 — Version Authoring Taxonomy (restricted, 2026-10-16)
**Status:** Working Gate 01 authoring methodology, aligned with the existing Character Identity architecture. **Four named Version families are user-defined (including Base); a fifth category slot is reserved, not invented.** This document is not a Registry v2 approval or a change to frozen game architecture, fixed rarity, combat kit, owner data, or Foundation.

## Existing architecture — authoritative

- **Identity:** the canonical person/character; can have multiple Versions.
- **Version:** immutable authored form/content definition of that Identity; may have separate 2D art, 3D model, kit, animation and combat role.
- **Manifestation:** an independently owned, persistently progressed runtime entity/card pointing to Identity and an applicable Version. One Ruler may own multiple instances with different progress.
- **Skin:** cosmetic alternative without separate form/kit/ontology.
- **Appearance state:** CLOTHED / UNDERWEAR / NUDE, where applicable; these are not Versions, owned Manifestations or separately acquired characters.
- **Rarity:** independent system and Gate 10 authority; neither the Version type nor presentation automatically determines rarity.

These existing layers must not be renamed, merged or reinterpreted even if a conversation informally calls a Version a “manifestation.” Preserve existing Registry v2 and approval boundaries.

## User-defined Version categories (up to five, counting Base)

| Type | Operational definition | Evidence that distinguishes it from a skin |
|---|---|---|
| **Base** | The primary / identity-anchor Version of a character. | Carries the agreed canonical starting depiction, powers and role. |
| **Alter** | An **inverted, darker, or fundamentally altered expression of the same Identity**, using *Fate* Alter Servants and *Date A Live* Inverse Spirits (Sephira inversion) as the user's intended reference family. A defining aspect of the character is inverted, distorted, suppressed or re-expressed: disposition, instincts, allegiance, values, emotional state, power aspect, symbolism, or an alternative interpretation of their essential nature. | The alteration must be **character-specific, substantive, and legible** in behavior and power identity as well as visual language, not merely clothing/hair/palette. It may be a distinct persona or a recognizably continuous self; not every Alter is evil, mindless, corrupted or stronger. |
| **Awakened** | The character's original identity, motifs and powers are **drastically amplified** into a distinctive evolved design, without requiring holiness, a divine theme or bodily mutation. | The character herself is visibly redesigned through a meaningful combination of hair color gradient/new-colored strands, hairstyle/volume, eyes and expressions, facial/body markings, clothing cut and silhouette, character-specific equipment, and optionally anatomy or integrated power features. Effects and background are supporting elements, never the main transformation. |
| **Berserker** | A distinct Version where the character enters an overwhelmingly unleashed, feral, frenzy-driven, unstable or destructively overpowered state. **Milim Stampede** is the user's reference example for this family—not for Awakened. | Distinctly transformed expressions, hair, silhouette, garments, markings, anatomy when appropriate, motion/posture and combat presentation embody loss of restraint or overwhelming aggression. Mere rage effects, particles or another costume are insufficient. A fixed level of consciousness or evil alignment is not assumed; define each character's control and trigger. |

**Category count:** 4 named categories = Base + Alter + Awakened + Berserker. **Up to 5 total categories including Base**, so **1 optional category remains undefined** pending explicit user approval. This is a limit on the *design taxonomy list*, **not** a new engine limit on the number of actual Versions per Identity, owned Manifestations or character copies. Do not enforce a gameplay/database cap from this document. Existing independently approved content rules elsewhere in the game remain untouched; unapproved category proposals stay outside the source taxonomy.

## Alter — reference-based creative standard (corrected 2026-10-16)

**User-provided references, not copied mechanics:**

- *Fate*: Alter Servants, particularly the notion of an altered/inverted aspect or alternative interpretation of an established Heroic Spirit. Alter appearance, behavior, power style and symbolism can change considerably; Grail corruption is **one** possible mechanism, not a requirement for all Alters.
- *Date A Live*: the **Inverse Spirit / Sephira inversion** example, in which a Spirit's internal nature, state, attire and armament can invert (Sephira -> Qlipha; Angel -> Demon King in that franchise). **Do not import** Sephira, Qlipha, Demon King systems or their precise triggers into NoWorld. They illustrate depth of inversion.

### Design intent
An Alter is **not** a broad alternate-outfit fashion redesign. The fundamental change is an **inversion or distortion of some core facet** of who the character is, how they act, or how their power expresses itself. The visual treatment must communicate that transformation. The altered facet must be chosen separately for each Identity; no universal evil/dark-haired/black-and-red template.

### Required Alter design sequence
1. **Establish original essence:** What are the Base character's signature values, temperament, motivations, symbolism, power identity, appearance anchors and relationships?
2. **Choose the altered axis and cause:** Precisely what was inverted, darkened, distorted, suppressed, amplified or reinterpreted—and why? May be psychological, narrative, metaphysical or a distinctly authored alternate interpretation; don't invent a mandatory mechanism.
3. **Define behavioral consequences:** Contrast motives, demeanor, morality/instinct, expressions, posture, voice and relationships; preserve sufficient canonical continuity to recognize the Identity.
4. **Transform power signature:** Determine how ability themes, weapon/armament, combat behavior and presentation change to express the new nature. Do not automatically assign superiority, rarity, class or fixed gameplay modifiers.
5. **Translate into visual grammar:** Reinterpret the Base design's identifiable palette distribution, silhouette, costume geometry, signature adornments and power motifs, with selective inversions/replacement. Maintain character-specific facial and structural continuity while permitting meaningful changes. A simple dark recolor alone is insufficient.
6. **Test recognizability and distinctness:** The player should identify both the original character and the radical difference in nature or expression. Document exact before/after design rationale, not just an adjective like 'edgy'.
7. **Approval and implementation:** Freeze the exact art (or reverse-reference an existing user-authored Koikatsu model first); send proposed combat differences to Gate 04, animations/acting to Gate 09, visual/rarity authority to Gate 10, Registry to Gate 11, and Unreal integration to Gate 12. Keep these independently unapproved until their actual review.

### Common Alter visual tendency — darker and bolder (user clarification, 2026-10-16)
An Alter **often** shifts toward a darker, bolder, more forceful visual interpretation. In art direction, test deepened value ranges, stronger black/dark-color massing, sharper and higher-contrast palette separation, stronger silhouettes and sharper costume geometry, more commanding/intense expressions, and more imposing armament or power presentation. Treat these as **strong reference tendencies**, not mandatory fixed requirements. A character-specific inversion may use other colors or contrasting directions. Do not apply automatic black hair, red eyes, black-and-red costume, sexualization, evil alignment or a universal template. The key is that the bold/dark choices visibly express the Alter's transformed inner nature and still preserve recognizable identity cues.

### Distinguish Alter from Awakened
**Alter:** a shift/inversion/contrary interpretation of a character's *nature or expression*. **Awakened:** realization/extension of the character's *latent potential*. A dramatic visual change can happen in either; the story/psychology and power premise decide, not hair color.

## Awakened — substantial character redesign standard (refined after visual review 2026-10-16)

**Important distinction:** Milim's Stampede form, previously provided by the user, belongs to the **Berserker** Version category and **must not be used as an Awakened classification**. It can illustrate the magnitude of character-centric visual redesign across form types, but its premise is different. Awakened does **not** mean automatically angelic, holy, lighter, divinely armored or regal.

**User correction:** A drastic transformation can be accomplished through **hair color gradients, contrasting strands, new hairstyle, hair length and volume, changed clothes, altered silhouette, eyes, expression, markings, proportions, ornamentation and power-specific body traits**. Physical anatomy mutations are **optional**, not required. Do not impose wings, horns, claws, tattoos, excessive armor or any other generic trope.

### Practical design sequence

1. **Identify the original recognition anchors.** Decide what must remain unmistakably the same character: face, hair motifs, distinctive palette cues, silhouette themes, emotional signature, powers, weapon, etc.
2. **Choose a character-specific amplification premise.** Intensify the existing identity rather than shift its underlying nature (that is the conceptual difference from Alter). The awakened form should be unmistakably distinct even without flashy surroundings.
3. **Redesign the subject across several visible axes:** e.g., add dramatic gradient or colored streaks to signature hair; change its shape, parting, length and motion; develop iris and eye expression; change body language and markings; evolve garment architecture, outfit shape, layering and accessories; change weapon form; optionally alter anatomy where motivated. These are **all valid and potentially central design changes**.
4. **Balance recognizability and transformation.** Preservation of visual anchors does not imply identical hairstyle, clothing or body shape. Keep a coherent through-line while making the evolved form strikingly different at a glance.
5. **Validate with a neutral-background, character-only image.** No effects required; the face, hair, clothing, silhouette and signature design should already communicate a distinct Awakened form. Do not strip the clothing out of the comparison: **clothing is part of the character design**, not mere background. Optionally check an isolated silhouette and head/upper-body detail.
6. **Add VFX/composition after the core redesign.** Auras, particles, magic circles, banners and glowing geometry may strengthen the result but must not substitute for changing the character herself.
7. **Evaluate meaningful difference against Base.** Compare hair palette/shape, eyes/face, outfit and clothing shape, silhouette, body/posture, motifs, character-specific equipment and power presentation. No minimum anatomy mutation count or mandatory color palette.

**Gate 01 visual QA:** REJECT when the awakened character is essentially the original face, hair and clothes plus a new staff, halo, background or particle effects. DO NOT reject simply because a legitimate character-specific transformation uses hairstyle, gradient/colored strands, wardrobe or silhouette changes rather than transformed anatomy. A single trivial recolor or costume swap without any broader awakened rationale may still be a Skin.

**Current visual review status:** prior officer-mage Awakened renderings are unapproved; they overrelied on decorative effects or inadequately distinguished the character. Future art must be judged against the full redesigned subject, not just VFX. This rule applies across all three Gate 01 character workflows and changes no existing Registry record.

## Berserker — separate character Version family

**Core premise:** the character is driven into a power-overflow, frenzy, feral, rage, loss-of-restraint or otherwise destructively unleashed condition. The original person remains recognizable; the transformation expresses uncontrolled or extreme instinct/power rather than ordinary mastery.

**Authoritative visual reference supplied by the user:** Milim's Stampede/berserker form relative to her Base. The example belongs **here**, not under Awakened.

**Character-led visual evolution:** dramatically change facial intensity and expressions, hairstyle/wildness and color patterns, body language, stance, costume structure/damage or integrated armor, power-linked body markings, aggressive silhouette, and character-specific anatomy if justified. No mandatory horns, claws, wings, darkness, gendered styling, color, or moral alignment; do not treat generic external fire/aura as a sufficient design.

**Difference from Awakened:** Awakened shows amplified/developed potential that may remain deliberate, refined or controlled. Berserker conveys **loss of restraint or runaway destructive escalation**; any regained control or stable Berserker form requires explicit character-specific lore. Both can have extreme visual redesigns, but their transformation causes and behavior are different.

**Difference from Alter:** Alter changes/inverts/reinterprets a character's fundamental nature or identity expression, often with darker/bolder presentation. Berserker centers on the character's **unleashed frenzy or overwhelming power state**. Do not conflate just because both look intimidating.

**Acceptance test:** On a neutral background, with background effects hidden, the actual character's hair, face, clothing, posture, silhouette and characteristic traits should communicate the berserker transformation. Define control state, transformation trigger, reversibility and combat behavior before registration. No automatic rarity or power ranking.

## Reusable Gate 01 creation protocol (all characters)

1. **Locate authentic source.** For existing characters inspect user-supplied 2D art or original 3D with live Koikatsu capture, original native card, textures/rig and hashes. Preserve the original; never replace by approximation.
2. **Compare against Base.** Log deliberate and accidental differences: face/eyes/hair/body proportions, anatomy, skin/materials, outfit, accessories, silhouette, power themes and character recognizability.
3. **Classify accurately.** Different costume/hair/palette alone => Skin. Meaningful independent change => propose a Version and choose an explicitly authorized type (Base, Alter, Awakened, Berserker). Temporary mechanics may be state changes; don't invent automatic Version ownership.
4. **Specify creative cause** in plain language: what changed, why, how the new form still relates to the Identity, permanence/reversibility and intended playstyle distinction.
5. **Produce accurate, editable visual reference.** Exact 2D turnaround (front/side/back), expression sheet, palette/annotations, outfit/accessory details and finished standalone artworks, consistent with premium hand-drawn anime style and the game's existing art direction.
6. **3D-first exception.** If the user has already made the Version in Koikatsu, derive 2D material FROM observed real 3D first; distinguish faithful reproduction from proposed repairs; ask for approval before modifying user 3D or freezing the 2D target.
7. **For newly approved designs** follow the original v6 2D freeze before new 3D creation; use learned Koikatsu Party -> KKBP -> Blender -> Unreal automation (preserve mappings, corrections, export fidelity); retain distinct state cards where applicable and genuine source hashes.
8. **Coordinate specialists:** Gate 04 kits/abilities, Gate 09 animation/expressive movement, Gate 10 rarity/presentation, Gate 11 Registry v2 authority, Gate 12 Unreal/Android integration; Gate 00 creative approval. No fabricated IDs, awards or passes.
9. **Respect three independent entry workflows:** Original Character Creation, Feminisation, Existing Character Adaptation. Never substitute this Version classification for the entry choice.

## Minimal proposal fields

- Verified Identity / Base source references and hashes
- Version type (Base, Alter, Awakened or Berserker); avoid assuming user meant runtime owned Manifestation
- Cause of divergence / awakening / berserker escalation and Identity continuity invariants
- Diff table: psychology, anatomy, face, outfit, power, presentation, gameplay role
- Confirmed user-authored source versus artist-suggested correction
- Actual 2D exact-design approval and 3D fidelity approval, each independently recorded
- Gate 04/09/10/11/12 handoff statuses, recorded as pending until evidenced

*This revision supersedes the unapproved speculative list in the first 2026-10-16 draft. Those names have been removed rather than added as official game types.*
