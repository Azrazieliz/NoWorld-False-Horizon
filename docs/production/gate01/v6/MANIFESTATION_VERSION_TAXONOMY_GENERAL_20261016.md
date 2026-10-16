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
| **Alter** | A meaningfully divergent interpretation or developmental route *of the same Identity*. The divergence is explicitly characterized in lore/psychology/powers and translated into the visual design. | Not solely an outfit, recolor or haircut; substantively different version premise. |
| **Awakened** | A character who realizes previously latent potential, abilities, state or understanding through a defined trigger and progression. | Cause and resulting power/physical/behavioral changes are specified; not simply brighter effects or recoloring. |

**No additional Version-type names are authorized by this document.** Existing independently approved content rules elsewhere in the game remain untouched. New category proposals stay OUT of the source taxonomy unless the user approves them.

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
