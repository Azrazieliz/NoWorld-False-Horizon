# v0.40 historical master — source extraction 9/10
Source: Offline_Adult_Gacha_RPG_Master_Architecture_Baseline_v0.40_FINAL_PRECODING_FREEZE_RELEASE.docx, 175 pages; uploaded 2026-11-01. Parsed original text lines 7201–8100. Historical source only: newer explicit instructions, verified latest code decisions and v0.41 authoritative narrow addendum `docs/MATURE_CONTENT_V041_PRODUCTION_FREEZE.md` and later explicit user decisions may supersede conflicting clauses. Original DOCX remains higher-fidelity for embedded images and precise tables.

   Character Version still exposes bSexualContentEligible; this is an obsolete second adult-access gate
    and must be replaced by descriptive profile/asset references.
   Current vertical slice pulls before Territory exists; frozen design requires continuous Territory
    control for more than one in-game month before permanent gacha unlock, then first World-Mode
    anchoring in controlled Territory.
   Action combat enforces local Character Identity exclusivity, while turn combat currently does not;
    both must use one shared rule with explicit mechanic overrides.
   Current startup applies migrations before creating its rotating snapshot; the untouched pre-
    migration history is therefore not guaranteed if a future migration fails.

                                                 Page 152

<PARSED TEXT FOR PAGE: 153 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AY.4 Numbered migration plan
0007 - Character Manifestation instances / acquisition history
   Stop gameplay use of duplicate_acquisition_count; preserve the legacy column only as migration
    provenance.
   Add acquisition tick/ordinal, source pull event, first World-Mode anchor Territory/tick, lifecycle state
    and build label to Manifestations.
   Fan out legacy duplicate counters deterministically into distinct Manifestations, using recorded
    gacha events where available.
   Replace single owner+Identity lookup with list APIs; every qualifying character pull creates a new
    Manifestation.

0008 - Territory claims / reclamation / sovereignty / gacha access
   Add many-to-many Location-Territory membership so overlapping physical control is representable.
   Add persistent Territory claims with control strength/state, displacement tick and five-day reclaim
    deadline.
   Add Ruler sovereignty state with current/historical peak title; only Ruler and Overlord are universal
    titles.
   Add one-month continuous-control gacha qualification with suspension during reclaim grace and
    permanent unlock once achieved.
   Use Manifestation anchor state to prevent first World-Mode deployment before controlled-Territory
    anchoring.

0009 - Domain heart / Core fusion lineage
   Add Territory Domain-heart state separate from physical Territory damage.
   Preserve zero-durability -> Broken and Broken cannot be normally captured.
   Add normalized Core Concepts, fusion events, asymmetric absorber/absorbed order, synthesis rule,
    deterministic seed, instability state and persistent lineage.
   Protected Core + devastated Territory remains repairable/Domain-functional; lost Core triggers
    Domain-heart ruin and exceptional reconstitution rather than ordinary repair.

0010 - Rank / Factors / Classes / route progression / Grand Convergence
   Add data-driven attained/effective/peak Existence Rank state with Levels 1-100.
   Add Factor instances/provenance/lineage with no active-slot disable; all acquired Factors remain
    causal.
   Add multi-Class state, Crown mastery and one active Grand seat per canonical Class.
   Add learned-skill provenance, open-ended route nodes, forms and reinforcement state.
   Add explicit Grand Convergence and source Manifestation records; source copies become
    historical/converged rather than deleted.

0011 - Reality nodes / World Rank / time domains / World Director
   Add Worlds/Dimensions/subrealms as reality nodes with World Rank, law profile and time domain.
   Add deterministic local time-domain ratios/calendars while preserving a monotonic canonical tick.
   Add Junction state and requirements.
   Add Director schedule/provenance, sealed-content release state and offline-catch-up governor state.

                                                  Page 153

<PARSED TEXT FOR PAGE: 154 / 175>

                                                       OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   The lore mechanism is semi-conscious in a metaphysical/homeostatic sense; runtime scheduling
    remains bounded and auditable.

0012 - Dispatch / war / army capabilities / Projects / civilization / logistics
   Dispatch gains primary/secondary objectives, hard constraints, abort policy, risk tolerance and
    richer outcomes.
   War becomes a persistent parent with fronts, changing objectives, orders and participant history.
   Army EffectivePower becomes only a projection; authoritative strategy uses capability vectors and
    named-character intervention.
   Projects retain lazy progression but gain optional phases/assignments for genuinely complex work.
   Civilization progression uses authored civilization-specific development dimensions rather than one
    modernization level.
   Logistics routes support real transport capabilities, including teleportation/dimensional travel when
    actually owned.

0013 - Items / knowledge / NPC promotion / mature state / Heroic Records
   Add physical item instances, body-specific equipment bindings, inventory containers and item
    modifiers/history state.
   Extend knowledge with belief state, confidence, provenance, rumor/language context and semantic
    memory.
   Add automatic NPC promotion state that preserves the same entity identity/history.
   Add mutable adult-content expression/profile state only; no eligible/consent/permission boolean.
   Add Heroic Records so sufficiently significant dead identities can become later gacha-accessible
    patterns without rewinding canonical death.

0014 - Packages / Reports / Android notifications / recovery metadata
   Add package installation URI, storage class, sealed/download state, compatibility and normalized
    dependency records.
   Dependency minimum versions and cycles must be validated before activation.
   Add Reports and delivery state; Android notifications are projections of Reports, never authoritative
    world state.
   Protected backup catalog lives outside the canonical DB and tracks URI/hash/schema/build/validation
    metadata.
   Clear World does not erase external backups unless explicitly requested.

