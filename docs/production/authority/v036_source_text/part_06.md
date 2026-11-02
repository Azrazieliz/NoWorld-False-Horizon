# v0.36 full source text — segment 6
Original: Offline_Adult_Gacha_RPG_Master_Architecture_Baseline_v0.36_checkpoint_RulerOverlord.docx (143 pages), uploaded 2026-11-01. Derived text extraction, not a replacement for original embedded figures/tables. SOURCE LINE WINDOW 5001–6000. Historical decisions may be superseded by 7★ overlays.

   To protect against accidental uninstall/app deletion, the game must maintain recoverable backups
    outside app-private storage. App-private Android data alone is insufficient because uninstall may
    remove it.
   Preferred implementation: rotating compact state snapshots in a user-visible persistent
    Documents/Downloads-style backup location selected/authorized through Android's Storage Access
    Framework, plus a manual Export Save operation. Content assets are not duplicated into the save
    backup; only authoritative state, schema/version metadata, hashes and required small metadata are
    included.
   Backups should be integrity-checked, versioned and migratable. Maintain several recent snapshots
    plus less-frequent older snapshots rather than only one overwrite-prone file.
   After reinstall, the game exposes Recover Existing World / Import Backup so the user can reselect the
    persistent backup file/folder and restore the single canonical history.
   Optional secondary copies to a cloud/archive service such as TeraBox may be added later, but core
    save survival must not depend on cloud connectivity.

AI.8 Summon materialization behavior - RESOLVED
   When a character is pulled, her Manifestation appears immediately in the portrait/vertical Ruler
    presentation at summon time. However, she does not become available for active World Mode
    deployment until the protagonist returns to controlled territory, where the new Manifestation is
    physically/operationally anchored into the active roster.


                                                 Page 105

<PARSED TEXT FOR PAGE: 106 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AJ. Ruler Access, Gacha Anchoring, Factors and Protagonist
Progression
STATUS: CURRENT AUTHORITATIVE BASELINE except where explicitly marked OPEN. This checkpoint
supersedes the older requirement that every Ruler must already possess a subordinate contract.

AJ.1 Ruler status and gacha unlock
   Ruler status does not require an existing subordinate/character contract.
   A lone individual may qualify as a Ruler if they possess/control territory and have enough personal
    strength, Authority or other valid capability to establish and keep that control.
   This permits solitary Rulers whose sovereignty exists because nobody can successfully remove them,
    even if they have no subjects or contracted characters.
   Gacha access is a later validation of stable sovereignty rather than an instantaneous reward for
    touching/claiming land.
   The protagonist unlocks the canonical gacha/contract-pull mechanism after keeping a territory
    under continuous control for more than one in-game month.
   If the protagonist deliberately remains territoryless for a long period, gacha remains unavailable for
    that period; this is a valid player trajectory, not a progression failure.

AJ.2 Summon appearance and active-roster anchoring
   A successful pull immediately establishes ownership of the new ruler-specific Manifestation and
    presents her directly in the portrait/vertical Ruler interface.
   The pull does not require the character to physically appear beside the protagonist in the current
    World Mode location.
   A newly pulled character becomes available for active World Mode deployment only after the
    protagonist returns to controlled territory. Returning to territory performs the practical/physical
    roster anchoring required for field deployment.
   After that first anchoring, ordinary deployment follows the normal party/availability rules unless a
    later mechanic says otherwise.

AJ.3 Protagonist starting state
   The protagonist begins with no Factor at all. The human-like starting body does not secretly count as
    a Human Factor.
   His starting physical capability is only slightly above that of a random ordinary NPC; he does not
    begin overpowered.
   He wakes without a weapon. Bare-handed fighting is possible if necessary, but avoidance is normally
    the sensible response to early monsters.
   There is no canonical starter weapon class. The first usable weapon can be whatever he organically
    finds, buys, steals, crafts, receives or otherwise obtains.
   The protagonist begins classless.

AJ.4 Class philosophy - superseded by AK
   Classes exist conceptually, but they are not hard equipment/ability locks.


                                                 Page 106

<PARSED TEXT FOR PAGE: 107 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   A class represents specialization, training, technique, affinities and developed competence rather
    than a prohibition system.
   For example, a Swordmaster may still use a bow, rifle or magic. Without relevant talent/training the
    off-specialty option is weaker or less efficient, but it is not artificially forbidden.
   Pugilist is a valid example of a possible class/specialization.
   Factors are expected to be more fundamental to the protagonist's long-term development than class
    labels.
   Class acquisition, coexistence, advancement and Factor interaction are now defined in checkpoint
    AK.

AJ.5 Skill learning and adaptation
   The protagonist can learn skills through any logically valid source: training, observation, teachers,
    books, artifacts, Factors, technology, cultivation, supernatural events and other world-consistent
    methods.
   He may learn another being's technique through observation, comprehension and training when his
    body/Factors/capabilities permit it. This is learning, not universal magical copy-the-skill
    functionality.
   Skills may mutate, branch or evolve because acquired Factors alter the body, energy systems,
    instincts or execution method used to perform them.

AJ.6 Factor acquisition, scale and interaction
   Factor acquisition method is Factor-dependent rather than one universal mechanic. Different
    Factors may require absorption, inheritance, exposure, ritual, implantation, consumption,
    transformation, infection/corruption, experimentation, training or other appropriate causes.
   Factors may be acquired involuntarily where the relevant mechanic supports it.
   There is no fixed numerical Factor-count cap. If the game eventually contains hundreds of
    species/Factor types, the protagonist may theoretically accumulate hundreds of Factors.
   Factor incompatibility does not automatically create generic damage/rejection. Harm, mutation,
    rejection or destabilization requires an explicit integration/absorption/conflict mechanic that
    actually produces it.
   Factors may have very different ontological weight/potency. A Dragon Factor can naturally dominate
    a weak Human Factor without both being treated as equal-weight tags.
   One Factor may suppress/domininate another when their actual rules/power justify it.
   Compatible Factors may fuse into a genuinely new hybrid Factor rather than merely remaining side-
    by-side.
   Factor removal is possible only through appropriate mechanics and is normally difficult to reverse
    once deeply integrated.
   Factors may cause major anatomical/physiological changes including wings, horns, altered organs,
    skin, eyes, skeleton or other body systems.
   Where physically/metaphysically plausible, the protagonist may suppress or conceal cosmetic Factor
    traits without deleting the underlying Factor.
   A Factor may introduce entirely new resources, mechanics, movement modes, senses,
    vulnerabilities, transformations and skill grammars. Factors are not merely stat packages.




                                                 Page 107

