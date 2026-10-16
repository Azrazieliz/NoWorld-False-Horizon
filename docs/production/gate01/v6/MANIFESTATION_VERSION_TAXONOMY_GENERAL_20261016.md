# Gate 01 — Version Authoring Taxonomy (restricted, 2026-10-16)

**Status:** Working Gate 01 authoring methodology, aligned with the existing Character Identity architecture. **Only the three types explicitly identified by the user are enumerated.** This document is not a Registry v2 approval or a change to frozen game architecture, fixed rarity, combat kit, owner data, or Foundation.

## Existing architecture — authoritative

- **Identity:** the canonical person/character; can have multiple Versions.
- **Version:** immutable authored form/content definition of that Identity; may have separate 2D art, 3D model, kit, animation and combat role.
- **Manifestation:** an independently owned, persistently progressed runtime entity/card pointing to Identity and an applicable Version. One Ruler may own multiple instances with different progress.
- **Skin:** cosmetic alternative without separate form/kit/ontology.
- **Appearance state:** CLOTHED / UNDERWEAR / NUDE, where applicable; these are not Versions, owned Manifestations or separately acquired characters.
- **Rarity:** independent system and Gate 10 authority; neither the Version type nor presentation automatically determines rarity.

These existing layers must not be renamed, merged or reinterpreted even if a conversation informally calls a Version a “manifestation.” Preserve existing Registry v2 and approval boundaries.

## Currently user-specified Version design types

| Type | Operational definition | Evidence that distinguishes it from a skin |
|---|---|---|
| **Base** | The primary / identity-anchor Version of a character. | Carries the agreed canonical starting depiction, powers and role. |
| **Alter** | An **inverted, darker, or fundamentally altered expression of the same Identity**, using *Fate* Alter Servants and *Date A Live* Inverse Spirits (Sephira inversion) as the user's intended reference family. A defining aspect of the character is inverted, distorted, suppressed or re-expressed: disposition, instincts, allegiance, values, emotional state, power aspect, symbolism, or an alternative interpretation of their essential nature. | The alteration must be **character-specific, substantive, and legible** in behavior and power identity as well as visual language, not merely clothing/hair/palette. It may be a distinct persona or a recognizably continuous self; not every Alter is evil, mindless, corrupted or stronger. |
| **Awakened** | A character who realizes previously latent potential, abilities, state or understanding through a defined trigger and progression. | Cause and resulting power/physical/behavioral changes are specified; not simply brighter effects or recoloring. |

**No additional Version-type names are authorized by this document.** Existing independently approved content rules elsewhere in the game remain untouched. New category proposals stay OUT of the source taxonomy unless the user approves them.

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

## Reusable Gate 01 creation protocol (all characters)

1. **Locate authentic source.** For existing characters inspect user-supplied 2D art or original 3D with live Koikatsu capture, original native card, textures/rig and hashes. Preserve the original; never replace by approximation.
2. **Compare against Base.** Log deliberate and accidental differences: face/eyes/hair/body proportions, anatomy, skin/materials, outfit, accessories, silhouette, power themes and character recognizability.
3. **Classify accurately.** Different costume/hair/palette alone => Skin. Meaningful independent change => propose a Version and choose an explicitly authorized type (Base, Alter, Awakened). Temporary mechanics may be state changes; don't invent automatic Version ownership.
4. **Specify creative cause** in plain language: what changed, why, how the new form still relates to the Identity, permanence/reversibility and intended playstyle distinction.
5. **Produce accurate, editable visual reference.** Exact 2D turnaround (front/side/back), expression sheet, palette/annotations, outfit/accessory details and finished standalone artworks, consistent with premium hand-drawn anime style and the game's existing art direction.
6. **3D-first exception.** If the user has already made the Version in Koikatsu, derive 2D material FROM observed real 3D first; distinguish faithful reproduction from proposed repairs; ask for approval before modifying user 3D or freezing the 2D target.
7. **For newly approved designs** follow the original v6 2D freeze before new 3D creation; use learned Koikatsu Party -> KKBP -> Blender -> Unreal automation (preserve mappings, corrections, export fidelity); retain distinct state cards where applicable and genuine source hashes.
8. **Coordinate specialists:** Gate 04 kits/abilities, Gate 09 animation/expressive movement, Gate 10 rarity/presentation, Gate 11 Registry v2 authority, Gate 12 Unreal/Android integration; Gate 00 creative approval. No fabricated IDs, awards or passes.
9. **Respect three independent entry workflows:** Original Character Creation, Feminisation, Existing Character Adaptation. Never substitute this Version classification for the entry choice.

## Minimal proposal fields

- Verified Identity / Base source references and hashes
- Version type (Base, Alter or Awakened); avoid assuming user meant runtime owned Manifestation
- Cause of divergence / awakening and Identity continuity invariants
- Diff table: psychology, anatomy, face, outfit, power, presentation, gameplay role
- Confirmed user-authored source versus artist-suggested correction
- Actual 2D exact-design approval and 3D fidelity approval, each independently recorded
- Gate 04/09/10/11/12 handoff statuses, recorded as pending until evidenced

*This revision supersedes the unapproved speculative list in the first 2026-10-16 draft. Those names have been removed rather than added as official game types.*