AY.5 C++ contract changes
   Remove DuplicateAcquisitionCount from gameplay structs/results; repeat identity may remain only
    as presentation/history information.
   Remove FOGCharacterVersionDefinition::bSexualContentEligible. Replace it with descriptive adult-
    profile / presentation / scene-library references that never override Character Identity lore maturity.
   Keep EOGCanonicalMaturity on Character Identity as lore data only: Unknown / NonAdult / Adult.
   Create one shared local Character-Identity exclusivity validator used by both action and turn combat
    with explicit rule overrides.
   Add migration-safe world bootstrap, Territory control, sovereignty, gacha-access, Domain Core,
    Rank, Factor, Class, character progression, Grand Convergence, World time, World Director, strategic
    resolution, Reports/package services and Android notification bridge.
                                                    Page 154

<PARSED TEXT FOR PAGE: 155 / 175>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AY.6 Test rewrite contract - do not run yet
   Fix the stale Character Identity test contract before compile.
   Prove two same-Identity pulls create two persistent Manifestation IDs.
   Prove one-month Territory qualification, five-day reclamation behavior, permanent gacha unlock
    and first anchoring.
   Prove overlapping claims, Core-heart ruin, protected-Core reconstruction and asymmetric Core
    fusion provenance.
   Prove data-driven Rank, Factor persistence, Crown non-uniqueness, unique Grand seat, route state
    and Grand Convergence.
   Prove action/turn combat share Identity exclusivity and Rank hooks.
   Prove Director provenance/bounded delay/offline governor and objective-faithful
    Dispatch/continuous war.
   Prove lore-adult content references validate without appearance/Version permission gates; Privacy
    mode changes presentation only.
   Prove package dependency enforcement and migration-failure recovery preserve the untouched
    original DB.

AY.7 Validation sequence after reconciliation
9. 1. Static Unreal C++ preflight.
10. 2. UE 5.8 UHT/UBT/MSVC/link compile.
11. 3. OfflineGame automation tests.
12. 4. Android cook/package/install.
13. 5. Physical Samsung Galaxy S26 Ultra profiling.
14. 6. Optimize through streaming/tiering/abstraction while preserving the frozen player-facing design.




                                                Page 155

<PARSED TEXT FOR PAGE: 156 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AZ. Detailed Numerical Tuning and UI Freeze
STATUS: CURRENT AUTHORITATIVE PRE-AUDIO / PRE-ART-DIRECTION CHECKPOINT. This section
records the Creative Director's completed global numerical and UI decisions after the architecture freeze
and contradiction audit. It supersedes older OPEN/provisional wording on the same details. Music,
voice/SFX design and visual art-direction production are intentionally deferred to the next design passes.

AZ.1 Rank, Level and stat progression
   Keep the current stronger common Rank coefficient candidate: R(r) = 10^(0.40r + 0.040r^2), with
    Mortal r=0 and Primordial r=16.
   Target approximately 35% of each Rank step's logarithmic growth across Levels 1-100 and 65% at the
    actual breakthrough.
   Within-Rank Level progression is character-specific rather than universally linear. Each character's
    Level 1-100 curve is normalized to the shared 35% envelope.
   Character/stat-specific breakthrough bias may exceed the smooth Rank projection when the actual
    transformation, body, Factor, route or lore warrants it.
   Every character-facing stat remains character/Manifestation/build-specific. Rank contributes to
    growth, but individual stats do not all multiply by the same amount.
   Magnitude stats use a shared Rank scale plus a character/stat-specific response. The stat Rank-
    elasticity alpha is authored per character/stat within the previously frozen positive-decimal range
    1.1-2.5.
   Probability/rate/action-frequency stats such as Crit Rate, Hit, Dodge, Block and SPD remain
    character-specific and use stat-appropriate semantics rather than blindly treating the Rank
    coefficient as a percentage.
   Internal formulas, alpha values, Rank decomposition and implementation coefficients are never
    exposed in ordinary player UI.

AZ.2 DEF, damage and Rank Suppression
   Positive DEF keeps progressive ratio mitigation: multiplier = DefenseReference / (DefenseReference +
    effective DEF).
   The accepted generic negative-DEF candidate is exponential: for DEF < 0, multiplier = 2^(-DEF /
    DefenseReference). Skills/characters may author another base/reference when their mechanics
    explicitly require it.
   No universal damage ceiling is introduced. Extreme defense destruction, Crit, authored multipliers
    and high-order mechanics may create extremely large damage.
   Rank Suppression keeps the first general lower-to-higher interaction envelope around 100% / 100% /
    75% / 40% / 15% / 3% for gaps 0 / 1 / 2 / 3 / 4 / 5+, but actual values remain channel-specific.
   Separate Rank-Suppression channels include damage, effect/debuff penetration, control,
    perception/sensing, presence/environment tolerance and resistance breaking.
   Same/adjacent Rank combat remains ordinary mechanically; raw stat disparity can still be
    enormous because characters grow differently.

AZ.3 GP / Power
   GP/Power remains a visible summary estimate only and never directly determines combat outcomes.


                                                  Page 156

<PARSED TEXT FOR PAGE: 157 / 175>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   A raw logarithm must not be displayed as GP. A result such as '32 GP' beside quadrillion-scale stats
    would be misleading and is rejected.
   Engineering may combine values safely in logarithmic space internally to avoid overflow, but
    player-facing GP is converted back into the game's ordinary large-number/suffix presentation.
   GP may intentionally misestimate unusual, counter-based or highly synergistic kits; it is an overview
    number, not a truth oracle.
   Exact GP weighting/calibration remains deterministic simulation work.

AZ.4 Progression economy
   General scarcity gradient: acquisition < ordinary development < Current-Rarity promotion < high
    Rank breakthroughs < rare route completion < Transcendence < Grand Convergence.
   Individual characters, worlds and mechanics may legitimately violate that ordering when their
    causal content warrants it.
   There is no universal real-time target such as 'a Rank takes ten hours.' Progression is
    content/causality driven; internal time/cost bands exist only to detect balance outliers.