<PARSED TEXT FOR PAGE: 108 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AJ.7 Protagonist higher-order reality mechanics
   The protagonist is not forced into the collectible-character World Fantasm progression because he
    has no Origin Rarity.
   When he possesses a territory, his Ruler-side higher-order reality expression is primarily Domain
    Manifestation/Domain Authority sourced from territorial sovereignty rather than a collectible-style
    World Fantasm grade.
   A territoryless protagonist may instead become capable of a personal World Manifestation-type
    reality field if his personal/existential power reaches the necessary level.
   For the protagonist, this personal manifestation is power/existence based rather than Origin-Rarity
    based.
   RESOLVED: a personal World Manifestation acquired through the protagonist's own existence
    remains an independent intrinsic power even after he later acquires territory and Domain
    Authority. Domain Manifestation/Authority and personal World Manifestation may coexist, overlap,
    reinforce, conflict or combine when explicit abilities/rules permit it; neither automatically replaces
    the other. Losing territory can remove or weaken Domain-derived effects but does not erase the
    personal World Manifestation.

AJ.8 Protagonist Transcendence - current direction
   The protagonist is not assigned a hidden collectible Origin Rarity merely to fit the character
    Transcendence table.
   His Transcendence outcome should derive from what he actually becomes through personal power,
    Factors, skills, experience and existential development.
   The five-grade vocabulary Ascendant / Unbound / Exalted / Transcendent / Absolute can therefore
    apply to the protagonist on a development basis rather than by birth rarity.
   RESOLVED: protagonist Transcendence is a single irreversible breakthrough. The protagonist may
    progressively qualify for Ascendant, Unbound, Exalted, Transcendent and Absolute before taking
    that breakthrough, but only Transcends once. The resulting grade is the highest grade fully qualified
    at the moment of Transcendence and is permanent under the normal progression rules.




                                                 Page 108

<PARSED TEXT FOR PAGE: 109 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AK. Class Architecture and Design-Collaboration Operating
Mode
STATUS: CURRENT AUTHORITATIVE BASELINE. This checkpoint records the Creative Director's answers
to the class-system pass and the new interaction policy for future design work.

AK.1 Core class model
   A Class is an objectively system-recognized description of developed mastery, not an equipped job
    slot and not a hard restriction layer.
   Classes are earned automatically from what a character has actually learned, practiced and
    mastered. The player does not need to select a class in advance in order to become capable of its
    discipline.
   A character may possess multiple Classes simultaneously. There is no universal
    primary-class/secondary-class slot limit.
   Classes are non-restrictive. A Swordmaster may still use bows, firearms, magic, tools or other
    techniques. Effectiveness comes from actual skills, talent, Factors, anatomy, equipment and
    proficiency rather than an arbitrary wrong-class penalty.
   The protagonist begins classless. Pugilist is a valid example of a Class that may emerge if unarmed
    practice and related skills become sufficiently developed.

AK.2 Normal Classes, composite/specific Classes and Grand Classes
   The class framework has at least two recognition tiers: Normal Class and Grand Class.
   Normal Classes identify a coherent developed discipline such as swordsmanship, archery, magic,
    pugilism, alchemy, engineering or other mechanically meaningful fields.
   When several disciplines, skills or Factors develop into a stable coherent style, the system may
    recognize a more specific/composite Class with its own name rather than forcing the character to
    pick only one parent Class. Parent mastery remains part of the character rather than being deleted.
   Grand Class is an exceptional recognition tier reached when mastery, scope, coherence and
    demonstrated capability substantially exceed ordinary Class standards. It is representative of what
    the character has become, not a permission flag that suddenly grants unrelated powers.
   Grand status can belong to a familiar discipline or to a highly specific/composite Class. The
    architecture therefore supports both Grand Swordmaster-like recognition and unique Grand Classes
    created by unusual combinations.

AK.3 Skills cause Classes, not the reverse
   Skills, techniques, experience and demonstrated mastery produce Class recognition; Class
    recognition does not automatically inject a package of new skills into the character.
   A character can learn techniques before the corresponding Class is recognized. Once enough related
    capability coheres, the Class label/state is earned automatically.
   Because Classes do not own learned skills, losing focus on a Class does not delete techniques already
    learned. Actual skill loss requires an explicit mechanic such as memory damage, sealed capability,
    bodily loss or another authored effect.
   Class advancement is therefore driven primarily by real use, training, demonstrated competence,
    relevant achievements and the development of the underlying skills rather than generic class XP.

                                                 Page 109

<PARSED TEXT FOR PAGE: 110 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AK.4 Class evolution, fusion and Factors
   Classes may branch or evolve when the underlying skill set develops into a different coherent
    specialization.
   Multiple Classes may fuse into a new composite/specific Class when the combination becomes a real
    integrated discipline rather than merely a list of unrelated competencies.
   Factors may alter, mutate or enable unusual Class evolutions when they materially change what the
    body/existence can do.
   A universal Class should generally not require a specific species Factor merely as a gate. If a route
    depends on unique anatomy/existence - for example a wing-specific draconic aerial style - it is better
    represented as a character-/Factor-specific Class or unique specialization rather than pretending it is
    universally available.
   Character-specific/Factor-specific Classes may coexist with ordinary Classes and may themselves
    become composite or Grand if development justifies it.

AK.5 Class representation and gameplay use
   Classes may be used by AI, story logic, training systems, appraisal, eligibility checks, NPC social
    recognition, synergies and authored mechanics, but should not substitute for the actual underlying
    capabilities.
   The UI should emphasize the most relevant/highest developed Classes and Grand Classes rather than
    forcing a giant equipped-class list. Full history can live in deeper character information if useful.
   No universal class-count cap is required. Practical readability comes from relevance sorting, not
    artificial deletion of learned disciplines.
   Raw stat development should come from the character's actual training, Factors, Rank, equipment
    and mechanics. A Class may summarize that development or provide class-keyed interactions, but it
    should not create arbitrary stat multipliers simply because a label changed.

AK.6 Gacha sovereignty details selected under delegated design ownership
   The initial one-month gacha qualification requires continuous territorial control. Losing all valid
    control before qualification resets that qualification timer.
   Once gacha has been unlocked by proving stable sovereignty, the unlock is permanent; later
    temporary loss of all territory does not erase the dimensional access already established.
   A Ruler may continue to perform pulls while away from territory or temporarily territoryless after
    permanent unlock.
   Every newly pulled Manifestation still requires a first anchoring in controlled territory before active
    World Mode deployment. If no territory is currently controlled, the Manifestation remains owned
    and visible in Ruler Mode but unanchored until valid territory is regained.