AZ.5 Gacha rates, income and tickets
   Current standard limited-banner simulation baseline is accepted: 1.2% base top-Origin-Rarity rate;
    soft pity after pull 50; +5 percentage points per pull from 51-69; hard pity 70; 50/50 featured then
    guaranteed; compatible pity/guarantee carry; ten-pull floor at least SR+ unless the banner defines a
    stronger floor.
   Gacha acquisition becomes much richer with actual progression. A level-1 wanderer/new Ruler and
    an Absolute/late-game Overlord do not have remotely equal income.
   Income scales because higher progression unlocks objectively richer content, sovereignty, Projects,
    challenges, dimensional sources and dedicated farming rather than through an invisible account-
    level reward multiplier.
   Revised acquisition-focused simulator bands: newly qualified Ruler about 4-8 pull-equivalents/hour;
    established Ruler/Domain about 7-14/hour; ordinary Overlord-scale economy about 14-27/hour;
    extreme late-game/high-order dedicated farming about 27-52+/hour where actual content justifies it.
   These bands include currency, tickets and burst rewards; they are not fixed wages or guaranteed
    hourly payouts.
   Ticket taxonomy remains data-defined and includes standard single-pull, multi-pull,
    event/dimensional/banner-compatible, guaranteed-rarity/restricted-pool and exceptional
    selector/designation tickets.
   Tickets normally do not expire. Banner/event tickets remain stored for compatible reruns unless
    their actual authored mechanic explicitly says otherwise.
   When compatible tickets and ordinary pull currency can both pay for a pull, tickets are
    automatically consumed first, then currency.
   Late-game content may reward better ticket quality, not merely larger quantities of basic currency.

AZ.6 Character roster UI - visual reference
   The roster is visual/card-first in portrait Ruler Mode.
   One main roster tile represents one Character Identity; owned Manifestations are grouped beneath
    that Identity instead of appearing as unrelated duplicate characters.
   Each card uses a rarity-specific border/frame.

                                                 Page 157

<PARSED TEXT FOR PAGE: 158 / 175>

                                                  OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Search is prominent; compact filter/sort controls remain available.
   Target default viewport density presents three visible roster rows rather than four.
   The user can switch roster density where appropriate.
   Filters: Rank, Classes, Rarity and World Fantasm grade. Sorting supports the same major dimensions
    where meaningful.
   Favorite and Protected/Locked are separate: Favorite is UI pin/filter; Protected blocks
    destructive/Convergence operations without explicit confirmation.
   Cards stay visually readable and do not become stat spreadsheets.




                                               Page 158

<PARSED TEXT FOR PAGE: 159 / 175>

  OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE




Page 159

<PARSED TEXT FOR PAGE: 160 / 175>

                                                            OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE
    Creative Director roster composition reference. Adaptation target: card-first portrait roster, rarity-framed cards,
     search/filter density, three visible roster rows. Branding/art shown here are reference only and are not copied.


AZ.7 Character Identity page - visual reference and navigation
   The full character presentation dominates the page. Identity/status/equipment/navigation
    information frames the character instead of replacing the visual presentation with a spreadsheet.
   Current display defaults to the last-used Manifestation, not an independently maintained universal
    'Preferred' copy.
   Major bottom tabs are visually separated as: Overview; Skills / World Fantasm; Equipment;
    Progression.
   Forms, Manifestations, History and Adult are separate upper/utility destinations, not additional
    bottom tabs.
   Secondary utilities such as wardrobe/skins may live as contextual/side controls rather than
    consuming a major bottom tab.
   Manifestations show editable build label, Rank/Level, Current Rarity, major route/form state,
    relevant equipment and assignment/deployment state.
   Two Manifestations can be selected for side-by-side comparison of resolved stats, routes, skills and
    equipment.
   The UI never replaces the user's active copy with an automatically calculated 'strongest' copy.




                                                         Page 160

<PARSED TEXT FOR PAGE: 161 / 175>

  OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE




Page 161

<PARSED TEXT FOR PAGE: 162 / 175>

                                                         OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE
Creative Director character-detail composition reference. Adaptation target: character art dominates the screen while
 compact identity information, equipment/status and navigation frame it. Exact art, branding and ornamentation are
                                                    reference only.


AZ.8 Character stats, progression, routes and skills UI
   Default stat presentation is compact and player-facing.
   A detailed stat view may expose more resolved values and comparisons, but never internal
    formulas, alpha factors, Rank-contribution decomposition or hidden implementation coefficients.
   Progression/routes use a zoomable graph/tree where appropriate. Unknown/secret route nodes
    remain hidden or knowledge-gated.
   Characters may eventually possess 1000+ learned techniques. The main character UI must not dump
    an exhaustive giant skill list.
   Skills / World Fantasm focuses on the resolved combat kit, relevant integrated techniques, current
    field/Fantasm rules and manageable grouped/searchable subsets when needed.
   Meaningful History includes acquisition, breakthroughs, route choices, major equipment, Grand
    Convergence provenance and important world/story events; trivial battle spam is omitted.

AZ.9 Skins, outfits and physical clothing state
   Skins/outfits are added as a first-class presentation system.
   A skin is not automatically a new Character Identity, Version, form or power state.
   Skins may be cosmetic unless their authored content explicitly carries mechanics.
   Skin compatibility can depend on Character Identity, Version, body plan and current form.
   Wardrobe/skin access is contextual to the character page and does not consume a major bottom tab.
   Archive/replay can filter by outfit/skin.
   Compatible visible equipment remains present over/with the selected outfit when the actual
    equipment state says it is worn.
   Clothing/equipment is physically layered. Removing every removable clothing/equipment layer
    exposes the actual underlying body state instead of forcing an artificial permanent fallback outfit.