AK.7 Design-collaboration operating rule
   Future Creative Director questioning should use only large, subsystem-level questions that
    materially change player experience, ontology, progression architecture or narrative direction.
   Do not return to long micro-questionnaires by default. Intermediate rules, edge cases, UI details, data
    structures and engineering-compatible behavior should be designed autonomously from the
    established baseline.
   After each autonomous design pass, report the resulting design compactly: what was decided, why it
    is compatible with the Creative Director's rules, and which genuinely major choices remain.
                                                  Page 110

<PARSED TEXT FOR PAGE: 111 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   The Creative Director may modify any reported decision. New explicit direction supersedes the
    delegated design choice and must be propagated through the cumulative baseline and
    implementation documentation.
   Plot twists, hidden identities and future narrative answers remain spoiler-protected unless the
    Creative Director explicitly asks to inspect them; large questions may still be asked about tone, limits
    or desired experience.

AK.8 Major open choices retained
   RESOLVED by checkpoint AK: one irreversible protagonist Transcendence whose permanent grade
    equals the highest qualification satisfied at breakthrough.
   RESOLVED by checkpoint AK: intrinsic personal World Manifestation and territorial Domain
    Manifestation/Authority coexist as independent higher-order systems.
   RESOLVED by checkpoint AL: Ruler progression is organic and state-based, with major milestone
    titles rather than a universal numbered/name ladder.




                                                  Page 111

<PARSED TEXT FOR PAGE: 112 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AK. Protagonist Transcendence and Dual Higher-Order Reality
STATUS: CURRENT AUTHORITATIVE BASELINE. This checkpoint resolves the two remaining protagonist
higher-order progression choices from AJ.

AK.1 Single irreversible Transcendence
   The protagonist Transcends only once; it is an irreversible breakthrough, not a post-breakthrough
    rank ladder.
   Beforehand, he may progressively qualify for Ascendant, Unbound, Exalted, Transcendent and
    Absolute.
   On Transcendence, he receives the highest grade whose power ceiling and bespoke conditions are
    fully satisfied.
   That grade is permanent under normal progression; an Exalted breakthrough does not later become
    Transcendent or Absolute.
   Waiting preserves access to higher grades; transcending earlier trades that potential for immediate
    Transcendence power.
   Before confirmation, show the currently attainable grade and clearly warn that the choice is
    permanent; higher possible grades may be hinted without exposing every hidden condition.
   Each grade requires sufficient power plus meaningful save-specific conditions/feats rather than one
    universal checklist.

AK.2 Personal World Manifestation and Domain Authority coexist
   A sufficiently powerful territoryless protagonist may develop intrinsic personal World Manifestation
    from his own existence.
   If he later becomes a Ruler, it remains a separate intrinsic power; a Domain does not convert, delete
    or demote it.
   Domain Manifestation/Authority remains a separate layer anchored to territory and sovereignty.
   Both may coexist, overlap, reinforce one another or enter an explicit authored combined state.
   They do not automatically fuse or stack.
   Losing territory can remove/weaken Domain effects but cannot erase the intrinsic World
    Manifestation.
   Personal World Manifestation, Domain Authority and others' World Fantasms resolve through the
    same higher-order conflict rules: strength, Authority, counters, compatibility, relevance and explicit
    mechanics.

AK.3 Design consequence
Territory is the easier anchored route to large-scale reality control; the territoryless personal route
demands much greater development. Gaining both produces a genuine dual metaphysical toolkit
without making either playstyle mandatory.




                                                  Page 112

<PARSED TEXT FOR PAGE: 113 / 143>

                                                      OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AL. Organic Ruler Progression and Milestone Titles
STATUS: CURRENT AUTHORITATIVE BASELINE. This checkpoint replaces the idea of a universal fixed
Ruler-rank ladder with organic sovereignty evaluation and recognized milestone titles.

AL.1 Continuous sovereignty state
   Ruler progression is evaluated from actual state rather than one universal sequence of named tiers.
   Relevant dimensions include personal/existence power, effective Authority, territory control and
    quality, Domain/Core development, roster/contracts, subordinate Rulers, military/geopolitical reach,
    multi-world or dimensional reach, and lore-specific structural achievements.
   Different Rulers may therefore be incomparable on a single axis: one may be personally
    overwhelming with one territory, another may govern a vast hierarchy while relying on
    subordinates, and both remain legitimate Rulers.
   No hidden rubber-banding or automatic stat grant is attached merely to a title.

AL.2 Milestone titles
   Ruler is the foundational sovereignty status.
   Overlord is a structural milestone/title that reflects actual superior authority over one or more
    subordinate Rulers rather than a mandatory next level for every Ruler.
   Further milestone titles are data-driven. Some may be universal if they describe a genuinely
    universal structure; others may be culture-, world-, faction- or lore-specific.
   A milestone title is recognized only while its defining state is true unless the title is explicitly
    historical/earned and designed to persist as an honorific.

AL.3 Player presentation
   The Ruler UI emphasizes concrete sovereignty facts and capabilities rather than presenting one
    linear Rank bar.
   Milestone requirements may be shown directly when they are objectively knowable; hidden/lore-
    dependent milestones may instead appear through clues or discovered information.
   Internal comparison scores may exist for AI, matchmaking-like evaluation, simulation or debugging,
    but are not allowed to override the actual underlying state or become a universal power verdict.


AM. Detailed Finalization Register
STATUS: WORKING COMPLETION REGISTER. This is the authoritative checklist of areas that still require
detailed design, content definition, numerical tuning, production specification or empirical device
validation. It distinguishes one-time architecture work from ongoing content production and evidence-
driven tuning.

AM.1 Register
   Ruler milestones [ARCHITECTURE RESOLVED; content/UI tuning remains]: Ruler and Overlord are
    the only universal sovereignty titles; Overlord is the highest universal title and requires both
    sufficient personal/Authority capability and genuinely Overlord-scale sovereignty. That scale may be
    multi-Domain, hierarchical, world-spanning or even one contiguous dimension-spanning Territory.
    Joint Overlords are valid; active title and historical peak are tracked separately. See checkpoint AW.
                                                   Page 113