AZ.10 Mature/adult character UI
   For every canonically lore-adult character, Adult is an always-accessible utility destination from the
    Character Identity page.
   World/contextual adult interactions still exist independently; the character UI also allows direct
    interaction where the relevant state/content supports it.
   Adult may expose Profile, direct interaction, available systemic interactions and Archive-Replay
    rather than only a flat scene list.
   Libido/preferences use both descriptive presentation and exact values when detailed numeric
    inspection is genuinely useful.
   Contextual world prompts remain natural/subtle rather than giant explicit HUD buttons.
   Archive/replay supports historical/current appearance and filtering by participant, Version/form,
    outfit/skin and location.
   Replay remembers the last presentation preference.
   A fast Privacy/SFW Presentation toggle is accessible from a convenient pause/profile surface as well
    as Settings.



                                                      Page 162

<PARSED TEXT FOR PAGE: 163 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AZ.11 Equipment, affinity, proficiency and crafting UI
   Equipment UI follows the character's actual body/slot schema; it does not force every character
    through Helmet/Chest/Boots slots.
   The character remains visually central while equipped pieces are presented around/alongside the
    character where appropriate.
   Affinity and proficiency are separate systems.
   Affinity may use a continuous internal value plus authored milestones. Player-facing presentation
    defaults to grades/stages and milestone state; numeric values are shown only where useful.
   Proficiency likewise defaults to grades; raw numeric values appear only where they provide useful
    information.
   Affinity never means one universal '+X% damage per point' rule. Each item's actual mechanics
    decide what affinity improves, unlocks or evolves.
   A character may have high proficiency with a newly acquired low-affinity item.
   Equipment comparison shows useful resolved stat deltas, gained/lost skills/functions, compatibility
    warnings, Item Rank/Quality/Evolution, affinity/proficiency implications and relevant history.
   Historically significant/named items have Chronicle-style history views.
   Crafting exposes Known mode for established recipes/processes and Experiment mode for genuine
    invention with materials, facility/technique, intended function, appearance and desired properties.

AZ.12 Gacha UI
   Banner presentation prioritizes the featured character/Version.
   Banner state/time/context, pull currency, compatible tickets, pity count and featured-guarantee state
    are directly visible.
   Pull controls are accompanied by Details and History access.
   A Details icon exposes exact declared probabilities, soft/hard pity behavior, eligible Identity/Version
    pool, carry category and special ticket/pool rules.
   Pull history persists and can filter by banner, Identity, rarity and date.
   Multi-feature banners may expose an authored target/designation selector where that banner's rules
    support it.
   No mockup image is required at this checkpoint.

AZ.13 Territory / Domain strategic UI
   The Territory top-level destination is map-first.
   Selecting a Territory opens a detail/bottom sheet with quick access to Territory, Domain/Core,
    Dispatch, Projects, Armies/War and Logistics.
   Territory and Domain remain visually and semantically distinct concepts.
   Selecting Territory can expose linked Domain/Core contextually, while a dedicated Domain list
    remains useful at large sovereignty scale.
   Large sovereignty navigation is hierarchical or fragmented where more usable: Reality/Dimension ->
    World -> Region/Territory rather than forcing all cosmology onto one giant map.
   Major analytical overlays such as control, logistics, threat, resources, Projects and Dispatch are tap-
    to-enable and normally one major overlay is active at a time.




                                                 Page 163

<PARSED TEXT FOR PAGE: 164 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AZ.14 Records UI
   Records always returns to the general Records chooser/hub rather than reopening the last internal
    tab.
   General chooser exposes Reports, Chronicle, Codex and Intelligence.
   Chronicle is timeline-first and filterable by character, faction, Territory, World, event type and
    importance; trivial events are omitted.
   Intelligence visibly distinguishes Confirmed, Estimated, Rumor, Contradicted and Outdated
    information, with source/provenance shown when known.
   Codex uses searchable categories for Character Identities/observed Manifestations, creatures/species,
    Worlds/locations, powers/effects, items, factions and other entity families.

AZ.15 World Mode HUD and combat presentation
   Immediate party remains protagonist/Ruler plus up to two selected companions.
   Companion switch portraits/status are positioned on the upper-right side of the screen in the general
    HI3-like placement philosophy, not beside the lower-right combat buttons.
   Controlled character receives the main HP/resource presentation.
   Enemy/boss HUD uses one principal target panel with HP/state/status/phase/resource detail only
    when knowledge permits; additional targets use smaller secondary indicators.
   Status effects use compact icon + stack/turn presentation. Tapping opens precise effect information;
    holding is not required.

AZ.16 Large-number player presentation
   Player-facing numbers do not use scientific notation or x10^n notation.
   Use ordinary integer/suffix notation, e.g. 6B = 6 billion, and extend the suffix system to larger
    magnitudes.
   Character/stat screens have no fixed ten-character ceiling. Values can use the full screen width; do
    not solve length by shrinking typography to an absurd size.
   Floating combat values target roughly ten visible characters maximum.
   If knowledge does not permit a value, show Unknown.
   If the compact battle display range is exceeded, show Exceed rather than scientific notation.
   For enormous logical hit counts, do not show 'value x N'. Resolve the full logical hit sequence but
    show only a few representative integer/suffix damage triggers visually.

AZ.17 Turn battle UI, auto and recap
   Add 3x speed beside the established 1x and 2x modes.
   Ultimate/cinematic presentation remains independently configurable.
   Timeline is an upcoming-action strip showing future actors/events and visible action advances,
    delays and interrupt insertions.
   Auto provides quick presets plus an advanced per-team conditional rule editor.
   Normal player-facing recap is intentionally simple per character: direct/active damage dealt; DoT
    damage dealt; total healing/restoration delivered by any means, including direct heal, regeneration
    contribution and vampirism.
   Do not clutter the normal recap with shielding, damage prevented, damage taken, crit analytics or
    excessive micro-source breakdowns. Engineering diagnostics may retain deeper data.


                                                 Page 164