<PARSED TEXT FOR PAGE: 114 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Class recognition [ARCHITECTURE RESOLVED; content/tuning remains]: Classes arise automatically
    from real mastery; specific/composite/emergent Classes are supported; Crown is a non-unique apex
    mastery tier; each canonical Class can have only one active Grand bearer, appointed/recognized by
    the connected dimensional defensive order as the Class epitome against existential threats; attained
    mastery and current expression are separate; knowledge/appraisal remains non-omniscient. See
    checkpoint AV.
   Existence/Power Rank [ARCHITECTURE RESOLVED; tuning/content remains]: Universal named rank
    ladder, 1-100 levels per Rank, superlinear rank-step power scaling, breakthrough promotion,
    attained/effective Rank split, knowledge-dependent visibility, world-ceiling suppression and large-
    gap Rank Suppression are defined in checkpoint AN. Exact coefficient curves and per-content
    breakthrough requirements remain simulation/content work.
   Protagonist Factors [ARCHITECTURE RESOLVED; content/tuning remains]: Factors are causal
    pieces/aspects of existence, never inert tags. They have provenance, potency/quality, development,
    canonical acquisition, physiology/resources/abilities and real interactions. No hard Factor cap exists;
    all acquired Factors remain causally present; expression/main-Factor emphasis is player-
    controllable as mastery allows; conflict, synthesis, fusion and transformations are authored from
    actual Factor rules. See checkpoint AO.
   Protagonist higher-order progression [ARCHITECTURE RESOLVED; content/tuning remains]:
    Transcendence qualification combines universal minimum power/Existence floors with personal
    proofs from the one persistent world; Absolute remains theoretically reachable absent genuine
    irreversible foreclosure; personal World Manifestation is emergent/non-template and continues
    evolving; combined Domain + personal-Manifestation techniques are supported without automatic
    stacking. See checkpoint AT.
   Character progression [ARCHITECTURE RESOLVED; content/tuning remains]: Current Rarity reflects
    both collectible framework and attained reinforcement but is not combat power; Rarity promotion
    uses a shared backbone plus character-specific conditions; development routes are
    open-ended/composable; multiple owned Manifestations of one Character Identity may follow
    different routes; Grand Convergence can consolidate fully reinforced divergent manifestations into
    one final Grand Manifestation. See checkpoint AP.
   World Fantasm content [Ongoing content]: For each character/grade, author the actual field rules,
    visuals, counters, collision behavior, persistence exceptions and Transcendence improvements.
   Gacha economy [Simulation/tuning]: Finalize banner taxonomy, base rates, featured split, pity/soft
    pity, guarantees, carry rules, currencies/tickets, duplicate treatment, upgrade/development costs and
    Transcendence costs.
   World Director [ARCHITECTURE RESOLVED; content/tuning remains]: World-time/world-state
    cadence, bounded delays, offline event progression with catastrophic-loss safeguards, autonomous
    approved-template composition, concurrent crises, sealed-package activation, banner/world
    relevance and deterministic audit/replay are defined in checkpoint AR.
   Cosmology/dimensional architecture [ARCHITECTURE RESOLVED; authored lore/content remains]:
    semi-conscious metaphysical Dimensions, extensible higher structures, evolving/declining World
    Rank, Junctions, dimensional preparation, Identity-pattern access and broad external/internal
    existential-threat ecology are defined in checkpoint AS. Exact hidden history, named threats and
    protagonist-origin revelations remain spoiler-protected.
   Narrative production [Ongoing content]: Author the opening, major arcs, antagonists, mysteries,
    recontextualizations, failure/alternate resolutions, character arcs and hidden lore while preserving
    persistent-world causality and spoiler protection.


                                                 Page 114

<PARSED TEXT FOR PAGE: 115 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Starting dimension content [Major content system]: Finalize generation grammar for the
    historical-medieval starting world: biome pool, geography/landmark rules, settlement/faction
    templates, three-faction starting logic, dungeons, creatures, resources, local Ruler,
    equipment/technology and encounter tables.
   Later-world progression [Major content system]: Define world/dimension archetype library,
    transition/junction requirements, how settings escalate toward high
    fantasy/post-apocalyptic/cyber/space/abnormal physics, and world-specific ceilings/rules.
   Civilization simulation [Design detail]: Finalize the small data model for civilization development
    vectors, advancement triggers, decline/recovery, cross-vector asymmetry and how those values
    affect armies, structures, equipment, available characters and visuals without becoming a grand-
    strategy simulator.
   Population model [Engineering/design cleanup]: Finalize the aggregate population fields/tags and
    update rules that actually matter after AE.4; remove residual genealogy/demographic complexity
    except named/story-relevant facts.
   Faction/war resolution [Design detail + tuning]: Finalize army capability formulas, objective
    resolution, attrition/logistics effects, local battlefield promotion, surrender/retreat/conquest
    outcomes and Ruler/important-character intervention.
   Territory/Domain/Core [ARCHITECTURE RESOLVED; content/tuning remains]: effective-control
    claims, five-day reclamation, Core crystallization/awakening, Core-as-heart consequences, Core
    capture/destruction, previously defined Core fusion/Concept synthesis, populationless Domains and
    Overlord multi-Domain sovereignty are reconciled in checkpoint AQ.
   Projects/resources/logistics [Design detail + tuning]: Finalize resource schemas, crafting/project
    costs, route capacity/risk/blockade formulas, project timing, settlement state summaries and
    delegation outputs.
   Dispatch [Design detail + tuning]: Finalize assignment types/templates, duration/risk/success
    model, knowledge/reward/injury outputs, autonomous subordinate use and offline resolution.
   Turn combat tuning [Simulation/tuning]: Finalize exact damage/DEF curves, Rank Suppression,
    effect grades, readiness costs, duration/stacking numbers, enemy AI priorities, auto-combat policies
    and progression scaling.
   Action combat content [Content + tuning]: Finalize per-character cancel windows, combo/attack
    trees, parryable animations, dodge timing, QTE conditions, multi-tier Ultimate gestures, camera/lock-
    on tuning, boss mechanics and traversal-combat animation coverage.
   Hybrid bosses [Ongoing content]: Author phase-transition rules, state carryover exceptions,
    bespoke resources, arena/environment transformations and failure/escape behavior per major boss.
   Exploration/traversal [Design detail + content]: Finalize concrete controls/physics/content
    grammar for climbing, swimming, underwater, flight, mounts, vehicles, teleport infrastructure,
    destructibility and later space/abnormal environments.
   World time/calendar [Content-specific design]: Finalize starting-world day
    length/calendar/seasons/weather, per-world time ratios, sleep/travel time rules, offline catch-up time
    compression and calendar presentation.
   Ruler Mode UI [Major presentation]: Finalize the five primary bottom destinations, subnavigation,
    Home/lobby behavior, Territory/Domain overview/map, reports/events/Chronicle/Codex placement,
    gacha/roster flow and notification policy.
   World Mode HUD [Major presentation]: Finalize minimap/compass, health/resource presentation,
    party/switch/QTE controls, lock-on/aim UI, boss/status/damage presentation, interaction prompts,
    control customization, haptics and accessibility.


                                                 Page 115

<PARSED TEXT FOR PAGE: 116 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Art direction [Major presentation]: Finalize character rendering style, proportions,
    materials/shading, environment stylization, lighting, palette, outlines, equipment visibility,
    injury/blood/clothing-damage language and world/dimension variation rules.
   VFX/reality-field language [Major presentation/content]: Finalize visual grammar for Authority,
    Domains, Projection/Alteration/Materialization/Manifestation/Negation, collisions/mixed fields,
    Transcendence and high-rank effects while preserving readability/performance.
   Animation/cinematics [Production pipeline]: Finalize rig families, locomotion/combat shared
    libraries, character-specific layers, ultimate tiers, summon sequences, World Fantasm/Domain
    activation shots, story-cinematic tiering and camera language.
   Voice/audio/music [Production pipeline]: Finalize voice-coverage tiers/languages, battle/ambient
    voice rules, generated/promoted-NPC voice policy, spatial audio, adaptive music states, leitmotifs and
    World Fantasm/Domain audio interaction.
   Mature-content production [Protected content pipeline]: Finalize non-generic
    eligibility/consent/state data, scene/content taxonomy, reusable 3D rig/animation architecture,
    authored character-specific variations, integration with story/world state, and presentation/settings.
   Equipment/items/crafting content [Content + tuning]: Finalize item archetype catalogs,
    equipment visual families, modifier pools, rarity/rank/evolution costs, damage/destruction rules,
    historical/named-item evolution and crafting recipes.
   NPC generation/promotion [Content pipeline]: Finalize modular
    species/body/face/hair/outfit/voice/personality pools, generated history/skill grammars, promotion
    thresholds, immediate playable tier and bespoke-upgrade workflow.
   Dialogue/knowledge/memory [Design + engineering]: Finalize structured knowledge/belief
    schema, rumor propagation, dialogue pools, high-value generative-dialogue boundaries, memory
    consolidation, semantic retrieval and offline/no-model fallback.
   Persistence/save recovery [Engineering]: Finalize snapshot cadence/retention, Storage Access
    Framework UX, import/recovery, migration/rollback, integrity checks, Clear Data semantics, manual
    export and optional secondary archive/cloud copies.
   Content packaging/storage [Engineering/production]: Finalize package granularity, dependencies,
    sealed future packages, patch/eviction/cache policy, hot-set selection, external archive workflow and
    real asset-size budgets.
   Android performance [Device profiling]: Empirically finalize expedition/preload sizes,
    actor/animation/VFX budgets, texture/mesh budgets, HLOD distances, 30/60/120 profiles, thermal
    fallback and battery constraints on the S26 Ultra.
   Build/UE verification [Engineering gate]: Complete UE 5.8 UHT/UBT/link compile, automation tests,
    Android cook/package/install, device launch, persistence test, performance capture and iterative
    fixes under the required gate order.
   Balance simulation [Engineering/tuning]: Build deterministic simulators for combat, gacha,
    progression, economy, wars, projects and offline catch-up; use results to set numerical values rather
    than guessing.
   QA/recovery tooling [Engineering/QA]: Finalize save migration/fuzz/corruption tests, deterministic
    replay, long-history catch-up tests, content validators, package hash/provenance checks, crash
    diagnostics and hidden repair/debug UI.
   First production catalog [Ongoing production]: Create the large initial sets of female gacha
    characters, NPCs, monsters, bosses, factions, locations, equipment, skills, Factors, Classes, Domains,
    World Fantasms, story events and mature-content packages.



                                                 Page 116

<PARSED TEXT FOR PAGE: 117 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AM.2 Already resolved and should not be reopened without a reason
   Core product identity: offline-first, Android-first, one persistent world, Ruler Mode + World Mode
    sharing authoritative state.
   Hard simplification doctrine and protected pillars/cut systems.
   Core character ontology, Origin/Current Rarity distinction, Rank separation, MR World Fantasm
    unlock, World Fantasm grade table and character Transcendence grade table.
   Protagonist no-rarity model, Factor-first development direction, one irreversible Transcendence, and
    independent personal World Manifestation + Domain Authority.
   World Mode action-combat macro/controls and strategic turn-combat architecture.
   Gacha unlock after more than one in-game month of continuous territorial control, permanent
    unlock thereafter, first-return territorial anchoring for new Manifestations, and female-only pullable
    character intent.
   Organic Ruler progression macro: sovereignty state plus milestone titles rather than a universal
    fixed ladder.

AM.3 Completion logic
A system is considered design-final when its player-facing rules, data/state transitions, content-authoring
contract, failure/edge behavior and UI exposure are specified. Numerical values remain provisional until
deterministic simulation or S26 Ultra profiling validates them. Ongoing content catalogs never become
"finished" in the same sense; they become production-ready when their authoring templates and
validation rules are final.




                                                 Page 117

<PARSED TEXT FOR PAGE: 118 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AN. Existence / Power Rank
STATUS: CURRENT AUTHORITATIVE BASELINE. This checkpoint defines the universal personal-power
Rank architecture. It remains distinct from collectible Rarity, Class, Ruler sovereignty/milestone titles,
World Rank and Transcendence.

AN.1 Universal named ladder
   Existence / Power Rank uses one universal ordered ladder understood by the engine across all
    worlds and dimensions.
   Ranks are player-facing names rather than displayed numerals. The engine may retain an internal
    ordinal solely for ordering, arithmetic and migration.
   Cultures, factions and worlds may use local aliases or titles for the same underlying ranks without
    changing their systemic meaning.
   Current canonical ladder, from lowest to highest: Mortal -> Awakened -> Elite -> Champion -> Hero ->
    Sage -> Saint -> Legend -> Mythic -> Astral -> Celestial -> Divine -> Cosmic -> Dimensional -> Immortal
    -> Eternal -> Primordial.
Rank names are conventional ontology labels, not literal claims about profession, morality, religion,
wisdom, age, immortality or species. A Saint-Rank being is not necessarily holy, an Immortal-Rank being
is not automatically unkillable, and Primordial is the current highest universal Existence/Power Rank.
   The rank-name table is data-driven so future truly higher ontological bands can be
    inserted/extended without treating the visible name as a hard-coded enum contract.
   Ordinary people may occupy different levels within Common Rank. The protagonist can therefore
    begin slightly above an ordinary NPC without beginning at a higher Rank.

AN.2 Rank meaning and within-Rank levels
   Rank is a qualitative existence/power bracket, not merely a cosmetic label over larger statistics.
   Each Rank contains Levels 1-100. Levels describe advancement inside that qualitative bracket.
   Level 100 means the entity has reached the ordinary ceiling of its current Rank; it does not
    automatically grant the next Rank.
   Two entities in the same Rank may still differ enormously through level, stats, skills, Factors,
    equipment, counters, experience and unique mechanics.
   Rarity never substitutes for Rank. A low-Origin-Rarity character may reach very high Rank, while a
    high-Rarity character may temporarily or permanently remain at a lower Rank.