<PARSED TEXT FOR PAGE: 165 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AZ.18 Opening, save recovery and package UI
   The game has a real title/opening screen rather than booting directly into a utility menu.
   Final opening presentation target is an animated/moving premium title presentation, with a
    lightweight reduced-motion/static fallback. It must provide a fast path to Continue and may be
    replaced/update-packaged without changing save architecture.
   Primary opening actions: Continue prominently; Recover Existing World; Import Backup; Settings.
   Clear World is secondary/destructive and requires deliberate confirmation.
   Backup manager shows automatic/manual snapshots, date/time, world/schema/build information
    and integrity/validation state, with explicit Export/Import/Restore actions.
   Package/storage manager shows installed World/region/character/media packages, size, version,
    storage location, update state and move/archive controls where Android/storage rules permit.
   Metadata/update checks may occur automatically online.
   Large package downloads default to automatic download on Wi-Fi/unmetered connection.

AZ.19 Projects, Dispatch, war and worker abstraction
   Project duration remains causal rather than one universal construction/research coefficient.
   There are no generic worker pawns as a separate micromanagement system.
   Project duration derives from workload plus relevant aggregated capacity/population where
    applicable, infrastructure, facilities, resources, assigned named specialists/characters where
    meaningful, techniques, Authority, automation, environment and other causal capabilities.
   Dispatch remains capability/knowledge driven rather than a fake flat success percentage.
   Dispatch UI shows qualitative risk/ranges unless the player's actual information justifies exact
    probability.
   War similarly uses capability vectors, counters, logistics and intelligence; displayed outcome
    confidence respects known information.

AZ.20 Offline consequence governor and World Director cadence
   The approximately 15% sovereign strategic-weight figure is a maximum budget/ceiling for newly
    generated irreversible offline loss under normal protection rules, not a scheduled loss.
   Reconnecting does not imply losing 15% of Territory. Most catch-up intervals may produce zero
    permanent Territory loss.
   Consequences already causally locked before logout remain exempt when the prior world state
    genuinely made them unavoidable.
   World Director uses no single global event-every-N-hours cadence.
   Each Director content family defines eligibility, earliest start, preferred window, latest
    bound/cooldown and world-state relevance.
   Bounded quiet-period logic prevents absurd droughts without fabricating prerequisites.

AZ.21 World/local time presentation
   Earth-like/familiar day-night pacing is not universal.
   Use familiar pacing only for Territories/Worlds whose authored cosmology/environment actually
    supports it.
   Other Worlds/Territories may use radically different day lengths, calendars, light cycles, time ratios
    or no meaningful conventional day/night cycle.


                                                  Page 165

<PARSED TEXT FOR PAGE: 166 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AZ.22 Reference-image interpretation rule
   The two Creative Director screenshots in this checkpoint are composition/layout references only.
   The project should adapt the functional presentation principles - visual-card roster, rarity framing,
    character-art-first detail page, surrounding compact controls - to its own systems and branding.
   Do not copy the reference game's proprietary character art, branding, labels, ornamental frames or
    exact visual assets.

AZ.23 Closed questionnaire / next passes
   The global numerical and detailed UI questionnaire is closed and remains authoritative for
    implementation.
   Do not re-ask decisions frozen in this checkpoint unless the Creative Director explicitly revises them.
   Remaining numerical work is deterministic simulator/engineering calibration, not another broad
    preference questionnaire.
   Music / voice / SFX and visual art / VFX / animation / cinematic production-direction passes are
    complete in checkpoints BB and BC.
   All finite pre-coding architecture and production-direction work is complete. Proceed to reconciled
    schema/C++ implementation, deterministic simulator fitting, mandatory Unreal/Android validation
    and physical-device calibration.




                                                 Page 166

<PARSED TEXT FOR PAGE: 167 / 175>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

BA. Independent Full Pre-Coding Architecture Audit and
Contract Closure
STATUS: CURRENT AUTHORITATIVE PRE-CODING AUDIT CLOSURE. This checkpoint was produced by
independently re-auditing the complete v0.39 cumulative master, all current dedicated architecture
documents, inherited implementation-era specifications, the reconciliation matrix and relevant existing
C++/SQLite contracts rather than relying on previous claims that the architecture was already frozen.

BA.1 Audit verdict
   No remaining major Creative Director architecture questionnaire is required before coding.
   All player-facing system architecture needed for runtime reconciliation is sufficiently specified.
   Remaining numerical work is deterministic simulation/content/device tuning, not missing creative
    architecture.
   Remaining character, World, story, item, boss, music-track and asset catalogs are ongoing content
    production, not finite pre-coding architecture.
   Audio/music/voice/SFX and visual/VFX/animation/cinematic production direction are completed in
    checkpoints BB and BC.

BA.2 Missing persistence contracts found and closed
   Transcendence can no longer disappear into opaque progression JSON. Persistent higher-order
    progression records the selected grade, irreversible breakthrough tick, qualification/proof
    provenance and state required for deterministic history/explanation.
   Each owned Manifestation can persist its World Fantasm unlocked/evolved state. Base grade
    remains consistent with immutable Origin Rarity; later Transcendence/routes/content may evolve
    scale, efficiency, rules, counters or presentation without silently changing that base grade.
   The protagonist's personal World Manifestation receives explicit persistent state and provenance. It
    remains intrinsic and independent of Territory/Domain Authority; losing Territory never deletes it.
   Combined Domain + personal-World-Manifestation techniques use ordinary skill/effect provenance
    referencing both sources. There is no automatic generic fusion or x2 stacking state.
   Item-owner affinity receives explicit persistent owner-item state and remains distinct from
    equipment proficiency.
   Equipment proficiency receives its own authored proficiency identity/state and is not forced to be
    per-item.
   Skins/outfits/presentation state can persist without creating fake Character Identities, Versions or
    forms.
   Favorite, Protected/Locked, build labels and persistent last-used Manifestation selections are
    management metadata linked to the canonical world where necessary. Protected/Locked state must
    survive restart, backup/restore and device migration.
   Privacy/SFW Presentation, orientation lock, roster density, audio levels, haptics, control layout,
    reduced motion/camera shake, damage-number presentation, cinematic repetition settings and
    network/download preferences are non-authoritative profile/device settings. They may be
    exported/backed up but never rewrite canonical world state.
   Auto-combat presets/team policies are player-command preferences unless an actual queued/offline
    action has already been committed; committed world actions store the resolved command, not a live
    dependency on later preference changes.

                                                Page 167