AN.3 Superlinear Rank-step power scaling
   Crossing a Rank is intentionally non-linear: later Rank breakthroughs create much larger
    normalized power increases than early breakthroughs.
   Therefore the gain from a very high Rank step can exceed the combined normalized gain of several
    early Rank steps. The user's example principle is authoritative: a hypothetical tenth-to-eleventh Rank
    breakthrough may matter more than the combined first-to-fourth progression.
   Implementation uses a Rank-band coefficient layered beneath ordinary stats. The coefficient curve
    increases superlinearly with Rank ordinal; exact coefficients are tuning data, not fixed design
    constants.



                                                  Page 118

<PARSED TEXT FOR PAGE: 119 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Displayed/authoring stats may be normalized for readability inside each Rank while the combat
    evaluator applies the Rank-band coefficient and relevant metaphysical rules. This prevents late-
    game numbers from requiring unreadably huge raw UI values.
   Rank-band scaling does not erase build identity: mechanics, counters, resistances, skill multipliers,
    equipment, Authority and explicit exceptions still matter.

AN.4 Promotion / breakthrough
   At Level 100, advancement requires a genuine breakthrough rather than generic XP overflow.
   A breakthrough requires sufficient accumulated power/development plus one or more appropriate
    conditions.
   Lower-Rank breakthroughs may use broad reusable conditions such as training completion,
    resource integration, combat achievement, ritual, cultivation, Factor maturation or equivalent
    world-valid development.
   Higher-Rank breakthroughs become increasingly bespoke and existence-specific. They may require
    unique feats, transformations, conceptual understanding, extreme environments, rare resources,
    personal trials, Factor changes or other authored conditions.
   There is no universal mandatory punishment for a failed breakthrough attempt. Failure
    consequences exist only when the specific breakthrough mechanic logically carries risk.
   NPCs and owned characters use the same breakthrough ontology, though offscreen resolution may
    be aggregated.

AN.5 Attained Rank vs effective Rank
   Store Attained Rank/Level separately from Current Effective Rank/Level.
   Seals, injuries, curses, suppression fields, corruption, resource starvation or other mechanics may
    reduce effective Rank without erasing the Rank the being actually attained.
   Removing a temporary suppression can restore the prior effective state without forcing the original
    breakthrough to be repeated.
   A mechanic may truly damage/regress the underlying existence and lower Attained Rank, but this
    must be explicit rather than inferred from temporary weakness.
   Historical Peak Rank may be retained for Chronicle, diagnostics, story logic and special mechanics
    even after true regression.
   Regression/reincarnation mechanics may create a new current Rank while preserving selected soul-
    bound knowledge/powers according to their own rules.

AN.6 Rank Suppression between beings
   Adjacent and near-adjacent Ranks primarily resolve through actual stats, resistances, skills,
    equipment and mechanics. A one-Rank difference is not a universal invulnerability rule.
   As the Rank gap grows, qualitative Rank Suppression increasingly affects interaction between
    existences.
   Rank Suppression may reduce damage/effect penetration, resistance breaking, control application,
    perception, presence tolerance or other interactions where the metaphysical gap is relevant.
   The suppression curve grows with Rank difference and also inherits the superlinear value of later
    Rank steps; equal ordinal gaps at the top of the ladder can therefore represent much greater
    absolute separation than equal gaps near the bottom.


                                                 Page 119

<PARSED TEXT FOR PAGE: 120 / 143>

                                                      OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   The combat engine treats suppression by interaction channel rather than only one blanket damage
    multiplier, allowing an entity to resist mind effects yet still be physically vulnerable, or vice versa.
   Exact thresholds and coefficients are deterministic tuning data. Baseline qualitative bands:
    same/adjacent Rank = ordinary resolution; modest multi-Rank gap = soft suppression; large gap =
    strong suppression; extreme gap = existence-level interaction failure unless a bypass applies.

AN.7 Lower-Rank threats and explicit bypasses
   A lower-Rank entity may threaten a higher-Rank one through a specific skill, artifact, Authority,
    World Fantasm, Domain rule, conceptual counter, poison, seal, weakness, environmental condition
    or other authored bypass.
   Such a bypass does not retroactively make the lower entity the same Rank.
   Bypasses may be channel-specific: an artifact might pierce defense without granting resistance to the
    higher being's attacks; a seal might affect movement without allowing ordinary damage; a
    conceptual counter may ignore one suppression interaction only.
   True tactical upsets therefore remain possible without making Rank meaningless.

AN.8 World power ceilings and cross-world pressure
   Worlds/dimensions may have natural Rank ceilings or tolerance bands.
   A being above a local ceiling is not stopped by an arbitrary invisible wall. Entry and continued
    presence remain possible when a valid route exists.
   The farther the being exceeds the world's tolerance, the greater the world-side suppression and
    instability pressure.
   Possible consequences include reduced manifested output, constrained abilities, increased resource
    cost, local reality resistance, forced self-restraint, dimensional instability, environmental damage,
    breach phenomena or other world-specific consequences.
   A slight ceiling excess may be little more than pressure; a vast excess may make unrestricted
    manifestation catastrophically destabilizing.
   Specific abilities, vessels, avatars, seals, Domains, dimensional infrastructure or deliberate self-
    limitation may reduce or bypass this pressure.
   World-ceiling suppression is distinct from combat Rank Suppression: one is the environment
    resisting an existence; the other is a qualitative interaction gap between entities.

AN.9 Knowledge and UI
   Exact Rank is knowledge-dependent rather than omniscient UI metadata.
   Owned characters and sufficiently analyzed/known entities can show exact Rank and Level.
   Unknown beings may show Unknown, an estimated band, or no Rank information depending on
    available senses, appraisal, reports and knowledge.
   Characters can conceal, distort or falsify apparent Rank when an ability or situation plausibly
    permits it.
   Ruler Mode/character pages show Attained Rank and current Effective Rank separately when they
    differ.
   Combat UI should avoid spoiling unknown high-end threats merely because the engine knows their
    Rank.




                                                   Page 120

<PARSED TEXT FOR PAGE: 121 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AN.10 Interactions with other progression axes
   Class describes mastery/specialization and does not replace Rank.
   Rarity governs collectible progression/potential structures and does not equal Rank.
   Ruler/Overlord titles describe sovereignty structure and do not equal Rank.
   World Rank describes the world's own power/tolerance context and does not equal an individual's
    Rank.
   Transcendence is a separate breakthrough/state framework and may require very high Rank/power,
    but its grade is not simply another Existence Rank name.
   Factors, equipment, World Fantasm, Domain Authority and special mechanics can influence
    effective combat capability without silently rewriting Rank.