<PARSED TEXT FOR PAGE: 168 / 175>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

BA.3 Stale inherited specifications normalized
   The old Rule-Core Explicitly Deferred list no longer reopens settled damage, Crit, Hit/Dodge,
    cooldown/readiness or Ultimate-resource architecture. Only actual simulator/content values remain
    tunable.
   Location truth no longer contains a universal authoritative physically-accessible progression gate.
    Traversal/access is a causal query over location, route, environment, world law, body/abilities,
    equipment, infrastructure, knowledge and explicit mechanics.
   Duplicated pre-freeze detailed UI blocks are removed from the authoritative repository document;
    the completed Tuning/UI freeze controls detailed implementation.
   The title/opening presentation deferral is closed: animated/moving premium presentation is the
    default production target, with reduced-motion/static fallback.

BA.4 What remains deliberately unfinished
   Individual character kits, route nodes, visuals, voices and mature-content assets.
   Individual World Fantasms, Domains/Concepts, bosses, equipment, materials, NPC pools, factions
    and locations.
   Starting-world exact generated content catalog and later-World content catalogs.
   Opening/major narrative arcs, hidden lore, named endgame threats and spoiler-protected
    revelations.
   Individual compositions, performances, final audio mixes and per-World instrumentation catalogs.
   Exact gacha/economy/combat/war/project/dispatch coefficients that are explicitly simulator/content
    tuned.
   S26 Ultra actor/VFX/texture/HLOD/streaming/thermal budgets that require physical profiling.
   Final public game name/logo/icon and similar identity assets.
These are ongoing content production or evidence-driven calibration. They must not be mistaken for
missing architecture or used to restart broad design questionnaires.


BB. Audio / Music / Voice / SFX Production-Direction Freeze
STATUS: AUTHORITATIVE PRODUCTION-DIRECTION FREEZE. This checkpoint defines the player-facing
audio language and production contract without pretending individual tracks, performances or
physical-device mix values are already authored.

BB.1 Core audio identity
   Audio should feel like a premium character-action RPG inhabiting a persistent, physically coherent
    world rather than a menu-driven mobile gacha.
   The common identity is production quality, readable mix hierarchy, motif continuity and coherent
    systemic rules, not one universal musical genre imposed on every World.
   Character-specific anime action readability, cinematic high-order impact, environmental physicality
    and World/civilization-specific music are equally important.




                                                Page 168

<PARSED TEXT FOR PAGE: 169 / 175>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

BB.2 World/civilization music profiles
   Each major World defines instrumentation families, harmonic/melodic vocabulary, rhythmic
    language, ambience/noise palette, acoustic/electronic balance, cultural/metaphysical constraints and
    permitted hybridization rules.
   The historical-medieval starting World remains grounded in local cultures/materials. Later magical,
    post-apocalyptic, cybernetic, cosmic or abnormal-physics Worlds may change production radically
    while preserving motif continuity where history justifies it.
   Civilizations do not converge to one global soundtrack merely because they advance.

BB.3 Leitmotifs and adaptive music
   Important characters, factions, Worlds, Domains, World Fantasms and recurring concepts may own
    motif identities.
   Motifs may fragment, invert, reharmonize, change meter/tempo/instrumentation, corrupt/stabilize,
    become diegetic, combine contrapuntally or deliberately disappear.
   Adaptive states may include safe exploration, danger, ordinary combat, boss escalation, phase
    transitions, victory/recovery, Domain/World-Fantasm activation, overlapping-field conflict,
    catastrophic world events and Ruler-Mode strategic states.
   Transitions should preserve musical continuity when practical instead of restarting tracks on every
    state change.
   High-order reality effects may dominate/deform the current music instead of always replacing it
    with an unrelated track.

BB.4 Domain / World Fantasm audio
   Reality fields alter sound according to actual rules through music-stem dominance, filtering,
    space/reverb, propagation, directionality, pitch/time distortion, environmental transformation, motif
    takeover or selective silence as appropriate.
   There is no universal World-Fantasm sound or Rank aura sound.
   Overlapping fields follow mechanical interaction: boundaries, dominance pockets, mixed zones,
    fracture and negation may all produce distinct authored sonic results.

BB.5 Voice strategy
   Primary authored spoken language: Japanese.
   Major playable characters and major story roles target full or near-full coverage.
   Important promoted NPCs can receive bespoke upgrades when their persistent importance warrants
    it.
   Ordinary procedural NPCs use reusable voice families, contextual barks and partial coverage; family
    design may vary by species/anatomy, culture/region, vocal physiology, register and temperament.
   Player-selected/free-text ordinary protagonist dialogue is normally unvoiced; authored key
    protagonist lines/barks may be voiced.
   Combat voice uses multiple variants plus throttling/cooldowns so repeated actions do not become
    exhausting.
   Critical information never depends on voice alone.




                                                Page 169

<PARSED TEXT FOR PAGE: 170 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

BB.6 SFX grammar and combat hierarchy
   SFX is causal/material specific: weapon/contact, body/material, movement/traversal,
    guard/parry/perfect-dodge, skill resources, UI, environment/weather, machinery, magic/energy,
    Authority/Domain/Fantasm and destruction/aftermath families.
   The same nominal action can sound radically different with different anatomy, material,
    environment or reality law.
   High power does not automatically mean louder/denser. Law-level effects may become cleaner,
    quieter or unnaturally sparse.
   Mix priority: critical telegraphs/defensive cues, controlled-character confirmation, lethal/major
    enemy actions, important party/QTE/Ultimate cues, ordinary impacts, then non-critical ambience.
   Perfect dodge, parry, Ultimate readiness and major field collisions receive recognizable signatures.