AN.11 Engineering representation
   Authoritative state stores rank_id, attained_level, effective_rank_id/effective_level overrides,
    peak_rank_id, suppression sources and discovered/estimated knowledge separately.
   Ranks are stable data IDs with ordered metadata, display names, aliases, coefficient curves, ceiling
    interactions and migration-safe versioning.
   Combat and world simulation query Rank through declarative rule functions rather than hard-
    coding individual rank names in gameplay code.
   All exact numerical curves remain simulation-tunable and must be regression-tested so higher Rank
    steps preserve the intended increasingly large normalized gain without making explicit counters
    impossible.




                                                Page 121

<PARSED TEXT FOR PAGE: 122 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AO. Protagonist Factors
STATUS: CURRENT AUTHORITATIVE BASELINE. Factors are one of the protagonist's core ontological
progression systems. They are never cosmetic race tags or inert metadata.

AO.1 Causal ontology - no meaningless game-state labels
   A Factor is an actual incorporated aspect/piece of existence. It can change physiology, energy
    systems, instincts, senses, resistances, vulnerabilities, affinities, movement, transformations,
    resources, skills, Rank development and other real mechanics.
   Canonical player-facing concepts do not exist merely as decorative tags. A named state, Factor, Class,
    title or other meaningful designation must have causal, informational, systemic or social
    consequences.
   Titles follow the same rule: if the game recognizes a title, that recognition must matter to at least one
    rule, right, Authority relationship, social response, capability, knowledge channel or historical
    system.
   Purely internal implementation tags may still exist invisibly for indexing/querying, but they are not
    allowed to substitute for the real causal model.

AO.2 Factor identity, provenance and potency
   The same broad Factor identity can exist at radically different potency/quality. A weak trace Dragon
    Factor and an Ancient/True Dragon-derived Factor are not equivalent simply because both are
    Dragon.
   Each Factor instance/state retains meaningful provenance: source, acquisition event, lineage/subtype
    where relevant, initial quality, subsequent integrations and major evolutions.
   Potency is not only a flat number. A Factor can differ in purity, maturity, completeness, special traits
    and the concrete capabilities encoded by its source.
   Repeated acquisition of materially equivalent Factor substance normally integrates into the existing
    Factor state rather than creating meaningless duplicate tags. A second source remains separately
    recorded only when its provenance/subtype produces genuinely different behavior.

AO.3 Growth and evolution
   Factors can grow after acquisition. Growth may come from use, training, Rank advancement,
    compatible absorption, maturation, transformation, special environments, rituals, events or Factor-
    specific mechanisms.
   A Factor may unlock new organs, resource systems, forms, senses, resistances, vulnerabilities or
    techniques as it develops.
   Factor development is not required to be linear. Branches, mutations, refinements, awakenings,
    degradation and source-specific evolution are all supported where authored.
   High Rank can increase what the protagonist is capable of integrating, but Rank does not
    automatically strengthen every Factor identically.

AO.4 Canonical acquisition - no universal Factor token
   There is no universal 'gain Factor' recipe. Each Factor family/content definition declares the
    canonical ways by which it can actually be acquired.


                                                  Page 122

<PARSED TEXT FOR PAGE: 123 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Valid acquisition methods may include bloodline/inheritance, absorption/consumption,
    transplantation, infection, ritual, cultivation, exposure, transformation, implantation, symbiosis,
    experimentation, soul interaction, technology or another source-specific process.
   Some Factors may have only one canonical route; others may have several genuinely equivalent
    routes.
   The world resolves what the player physically attempts rather than refusing an action merely
    because the engine predicts it is unwise.

AO.5 Freedom-to-attempt rule
   General gameplay principle: the player is not blocked by arbitrary RPG permission checks from
    attempting a world-valid action simply because it is dangerous, inefficient, incompatible or likely to
    fail.
   If the protagonist can physically/metaphysically perform the attempt, the simulation should resolve
    it and produce the appropriate consequence.
   Failure, injury, mutation, corruption, wasted resources, hostile reactions or death may occur when
    the actual rules cause them; those consequences replace menu-level 'you are not allowed'
    restrictions.
   Only genuine impossibility, absence of the required interaction/capability, or an already-established
    hard world rule prevents resolution.
   This principle does not erase authored causality: freedom means freedom to attempt and bear
    consequences, not guaranteed success.

AO.6 Coexistence, dominance and no passive suppression
   Acquired Factors remain causally present. A stronger Factor does not passively switch a weaker
    Factor off or erase it.
   Relative potency can still determine dominance in phenotype, resource output, transformation
    stability or conflict resolution. Dominance is a weighting/interaction result, not a generic
    suppression status.
   Example: an overwhelmingly powerful Dragon Factor may naturally dominate the protagonist's
    visible biology over a weak Human Factor while the Human Factor still exists and can continue to
    matter.
   Incompatibility is never a universal generic rejection mechanic. Conflict exists only when the
    specific Factors' real rules produce conflict.
   Where a conflict does exist, it can create instability, damage, mutation, resource competition, altered
    forms or other authored consequences rather than deleting one Factor by fiat.

AO.7 Synthesis, fusion and merge
   Compatible Factors may synthesize/fuse/merge into a new composite Factor when the underlying
    rules justify a genuinely new existence state.
   Comparable potency can make true fusion especially natural because neither constituent simply
    overwhelms the other, but equal potency is not a universal mathematical requirement.
   Fusion is not automatically forced merely because two Factors are compatible. The player can
    normally preserve coexistence unless the specific acquisition/event says otherwise.
   A synthesized Factor retains provenance of its parent Factors and inherits/rewrites their capabilities
    according to authored rules.

                                                  Page 123

<PARSED TEXT FOR PAGE: 124 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Whether parent Factors remain independently expressible after a fusion is part of that fusion's
    definition. The engine never duplicates their full power accidentally by counting parent and
    composite as unrelated bonuses.

AO.8 Main / expressed Factors
   There is no hard total Factor cap. Hundreds of acquired Factors are structurally supported.
   For control and readability, the protagonist can develop one or more currently emphasized/main
    Factor expressions without making all other Factors inactive.
   Main/expressed status controls presentation priority, body-expression bias, training/development
    focus and which Factor-specific controls/resources are surfaced most prominently. It does not delete,
    suppress or unequip the rest.
   As mastery develops, the player may deliberately shift which Factors are emphasized according to
    the situation or desired build.
   There is no permanent fixed 'three Factor slots' or similar arbitrary cap. UI may pin a small number
    for convenience while the full Factor graph remains active and inspectable.

AO.9 Transformations and forms - careful separation from loadouts
   A transformation is a real bodily/existential state change produced by Factor capabilities, not a
    cosmetic preset and not a generic selection of Factor slots.
   A form may emphasize or express particular Factors more strongly, but the protagonist still owns
    and remains causally affected by the rest of his Factors.
   Forms may be Factor-specific, composite, learned, evolved or event-created. Their morphology,
    transition speed, duration, costs, vulnerabilities and retained abilities are authored from the
    involved Factors.
   Some transformations may be freely reversible; others may require time, resources, conditions or a
    specific capability. The game does not assume every anatomical change can be toggled instantly.
   A mostly human-looking baseline form, a Dragon-heavy combat form and a later multi-Factor hybrid
    form are all possible if the acquired Factors actually support them.
   Form switching never grants abilities that the underlying Factors/state do not possess.