BB.7 Spatial/world audio and Ruler Mode
   World Mode remains spatial/physical across interior/exterior transitions, occlusion, distance,
    terrain/material, underwater, vacuum/low-medium environments, caves/halls, weather, enormous
    entities, distant warfare and dimensional anomalies.
   World laws may explicitly override ordinary acoustics; those are authored mechanics, not random
    DSP decoration.
   Ruler Mode is lower-density and character/lobby focused, with subtle world-state adaptation,
    restrained UI feedback and Reports/urgent-event cues rather than generic dashboard audio.
   Mode switching preserves thematic continuity while allowing expensive 3D/spatial audio systems to
    unload.

BB.8 UI audio, accessibility and haptics
   UI audio has concise semantic families for navigation, confirm, cancel, destructive confirmation,
    acquisition, progression breakthrough, invalid/error, urgent Report and package/recovery state.
   Player controls include master/music/voice/SFX/ambience, optional dynamic-range profile and
    haptics intensity/off where supported.
   Subtitles/captions cover meaningful speech and critical non-speech cues. Muted play remains
    mechanically viable through visual/haptic redundancy.
   Distinct haptic families may accompany parry, perfect dodge, heavy hit, Ultimate, summon, field
    collision and major destruction, but haptics are never required to understand mechanics.

BB.9 Production/runtime contract
   Master assets remain high quality and are transcoded/streamed for Android through validated
    content packages.
   Gameplay requests stable semantic audio IDs/events/states rather than hard-coded file names.
   Adaptive cues, motifs, language variants and package dependencies remain replaceable/updateable
    without save migration.
   Exact loudness, codec, compression, streaming-memory and device budgets remain
    engineering/device-profiled values.




                                                 Page 170

<PARSED TEXT FOR PAGE: 171 / 175>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

BC. Visual Art / VFX / Animation / Cinematic Production-
Direction Freeze
STATUS: AUTHORITATIVE PRODUCTION-DIRECTION FREEZE. The common visual baseline is a high-end
anime/PBR hybrid with enough physical grounding for equipment, injury, weather, destruction and
radically different Worlds, without forcing photorealism or flat universal toon shading.

BC.1 Character identity and materials
   Character identity overrides one-body-template production. Height, proportions, musculature,
    species/anatomy, wings/tails/horns, mechanical parts, material composition and silhouette may vary
    materially.
   Shared rigs/retargeting are tools, not visual restrictions. Nonstandard anatomy receives appropriate
    rigs/animation when humanoid reuse would damage identity.
   Faces use anime-informed proportions with high-quality deformation/expression. Skin is stylized but
    physically responsive; eyes retain strong anime readability and depth; hair prioritizes authored
    clump/silhouette design before secondary strand detail.
   PBR response remains material-specific and stylized. Metal, cloth, leather, crystal, bone, scales,
    machinery, magical matter and abnormal materials must not collapse into one generic shader
    response.

BC.2 Equipment, outfits and skins
   Physically worn equipment normally appears unless a real state/ability hides, transforms,
    internalizes or removes it.
   Equipment respects body/form compatibility rather than universal humanoid slots.
   Skins/outfits are presentation content unless explicitly defined as gameplay-bearing Versions/forms.
   Clothing uses modular compatible layers where feasible so equipment, damage/removal and
    transformations can reflect actual state.
   No artificial permanent fallback costume is required when valid removable layers are absent;
    Privacy/SFW Presentation remains a display mask rather than canonical-state rewrite.

BC.3 Environment, civilization, lighting and palette
   Each World/Dimension owns architecture, materials, ecology, atmosphere, lighting, sky, weather,
    culture, technology/metaphysics and destruction/recovery language.
   Civilizations become more capable by deepening their authored identity rather than converging on
    generic modern Earth design.
   The starting World remains broadly historical-medieval and comparatively grounded, emphasizing
    dangerous wilderness, believable settlements/materials and gradual discovery of stranger
    metaphysics.
   Seed-generated regions use authored biome/style grammars so procedural composition remains art-
    directed.
   Lighting is cinematic but legible: character readability, strong time/weather/world identity,
    restrained bloom and authored exposure. There is no universal neon wash.
   There is no universal Rank color code. Color belongs to character, faction/culture, material,
    environment, ability, field and UI semantic state. Rarity may affect card/frame treatment without
    dictating the whole palette.
                                                   Page 171

<PARSED TEXT FOR PAGE: 172 / 175>

                                                      OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

BC.4 VFX and reality fields
   VFX communicates what rule is occurring, not merely how expensive/strong an attack is.
   Reusable primitives include particles, trails, decals, mesh/material effects, volumetrics, lighting, sky
    changes, geometry deformation, field boundaries, post-process and environmental simulation cues.
   Projection, Alteration, Materialization, Manifestation and Negation are behavioral grades, not
    universal color templates.
   Reality fields may alter sky, light, geometry, materials, weather, gravity/motion cues, space, objects
    and law-specific structures.
   Overlapping fields can show boundaries, mixed zones, fractures, dominance pockets, local
    overwrite, unstable interfaces and clean negation according to actual mechanics.
   Very high power may become visually quieter when precision/law-level control fits better than
    particle density.
   When spectacle conflicts with readability, priority is critical telegraphs, defensive readability, active-
    character silhouette, boss/major field boundaries, then secondary spectacle.

BC.5 Injury, destruction and restoration
   Wounds, severing/dismemberment, armor/equipment/clothing damage, mechanical breakage,
    crystalline/elemental fracture, species/material-specific fluids and later regeneration/reconstruction
    are supported when mechanics/lore justify them.
   One generic human-blood effect is never applied to every entity.
   Persistent injuries/destruction remain visible while authoritative state persists.
   Locations may remain destroyed, be repaired, regrow, transform, reconstruct or reach another
    equilibrium. Streaming never silently rolls history back.

BC.6 Animation, locomotion and camera
   Animation uses a HI3rd-like production philosophy: compatible shared foundations plus bespoke
    combat/movement expression where identity matters.
   Strong anticipation/follow-through, mechanic-defined responsive cancel windows, clear defensive
    telegraphs, personality in idles/transitions, and bespoke signature Ultimates/World-Fantasm
    activations are required.
   Animation responsiveness takes precedence over cinematic inertia.
   Locomotion can differ materially for humanoids, heavy bodies, non-humanoids, flight,
    swimming/underwater, climbing, transformed bodies, abnormal gravity, mounts and vehicles.
   World Mode uses a readable character-action camera with controlled target framing, limited shake,
    large-enemy dynamic framing and temporary cinematic camera authority only when gameplay state
    permits.

BC.7 Cinematics and summon/progression presentation
   Story cinematics are primarily real-time/in-engine so current outfit, form, injury, equipment,
    location and world state remain accurate.
   Illustrated/prerendered sequences remain valid when artistically superior or technically justified.
   Major first-time scenes use full presentation; replayed content may be skipped more freely.
   Gacha/breakthrough sequences are premium but original: dimensional/contract imagery, Identity-
    pattern formation, rarity reveal and character-specific motif/VFX accents are valid. Repeat
    acquisition still presents a meaningful full Manifestation, not shard conversion.
                                                   Page 172

<PARSED TEXT FOR PAGE: 173 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

BC.8 Ruler Mode visual identity and opening
   Ruler Mode uses a premium character-first dimensional-command language rather than a generic
    dashboard.
   Direction: dark/neutral structural surfaces, context-driven luminous accents, restrained
    dimensional-line/lattice motifs, layered depth/translucency where performance permits,
    character/world imagery over decorative chrome and compact information density without
    spreadsheet aesthetics.
   Rarity frames remain readable without excessive ornament. Destructive/recovery actions are
    visually distinct from normal navigation.
   Default title/opening target is animated/moving premium presentation with reduced-motion/static
    fallback, fast Continue access and clear Recover/Import/Settings access.
   Portrait Ruler Mode to landscape World Mode transition uses brief intentional
    dimensional/map/camera/UI handoff language; it must not be used to hide long synchronous loading.

BC.9 Scalability and asset contract
   Quality scales per subsystem: effect density, particles, hair/cloth simulation, shadows, crowds,
    reflections, post effects, environment HLOD and distant animation update rates.
   Core character silhouette, critical telegraphs and gameplay-state readability are protected across
    profiles.
   Exact budgets are determined on the physical S26 Ultra.
   Stable content/package IDs must allow higher-quality asset replacements, extra animation/audio
    packs, promoted-NPC presentation upgrades and World-specific renderer profiles without changing
    persistent entity identity.
   Final game name/logo/icon remain identity assets rather than architecture blockers.


BD. Tuning Ownership and Coding / Testing Only Handoff
STATUS: FINAL PRE-CODING HANDOFF. From this checkpoint onward, the next project
conversation/workstream is restricted to coding, testing, deterministic simulator fitting, build validation,
Android packaging and physical-device profiling unless implementation exposes a genuine contradiction
between frozen player-facing rules.

BD.1 Tuning parameter classes
   FixedRule: creative/system invariant that code/tests may rely on until explicitly revised.
   TuningCandidate: accepted starting value that deterministic simulation may change without
    becoming a design revision.
   ContentAuthored: value belongs to a character/skill/item/World/event definition.
   DeviceProfiled: value comes from physical S26 Ultra profiling/platform constraints.
   Derived: computed from other authoritative values and never independently tuned.
   Every tunable parameter records stable ID, class, type/unit/domain, range/invariant, owning system,
    source/version, rationale and simulator/test coverage.

BD.2 Fixed implementation invariants
   10,000 basis points = 100%.


                                                  Page 173

<PARSED TEXT FOR PAGE: 174 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Current universal Existence Rank ontology uses 17 named bands with Levels 1-100 while definitions
    remain data-driven/extensible.
   Territory reclamation grace = five in-game days.
   First gacha unlock requires more than one in-game month of continuous valid Territory control and
    remains permanent once earned.
   Turn battle supports up to 18 characters per side with up to six active and successor structure.
   World Mode immediate party baseline = protagonist plus up to two switch/QTE companions.
   Effective Crit Rate overflow after resistance converts +1% -> +2% Crit Damage.
   Hit overflow creates guaranteed eligible hit instances per full +100% band plus seeded remainder.
   Direct percent-MaxHP offense is not ordinary generic attack math.
   No real-money/IAP gacha economy.
   Repeat character acquisition creates a full persistent Manifestation.

BD.3 Versioned tuning candidates
   Rank coefficient candidate: R(r) = 10^(0.40r + 0.040r^2).
   Within-Rank/breakthrough logarithmic growth target: approximately 35% / 65%.
   Generic negative-DEF branch: 2^(-DEF / DefenseReference).
   Generic lower->higher Rank-Suppression envelope: 100 / 100 / 75 / 40 / 15 / 3% at effective gaps 0 / 1 /
    2 / 3 / 4 / 5+, with channel-specific tuning.
   Standard-banner simulation candidate: 1.2% base top rate, soft pity after 50, +5 percentage points on
    pulls 51-69, hard pity 70, 50/50 featured then guarantee.
   Acquisition-focused progression bands: approximately 4-8 / 7-14 / 14-27 / 27-52+ pull-equivalents per
    hour for the current simulator scenarios.
   Offline newly-generated irreversible strategic-loss ceiling candidate: approximately 15% current
    sovereign strategic weight per catch-up resolution, as a ceiling rather than scheduled loss.
Changing a TuningCandidate after deterministic simulation does not constitute an architecture revision
unless it violates a FixedRule or frozen player-facing invariant.

BD.4 Repository handoff