AO.10 Skills, Classes, Rank and resources
   Factors may enable, mutate or fundamentally rewrite skills because they change the body/existence
    that performs those skills.
   Factor-created techniques can contribute to automatic Class recognition; Classes remain downstream
    descriptions of actual mastery rather than prerequisites for Factor abilities.
   A Factor can introduce an entirely new resource system, UI meter, movement model or combat
    grammar. This is a required capability of the architecture, not an exceptional hack.
   Factors may alter Rank-breakthrough requirements because they change what the protagonist is and
    what constitutes the next qualitative ceiling.
   Factor potency contributes to actual power but does not map one-to-one onto Existence Rank. A
    powerful Factor in an underdeveloped host can remain incompletely expressed.

AO.11 UI and scalability
   The main character sheet surfaces dominant/expressed Factors, important resources/forms and
    recent changes first.
                                                Page 124

<PARSED TEXT FOR PAGE: 125 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   A deeper Factor view exposes the full acquired Factor graph, provenance, potency, development,
    synthesis ancestry, current expression and known interactions.
   Filtering/grouping is by real provenance/body-system/lineage/function relationships rather than
    arbitrary collectible tags.
   At large counts, inactive UI detail is lazy-loaded; the simulation stores causal Factor state without
    rendering every Factor as a constantly active widget.
   Important changes create Chronicle/progression records so the protagonist's hybrid existence can be
    reconstructed historically.




                                                 Page 125

<PARSED TEXT FOR PAGE: 126 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AP. Character Progression, Divergent Manifestations and
Grand Convergence
STATUS: CURRENT AUTHORITATIVE BASELINE. This checkpoint supersedes the older one-owned-
Manifestation/duplicate-counter assumption. A Ruler may own multiple persistent Manifestations of the
same canonical Character Identity so that the same character can be developed along genuinely
different routes.

AP.1 Current Rarity
   Immutable Origin Rarity remains the character's acquisition-origin/potential classification and still
    determines Origin-Rarity-dependent systems such as World Fantasm grade and ordinary character
    Transcendence grade.
   Current Rarity represents how far that particular Manifestation has reinforced/developed the
    collectible character framework. It therefore partly reflects actual development, but it is not a direct
    combat-power measurement.
   A lower-Current-Rarity Manifestation may outperform a higher-Current-Rarity Manifestation
    through Rank, level, skills, equipment, Factors/biology where applicable, route synergies, counters or
    encounter relevance.
   Current Rarity is normally monotonic. Gameplay does not use routine Rarity loss;
    weakening/regression is handled primarily through Rank, effective state, injuries, seals, route states
    or explicit mechanics instead.

AP.2 Rarity promotion
   All characters share a common promotion infrastructure so the game can author, validate, simulate
    and display rarity progression consistently.
   Promotion is not a generic currency-only button. Each Character Identity/Manifestation can require
    character-specific materials, feats, story/world conditions, development states, transformations,
    victories, breakthroughs or other canonical requirements.
   Requirements may depend on the Manifestation's chosen development route, so two copies of the
    same Character Identity can reach the same Current Rarity through meaningfully different histories.
   Promotion never erases earlier route history, forms, skills or provenance.

AP.3 Open-ended development-route framework
   Evolution, Awakening and Corruption are initial named development-route families, not an
    exhaustive closed list.
   The route framework is data-driven and extensible. Future systems may add new route families
    without modifying the core character ontology.
   Routes may branch, recurse, combine, mutate and interact. Development is intended to be effectively
    unbounded at the content level rather than a short fixed tree.
   A Manifestation can hold multiple compatible development states simultaneously. Evolution,
    Awakening, Corruption and later route families are not globally mutually exclusive.
   Individual route nodes may still be mutually exclusive when their actual fiction/body/state makes
    simultaneous expression impossible.



                                                  Page 126

<PARSED TEXT FOR PAGE: 127 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Corruption is morally neutral as a system. It does not mean evil, inferior or damaged. Like an Alter-
    style development, it is simply one possible transformation/development route whose benefits, costs,
    personality effects and story meaning are character-specific.

AP.4 Multiple Manifestations of one Character Identity
   The same Ruler may own multiple persistent Manifestations/copies of one canonical Character
    Identity.
   Every owned Manifestation has its own stable Entity/Manifestation ID, Rank/Level, Current Rarity,
    equipment, learned skills, development-route graph, forms, Corruption/Awakening/Evolution states,
    history and reinforcement state.
   All such Manifestations still point to the same immutable Character Identity. They are divergent
    owned realizations/builds of one canonical identity rather than separate unrelated characters.
   A duplicate gacha acquisition therefore creates another owned Manifestation branch rather than
    merely incrementing an inert DuplicateAcquisitionCount.
   Local encounter identity-exclusivity remains the default: multiple Manifestations of the same
    Character Identity are normally not fielded simultaneously in one local combat unless a specific
    mechanic explicitly permits it.
   Different copies may nevertheless be developed, equipped, trained and assigned independently
    outside that local-combat restriction.

AP.5 Skill accumulation and compact kit integration
   A Manifestation may learn very large numbers of techniques over its lifetime.
   Learned skills are persistent capability/provenance, not a giant manual hotbar.
   The compact combat kit evolves through a mixture of autonomous character synthesis and player-
    directed build decisions.
   Minor compatible techniques may be absorbed into passives, attack properties, skill branches or
    improved versions automatically when the character's development supports that synthesis.
   Major build-defining integrations, route commitments and mutually exclusive expressions remain
    player-directed.
   The same Character Identity can therefore produce radically different compact kits across different
    owned Manifestations.

AP.6 Reinforcement and path completion
   Maximum reinforcement is evaluated per Manifestation, not merely by reaching one numeric level.
   A max-reinforced Manifestation must satisfy its current highest intended Rarity/Rank/progression
    ceiling plus its route-specific reinforcement requirements, skill/equipment or breakthrough
    conditions where that character definition requires them.
   Because route space is intentionally open-ended, 'complete the character' cannot mean exhaust
    every theoretically possible future route. Content definitions instead declare meaningful
    convergence-relevant route lineages/achievements.
   A new content package may add later development beyond an older maximum; historical max-
    reinforcement status remains recorded while the new ceiling becomes available.




                                                 Page 127

<PARSED TEXT FOR PAGE: 128 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE