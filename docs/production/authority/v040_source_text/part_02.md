# v0.40 historical master — source extraction 2/10
Source: Offline_Adult_Gacha_RPG_Master_Architecture_Baseline_v0.40_FINAL_PRECODING_FREEZE_RELEASE.docx, 175 pages; uploaded 2026-11-01. Parsed original text lines 901–1800. Historical source only: newer explicit instructions, verified latest code decisions and v0.41 authoritative narrow addendum `docs/MATURE_CONTENT_V041_PRODUCTION_FREEZE.md` and later explicit user decisions may supersede conflicting clauses. Original DOCX remains higher-fidelity for embedded images and precise tables.


26. Animation, cinematics and audio
CURRENT TIERING; PRODUCTION PIPELINE DETAILS REMAIN

26.1 Character animation
Playable 3D characters require reusable locomotion/combat rig infrastructure plus character-specific
layers. Full bespoke animation sets for every unit are not sustainable for an unbounded roster; rig
families and reusable primitives are mandatory.

26.2 Ultimate presentation tiers
 Tier                              Use                                        Typical presentation
                                                                              In-engine sequence or brief stylized cut-
 Standard                          Ordinary characters/abilities
                                                                              in.
                                                                              Bespoke short anime sequence, often
 Premium                           Major/rare characters
                                                                              around ~10 seconds when appropriate.
                                                                              Potential ~30-second premium anime
                                   Ultra-high rarity / above-ultimate
 Exceptional                                                                  sequence or comparable exceptional
                                   moments
                                                                              presentation.
                                                                              Full pre-rendered anime-style cinematic
 Story                             Major narrative moments
                                                                              package when justified.




26.3 Voice
Voice architecture should be supported from the start, but full voice coverage is not mandatory. Main
story, major characters, important relationship scenes and signature combat lines are the highest-value
targets. Routine/generated dialogue may remain unvoiced or use optional future local synthesis.


27. Asset architecture and content packaging
CURRENT PRINCIPLES
Assets are expected to be the dominant production bottleneck. The pipeline must maximize reuse of
technical infrastructure without forcing visual sameness.
 Asset family                                            Strategy
                                                         Shared rig families, common locomotion/IK/attachments,
 Playable characters
                                                         unique geometry/materials/signature animation.
                                                         Highly generic templates allowed; faction/world-specific
 Ordinary mobs
                                                         behaviors and appearance variants.
                                                         Richer individual presentation may be used according to
 Female ordinary NPCs
                                                         design needs; eligibility systems remain separate.
                                                         Culture-specific modular kits + procedural grammar + hero
 Architecture
                                                         landmarks.
                                                         Biome kits, PCG-assisted layout, authored anchors,
 World
                                                         deterministic regeneration where possible.

                                                   Page 18

<PARSED TEXT FOR PAGE: 19 / 175>

                                                         OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE
    Destruction                                            Reusable modular fracture/structural state templates.
    Cinematics                                             Separate cacheable/downloadable video packages.
    VFX                                                    Reusable library plus hero/character-specific signature effects.
                                                           Common banks + character/event packages; optional voices
    Audio
                                                           separated where practical.




27.1 Content manifests
Every package should have an ID, version, hash, dependencies, ownership/availability rules and
provenance. This enables validation, patching, eviction and redownload without turning the base APK
into a monolithic installation.


28. Storage and external archive policy
CURRENT BASELINE
The practical local target is at most roughly 100 GB, with a preference for a lower normal operating
footprint (for example 60-80 GB) to leave patch/cache headroom. Exact budgets remain subject to
profiling and asset measurements.

28.1 Local hot set
      Core engine/game/common assets
      Current/adjacent regions
      Current expedition and frequently used roster assets
      Current/recent events
      Frequently used cinematics and audio
      Patch/download working space

28.2 External cold storage
TeraBox or another external archive may hold source-quality assets, inactive regions, old cinematics,
archived builds and optional packages. It must not be a hard runtime dependency. The game should
remain fully functional with already-installed content while offline.


29. GitHub, remote Unreal build and CI architecture
CURRENT DIRECTION
GitHub is the development control plane: source, issues, branches, CI definitions, tests, implementation
history and build orchestration. The S26 Ultra is the gameplay/test target but cannot replace an Unreal
development workstation.
     Creative Director feedback
          -> engineering changes
          -> GitHub repository
          -> CI workflow
          -> persistent remote Unreal-capable runner
            - compile
            - import/generate assets
            - run tests
            - build world data/HLOD
                                                       Page 19

<PARSED TEXT FOR PAGE: 20 / 175>

                                                            OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE
          - cook/package Android
        -> build artifact/release
        -> install/test on S26 Ultra


29.1 Remote compute requirement
A persistent cloud or self-hosted runner with Unreal and the Android toolchain is required. GitHub can
trigger it remotely, but GitHub itself is not the Unreal compute resource. The runner can remain
powered off between builds if the chosen infrastructure supports that.

29.2 Repository design
Authoritative gameplay logic should prefer C++ and reviewable data definitions. Blueprint should
primarily serve presentation/composition where useful, not become the only home of critical rules.
Generated/imported binary assets must have reproducible metadata and validation scripts.


30. QA, simulation testing and sole-beta-tester workflow
CURRENT PRINCIPLE
Because the Creative Director is also the only human beta tester, human testing should focus on fun, UX,
aesthetics, surprise and design intent. Machines should test correctness, invariants, migration safety,
economy stability, simulation consistency and long-term behavior.

30.1 Automated test categories
      Combat formulas/effects and impossible-state checks
      Gacha pity/guarantees/currency conservation
      Save migrations, crash recovery and corruption detection
      Long-horizon 30/90/365-day and multi-year world simulations
      NPC/faction economic stability and resource conservation
      War outcomes and strategy regression scenarios
      Population/demography sanity
      Construction/destruction/reconstruction consistency
      Director constitutional constraints
      Character progression and broken ability-loop detection
      Performance smoke tests and content-package validation

30.2 Beta reporting
The game should include a one-action developer report feature that captures build number, save/world
snapshot reference, region, party, recent actions, logs, Director state reference, schema version and
reproduction metadata. This reduces the burden on the sole beta tester and makes bugs reproducible.


31. Feasibility matrix and major risks
CURRENT ASSESSMENT
    Requirement                        Assessment                              Primary mitigation
                                                                               Separate Ruler/World modes sharing one
    Portrait + landscape same game     High feasibility
                                                                               database.



                                                          Page 20

<PARSED TEXT FOR PAGE: 21 / 175>

                                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE
    WuWa-like open world on one flagship                                                   Single-device optimization, streaming,
                                            Feasible but demanding
    target                                                                                 HLOD, mobile rendering.
                                                                                           Region graph + packages rather than one
    Virtually unlimited world expansion     High feasibility
                                                                                           infinite map.
    Large evolving populations              High feasibility                               Cohort + fidelity-tier simulation.
                                                                                           LOD, Mass-style data orientation, capped
    30-100 nearby NPCs                      Feasible
                                                                                           full actors.
                                                                                           Tiered structural destruction, selective
    Extensive destruction                   Feasible after optimization
                                                                                           terrain changes.
    12-18 character turn battles            High feasibility                               Separate executor/auto mode.
    Large wars                              High feasibility if abstracted                 Strategic simulation + local embodiment.
                                                                                           Shared rules core + bespoke phase
    Hybrid action/turn bosses               Feasible, content-heavy
                                                                                           controller.
                                                                                           Rig families, modular assets, content
    Effectively unbounded 3D roster         Runtime feasible; production risk
                                                                                           pipeline.
                                                                                           Pool dialogue + event-driven NPU
    Local free-text NPC conversations       Feasible selectively
                                                                                           escalation.
                                                                                           Multi-resolution catch-up + history
    Centuries of history                    Feasible
                                                                                           consolidation.
                                                                                           Tiered cinematics, separate
    Anime cinematics at scale               Storage/production risk
                                                                                           packages/cache.
                                                                                           Remote Unreal runner handles
    Phone-only play/test                    Feasible
                                                                                           development/build.
                                                                                           GitHub-triggered remote Unreal build
    Literal phone-only Unreal development   Not feasible
                                                                                           infrastructure.




31.1 Largest remaining risks
       High-quality 3D character production at effectively unlimited roster scale
       Anime cinematic generation/QA/storage
       System interaction explosion: destruction -> logistics -> war -> politics -> demographics ->
        relationships
       Maintaining balance with highly divergent character growth and rule-breaking abilities
       Remote Unreal build compute cost/availability
       Thermal/performance stability of open-world rendering on the single mobile target
       Keeping content generation reproducible and safe for long-lived saves


32. Optimization decisions accepted
CURRENT OPTIMIZATION PASS
    #                                                                Accepted decision
                                                                     Tiered structural destruction instead of universal voxel
    1
                                                                     destruction.
                                                                     Huge expandable region graph instead of one literal infinite
    2
                                                                     map.
                                                                     Preserve traversal freedom while reducing arbitrary
    3
                                                                     geometry modification.
                                                                     Hierarchical NPC simulation; faction/world-specific behavior
    4
                                                                     packages; minimal AI.
                                                                     Species/faction-specific demographic models rather than
    5
                                                                     generic AI demography.
    6                                                                Abstract distant wars; instantiate only relevant local portions.
    7                                                                Keep 12-18 participant turn battles with auto mode.



                                                               Page 21

<PARSED TEXT FOR PAGE: 22 / 175>

                                                        OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE
                                                          Bound field/expedition roster selected before leaving territory;
    8
                                                          war handled in Ruler Mode.
                                                          Generic mob rigs/features allowed; technical rig families
    9
                                                          mandatory.
                                                          Branching bounded character growth ending in highly
    10
                                                          specific/original abilities.
                                                          Automated combat/balance simulation accepted; detailed
    11
                                                          rules to come later.
    12                                                    World Director constitutionally bounded.
    13                                                    AI inference event-driven and minimized.
                                                          Pool-based dialogue by default; AI escalation for
    14
                                                          important/high-value cases.
                                                          AI depth concentrated on important characters/diplomacy
    15
                                                          rather than everyone.
    16                                                    Cinematic quality tiers accepted.
                                                          Ultimates generally short; premium ~10s, exceptional above-
    17
                                                          ultimate content may reach ~30s.
    18                                                    Tiered voice coverage accepted.
                                                          Mobile-oriented rendering baseline; further margin allowed
    19
                                                          after profiling.
    20                                                    Perceptual graphics budget accepted.
    21                                                    30/60/120 FPS profiles accepted.
    22                                                    Ruler Mode aggressively unloads/suspends 3D world.
    23                                                    Directive/template-based settlement construction accepted.
    24                                                    Selective meaningful persistence accepted.
    25                                                    History compression/consolidation accepted.
    26                                                    Offline simulation resolution decreases with elapsed time.
                                                          External archive as cold storage; lower local operating
    27
                                                          footprint preferred; further margin allowed.
                                                          Spend complexity budget on world simulation/agency rather
    28
                                                          than brute-force graphical simulation.




33. Constraints and engineering invariants
CURRENT BASELINE
        No critical gameplay rule may exist only inside an opaque/manual Blueprint graph if it can be
         represented in C++/data and tested.
        No generated content failure may corrupt the base game or existing save.
        Asterion/content-generation tools are external pipeline capabilities, not required runtime
         dependencies.
        World simulation must remain valid when no physical Unreal actor is spawned.
        Every save schema change must be versioned and migration-tested.
        Every content package must be versioned, hashed and dependency-validated.
        Important random/adaptive systems should use recorded seeds or sufficient provenance for
         reproduction.
        World Director decisions must be auditable in developer mode even when hidden from normal play.
        No hidden anti-player rubber-banding may violate the economy/world rules.
        AI output must not override authoritative factual world state without an explicit system action.
        Canonical maturity/adulthood must be explicit lore data on Character Identity. For lore-adult
         characters, adult content is system-available by default; appearance is never an input and no second
         eligibility/permission classifier is added.

                                                      Page 22

<PARSED TEXT FOR PAGE: 23 / 175>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   External storage must never be required for already-installed core gameplay.
   Performance decisions are based on sustained profiling on the S26 Ultra, not desktop screenshots.


34. Intentionally open decisions
CURRENT CREATIVE BACKLOG
The following are the remaining material design areas after the later combat, character, equipment,
gacha, Domain, story, exploration, mature-content, economy, faction/war, UI, simplification and World
Fantasm checkpoints. Numerical tuning and engineering-owned implementation choices are excluded
from Creative Director questionnaires unless they materially change the player experience.
   World Mode action-combat grammar is defined by checkpoint AH, including character/action-
    dependent animation-cancel windows. Only device-profiled quantities such as preload/expedition
    sizes remain deferred.
   Starting-world and first-region content: biome/visual identity, geography, major landmarks, local
    threats, first factions/settlements, first dungeon/major encounter, and the actual Vertical Slice 0
    content roster.
   Opening/protagonist systemic rules are substantially defined by AI-AK. The actual authored opening
    narrative, local content, hidden mysteries and first production assets remain content work.
   Remaining cosmology/lore specifics: whether dimensions grant the preparation system consciously
    or automatically, the nature/origin of the true endgame enemies, and named cosmological/history
    details not required by the runtime architecture.
   Ruler progression macro is now resolved by checkpoint AL: no universal fixed ladder; continuous
    sovereignty state plus major recognized milestone titles. Only milestone catalog/content and
    presentation details remain.
   World Director content-policy details: event/banner cadence, release rhythm, surprise rules, and the
    initial prepared content pool. Exact rates and economy numbers remain simulation/tuning work.
   Presentation identity: final art direction, character/environment rendering target, VFX language,
    World Fantasm visual language, cinematics, voice/audio strategy, and asset-production methodology.
   UI finishing decisions: the other four primary Ruler Mode bottom-navigation destinations, exact
    Territory/Domain information layout, World Mode HUD/control layout, and final portrait-to-
    landscape transition presentation.
   Content catalogs rather than new universal systems: starting characters, enemies, bosses,
    equipment, materials, species, factions, locations, stories, adult-content scenes/states, and World-
    Fantasm-specific authored content.
   Starting-world calendar/time values and environment-specific time ratios where actual content
    needs them; the generic nested-time architecture is already settled.
   Performance-dependent quantities such as maximum expedition/preload roster size are deliberately
    deferred to real S26 Ultra profiling rather than decided as creative guesses.
   Balance-only values such as exact pull rates/soft-pity curves, stat coefficients, damage formulas,
    duplicate conversion quantities and resource income are not missing creative architecture; they are
    later simulation/tuning work.




                                                 Page 23

<PARSED TEXT FOR PAGE: 24 / 175>

                                                                              OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE




35. Recommended refinement order
CURRENT NEXT PHASE
The remaining creative work should now focus on content-defining and player-facing gaps rather than
reopening systems that later checkpoints already settled. Recommended sequence while the
Unreal/device gate is pending:
1. Detailed World Mode action combat.
2. Starting region, opening sequence and first vertical-slice content.
3. Cosmology/endgame-enemy lore and protagonist starting progression.
4. Ruler progression names/requirements and World Director content cadence.
5. Final Ruler Mode navigation, World Mode HUD and control presentation.
6. Art direction, VFX language, voice/audio and cinematic presentation.
7. First production content catalogs: characters, enemies, bosses, equipment, factions, locations and
   authored World Fantasms.
8. Numerical balance/economy tuning only after deterministic simulation and real-device profiling
   provide evidence.




Appendix A. Current system diagrams
REFERENCE


A.1 High-level runtime
     Creative Director rules
           |
           v
     World Director constitution                  GitHub / build pipeline
           |                         |
           v                         v
     +------------------ Authoritative Game Core ------------------+
     | World | Characters | Combat | Economy | Factions | History |
     +--------------------------+-----------------------------------+
                          |
                         SQLite
                          |
           +------------------+------------------+
           |                               |
       Ruler Mode                               World Mode
       portrait UI                             landscape 3D
       strategy/gacha                            exploration/action
           |                               |
           +------------------+------------------+
                          |
                      same world state


                                                                            Page 24

<PARSED TEXT FOR PAGE: 25 / 175>

                                                               OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

A.2 Simulation fidelity ladder
  Aggregate population
      -> lightweight persistent individual
           -> named individual
             -> important NPC
               -> embodied Unreal actor


  Promotion and demotion preserve important history while changing simulation cost.


A.3 Combat execution
  Shared combat definitions
      +-> Action executor (mobs/wildlife/exploration)
      +-> Turn executor (6 frontline + reserves)
      +-> War simulator (large strategic forces)
      +-> Hybrid phase controller (bosses / bespoke encounters)


A.4 Content packaging
  Source / generation pipeline
      -> validation
      -> content package (version/hash/dependencies)
      -> remote Unreal cook/build
      -> local device cache
      -> optional cold archive for inactive/source content



Appendix B. Preliminary data domains
PROVISIONAL
Domain                                                           Examples
Identity                                                         entities, characters, forms, species, factions, rulers
                                                                 regions, cells, settlements, structures, territory, resources,
World
                                                                 biome state
Population                                                       cohorts, individuals, births/deaths, genealogy, migration
                                                                 relationships, loyalty, contracts, knowledge, beliefs, rumors,
Social
                                                                 memories
                                                                 instances, progression, skills, branches, equipment, states,
Character gameplay
                                                                 resurrection
Combat                                                           effects, abilities, encounters, battle records, war fronts
Economy/gacha                                                    currencies, banners, pity, pulls, upgrades, shops, logistics
Politics                                                         alliances, wars, diplomacy, leadership, treaties, claims
                                                                 directives, jobs, workers, materials, progress,
Construction
                                                                 damage/reconstruction
                                                                 hidden schedules, event candidates, sealed packages, rule
Director
                                                                 constraints, audit log
                                                                 major events, summaries, milestones, succession, territory
History
                                                                 changes
Content                                                          package manifests, assets, versions, dependencies, provenance
System                                                           schema versions, migrations, snapshots, backups, diagnostics




                                                             Page 25

<PARSED TEXT FOR PAGE: 26 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

Appendix C. Decision log
REFERENCE

This appendix records the highest-level sequence of decisions so later refinements do not accidentally
revert them.
 Decision                                              Result
                                                       Supersedes the older starter where conflicts exist; design-
 Architecture starter v3
                                                       definition mode before implementation.
                                                       Unreal became preferred after the project was clarified as a
 Engine exploration
                                                       high-fidelity 3D open-world personal game.
                                                       Mix specialized runtimes/services, not multiple co-equal game
 Hybrid technology
                                                       engines.
                                                       Player is a customizable growing ruler; NPC simulated players
 World/player model
                                                       are rulers/entities in the same world.
                                                       Portrait Ruler Mode and landscape World Mode are both first-
 Dual-mode game
                                                       class and share state.
                                                       Effectively unlimited expansion through huge streamed
 World scale
                                                       regions/packages.
                                                       Hierarchical fidelity and deterministic systems are preferred
 Simulation
                                                       over pervasive AI.
                                                       Action + strategic turn + war simulation, with hybrid boss
 Combat
                                                       phases.
                                                       Start from nothing; directive-based settlement/kingdom
 Territory
                                                       development executed physically by NPCs.
                                                       Canonical death, geopolitical rise/fall and meaningful losses;
 Consequences
                                                       no secret anti-player rubber-banding.
                                                       Tier destruction, abstract distant war, pool dialogue, tier
 Optimization pass                                     cinematics/voice, selective persistence/history and variable
                                                       catch-up fidelity.
                                                       Single S26 Ultra target; 30/60/120 FPS modes; additional
 Performance
                                                       margin allowed when profiling requires it.
                                                       Minimize AI; use it only where free reasoning materially
 AI
                                                       improves important interactions.
                                                       GitHub controls remote Unreal build/test pipeline; phone
 Build infrastructure
                                                       remains play/test target.




End state of this baseline
      This document freezes the macro-design and engineering direction only. It is intentionally not yet
      the final GAME_DESIGN_SPEC, COMBAT_SPEC, ECONOMY_AND_GACHA_SPEC,
      NPC_SIMULATION_SPEC or WORLD_AND_STORY_SPEC. Those should be derived during the next
      refinement passes without silently weakening the decisions captured here.




                                                   Page 26

<PARSED TEXT FOR PAGE: 27 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

Checkpoint 2026-09-29 - Sections A-G
CURRENT DESIGN CHECKPOINT
This checkpoint supersedes conflicting details in earlier sections of this baseline. It records the
normalized decisions made during the A-G design questioning and the explicit scope correction that the
project remains a gacha/open-world RPG with strategic territory simulation, not a life/development/legal
simulator.

A. Cosmology, Rulers and Contracts
   Cosmology is a graph of universes, dimensions, worlds and sub-realms; different worlds may have
    different laws and power ceilings.
   World/dimension junctions are progression-gated by relative world rank/power compatibility;
    irregular events can temporarily violate this.
   Ruler is a status, not a combat class. Ruler status requires actual territorial authority/control and
    sufficient personal/Authority capability to establish and keep that territory. A subordinate contract is
    NOT required: an individual may become and remain a Ruler alone if they can hold their territory
    through sheer strength or other valid means.
   NPC rulers use the same systemic rules as the player. Multiple rulers may coexist only through
    shared, mixed, partitioned or otherwise valid joint territory structures.
   Ruler authority is strongest within owned territory. Ruler tiers/classes may exist; subordinating
    several rulers can produce an Overlord-type hierarchy.
   Contracts are reality-binding subordinate contracts. Default contract establishment requires valid
    contractor/contractant contact and blood, with remote/territory-mediated mechanisms possible at
    higher authority.
   A contract cannot normally be broken by the contractant; transfer, inheritance, absorption,
    corruption or override can occur through valid higher-order mechanics.
   One individual normally has one active ruler contract unless the apparent character is canonically
    multiple independent individuals.
   On ruler death, territory/contracts can pass through a valid dynasty/succession path or become
    neutral/unclaimed and vulnerable to absorption.
   Rarity is a common in-world grading system but is relative to origin/species and does not equal
    actual combat power. UI gacha is a presentation abstraction over canonical contract mechanics.
   No supreme administrator/god tier automatically sits above rulers; gods and other entities can
    themselves be rulers, subjects or independents.
   Protagonist simply wakes in the setting; there is no mandatory chosen-one guarantee.

B. Protagonist
   Protagonist is an adult male and uses predefined selectable body/face/hair/voice options; appearance
    can later be changed.
   Initial race is Unknown. Race/species development occurs by acquiring Factors that can hybridize,
    coexist, conflict and alter appearance, biology, lifespan and abilities.
   Aging is factor/species dependent. Agelessness does not imply combat immortality.
   Protagonist death has two major paths: revival/continuation when a valid factor, state, loyal-
    character mechanic or stored power permits it; otherwise Regression.



                                                  Page 27

<PARSED TEXT FOR PAGE: 28 / 175>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Regression does not rewind world history. The previous incarnation remains historical; certain soul-
    bound powers, objects or mechanics may cross into the new beginning.
   Protagonist progression is distinct from ordinary collectible-character progression. He has no gacha
    rarity, but can have class, existence/power rank, ruler rank, skills, factors and equipment.
   Protagonist can learn broad skills through exploration and systems, and also has a separate ruler-
    development axis.
   Population size does not directly increase personal combat stats; larger domains increase
    geopolitical influence, resources and access to opportunities.
   Personal combat strength still matters politically: a weak ruler can govern brilliantly but remains
    vulnerable in direct conflict.
   Abdication/succession is permitted. Ascension to higher worlds can leave earlier territories to
    successors/dynasties while the protagonist continues upward.
   Truly unique protagonist abilities are supported.

C. Time, Calendars and History
   Every domain may have its own time ratio. Time hierarchy supports universal chronology,
    dimension time, regional/local modifiers and individual time effects.
   Sleep uses selected duration and can be dangerous in unsafe locations. Long
    cultivation/hibernation/rest states are allowed; regents/co-rulers may govern during absence.
   Physical travel consumes time. Same-dimension teleport can be instantaneous; cross-dimensional
    travel uses a breach/transition and is not inherently instantaneous.
   Seasons and calendars are world/civilization specific. Individual biological aging follows time
    personally experienced, modified by species/factors/abilities.
   Ruler Mode strongly slows time. Turn-based battle freezes or nearly freezes the local external world;
    action combat advances normal local time unless a mechanic modifies it.
   Offline/catch-up evolution is progressive and proportional to actual local elapsed time. No arbitrary
    long-absence time cap is imposed; simulation resolution becomes more abstract over huge
    timescales.
   There is no ordinary administrative fast-forward button. Time advances through actual waiting,
    travel, rest, cultivation or time-manipulation mechanics.
   Time-manipulating abilities can be extreme. Local/individual rewinds are supported conceptually;
    full history rewrites remain bespoke/TBD to avoid universal paradox simulation.
   An automatic Chronicle records eras, rulers, wars, civilizations, discoveries and other meaningful
    historical events.

D. Geography, World Generation and Ecology
   Starting region is dangerous wilderness where the protagonist begins essentially alone; first
    seamless region target is roughly 10-20 km-scale perceived space, expandable later.
   Oceans are physical/explorable; underwater, underground, floating, inverted, sky, space, pocket-
    realm and other unusual environments are supported.
   Outer space and other planets/stellar civilizations are eventually playable. Pocket dimensions may
    become territories.
   World-generation method varies by region: authored anchors plus procedural terrain/content where
    useful. Generated geography becomes canonical once established.
   Geology/climate can change through events: volcanoes, earthquakes, floods, climate shifts,
    magical/eldritch transformations and civilization terraforming.
                                                 Page 28

<PARSED TEXT FOR PAGE: 29 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Resources are type-dependent: some finite, some renewable, some unique. System must avoid hard-
    locking the save because a globally essential resource vanished; multiversal
    alternatives/substitutions can exist.
   Forests/ecosystems can recover. Local extinctions should generally be avoided as routine outcomes
    because species content is valuable; total global extinction is not a normal system goal.
   Invasive species and ecosystem interactions are supported at region-appropriate fidelity rather than
    universal high-detail ecology.
   Settlement growth can change ecology. Weather importance is region-dependent; severe weather
    can threaten flight and traversal.
   Environmental danger is physical, not level-gated. Warnings should be diegetic/internal
    thoughts/equipment/companion knowledge rather than omniscient system popups when possible.
   Procedural minor settlements can grow into important cities. Roads can emerge/evolve from usage
    and investment. Borders are primarily strategic/map abstractions with physical/magical
    manifestations where lore supports them.

E. Exploration and Traversal
   Exploration freedom is systemic: sprinting is unrestricted outside demanding situations; extended
    climbing/travel produces fatigue, food/sleep needs rather than a constant exploration stamina tax.
   Swimming/endurance is species/factor dependent. Any sufficiently deep water is potentially
    diveable; drowning exists when no valid adaptation/equipment prevents it.
   Flight model depends on source. Unlocking real flight is allowed to bypass terrestrial obstacles;
    aerial mobs, hazards and anti-air threats preserve challenge naturally.
   Mounts are real entities and may fight, be injured, die, breed and evolve. Vehicle architecture exists,
    but modern mundane traffic is not a core aesthetic target; ships, airships/zeppelins, fantastical/SF
    vehicles and spacecraft are supported.
   Teleport networks are strategic infrastructure that can require costly materials/energy and can be
    captured, sabotaged or destroyed.
   Enemies can pursue across traversal modes if capable. Combat can continue in air/water/climbing
    contexts when supported by participants.
   Parkour remains WuWa-like rather than a dedicated parkour simulator. Grappling/teleport
    dashes/water walking/phasing/tunneling/giant forms and other traversal abilities are supported as
    kit mechanics.
   Physical power can solve physical barriers directly. Nearly every plausible building is enterable;
    important interiors are authored, ordinary interiors use procedural kits.
   Breaking into/through structures is mechanically allowed. Reactions are handled through
    lightweight hostility/reputation/diplomatic consequences rather than a detailed legal simulator.

F. Crime/Law Scope Correction
   DELETED as a detailed subsystem. The game will not simulate legal codes, forensic procedure,
    courts, extradition frameworks, crime statistics or legal micromanagement.
   Only lightweight consequences remain: restricted areas where relevant, known hostile actions,
    bounties/hostility where gameplay-relevant, faction/territory reputation shifts and diplomatic
    consequences.
   Fate-style Lawful/Neutral/Chaotic-type alignment may exist as descriptive character tendency, not a
    universal Good/Evil score.


                                                  Page 29

<PARSED TEXT FOR PAGE: 30 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Actions are not blocked merely because they are illegal; when relevant, factions/territories may
    react. Many territories are lawless.
   No dedicated imprisonment, subordinate-liability, sovereignty-law or disguise-law subsystems are
    required as core architecture; these may exist only as bespoke events/abilities where useful.

G. Territory and Settlement Management - RPG Scope
   The protagonist begins with no territory. Territory-claiming method varies by world/system:
    occupation, domain mechanics, conquest, ruler recognition or other lore-valid methods.
   Borders can overlap and be disputed. Territory ownership does not vanish merely because it is
    ruined; ruined territory is a valid persistent territorial state, though functioning territory requires
    maintenance.
   There is no artificial territory-size cap. High ruler tiers/Overlords may hold territories spanning
    worlds or dimensions.
   Common strategic resources stay broad: food, raw/construction materials, wealth/currency,
    energy/power and special resources. Regions/factions add unique monopoly resources (e.g. dragon
    scales) and unique energy sources.
   Strategically meaningful logistics can be physical (caravans, shipments, routes) with costs and raids;
    low-value logistics remain aggregated.
   Construction consumes resources and uses ordinary NPCs without a visible worker-count
    simulation. What can be built depends on culture, knowledge, technology, magic and acquired
    blueprints/techniques.
   Player uses detailed high-level directives rather than placing every object. Named/characterized
    characters can be assigned as governors, architects, generals, treasurers, diplomats or other strategic
    roles. Routine administrators can act autonomously within granted authority.
   Settlements can specialize and use macro-districts. Only food, housing and safety are universal
    population needs in the common model; sanitation/education/crime-simulation subsystems are
    explicitly excluded.
   Religion/culture/social hierarchy/economic systems may matter at a high strategic level and vary
    radically by civilization, but are not life-simulator micro-systems.
   Cultures can mix/assimilate over time. Revolts and independence are allowed. Destroyed settlements
    can rebuild, migrate, be abandoned, disappear into ruins and later be reclaimed or occupied.
   No mandatory permanent capital. Ruler Mode includes a live strategic map for borders, settlements,
    armies, resources, construction and threats. World Mode expresses territory mainly through
    physical world signs rather than artificial glowing borders.
   GLOBAL SCOPE RULE: territory systems exist to support gacha/RPG progression, faction strength,
    wars, character use, exploration and geopolitics. They must not expand into a standalone
    city/life/development simulator.

Checkpoint feasibility guard
No accepted A-G decision is currently a fundamental feasibility blocker. Expensive systems must use
abstraction/tiering: event-driven geology, region-based world packages, hierarchical NPC/population
fidelity, aggregated distant logistics/warfare, procedural ordinary interiors, reusable traversal/vehicle
frameworks and strategic rather than life-simulation territory data. Any future RED feasibility item must
be challenged immediately rather than silently accepted.



                                                  Page 30

<PARSED TEXT FOR PAGE: 31 / 175>

                                                      OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

Checkpoint 2026-09-30 - Combat Architecture Refonte
Status: AGREED BASELINE / PROVISIONAL WHERE NOTED. This checkpoint supersedes earlier combat
assumptions where they conflict.

H. Core combat identity
   The game uses one canonical character/skill/effect definition with separate executors for turn
    combat, World Mode action combat, and large-scale war simulation. A power remains canonically
    the same power even when its spatial/execution representation changes by mode.
   Primary strategic character combat is a high-complexity gacha turn-RPG, inspired structurally by
    systems such as HSR, Shoujo Kaisen, 7DSGC, Fate/Grand Order, Tensura: Isekai Memories,
    Echocalypse, Dokkan and OPTC rather than by tactical-grid combat.
   World Mode remains third-person action combat centered on the Ruler. BBS is not a visual target.
   Combat is intended to reward team composition, adaptation and build analysis rather than simple
    Combat Power/stat checks.

H.1 Turn-team structure: 18 characters, 6 active
   A complete strategic battle team contains 18 characters arranged as three sequential waves/rows of
    six: first line, second line and third line.
   Only six characters are active/on-field at a time. If the first six finish the battle, the second and third
    lines may never act.
   The second and third lines are reserves/succession depth, not additional simultaneously rendered
    tactical rows.
   The default succession model is positional/lane-based: when an active character is defeated, the
    corresponding next-line character automatically enters; the third-line counterpart follows if the
    second-line character is later defeated.
   Manual play may permit voluntary reserve substitution when the relevant battle rules allow it. Auto
    play follows configured tactics/presets.
   Reserve characters are normally inactive unless their own kit explicitly grants reserve/off-field
    effects (for example a sniper firing from reserve).
   There is no general front-row tanking/reach geometry. Slot/position can matter when a particular
    skill explicitly references positions, lines, adjacency, lane or target geometry.

H.2 Turn order and action flow
   Turn combat uses a continuous speed/action-value timeline rather than full-team phases.
   Speed and action-value manipulation can alter action frequency/order. Kits may advance, delay,
    insert, interrupt, counter, follow up, gain extra actions or otherwise alter the timeline.
   Ultimates and extraordinary actions may be able to insert themselves outside the normal turn
    sequence when their rules permit it; this is a default capability, not an inviolable rule.
   Normal retreat is not a universal combat button. Escape/withdrawal requires an encounter rule,
    physical opportunity, skill or other explicit mechanic. Surrender is contextual and uncommon:
    intelligent enemies may surrender when personality, orders, morale, objectives or overwhelming
    circumstances justify it, but enemies do not generically surrender just because they are losing. An
    enemy surrender does not force the player to spare them.


                                                    Page 31

<PARSED TEXT FOR PAGE: 32 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

H.3 Character kit structure and passives
   A developed character generally targets roughly 8-12 meaningful kit components, with variation by
    rarity, evolution, corruption, form and character concept. The UI should not expose 18 ordinary
    buttons.
   A typical kit may include a basic attack, multiple active skills, an ultimate, several passives, special
    states/forms, entry/reserve/switch mechanics and exceptional rank-specific rules.
   Passives are never player-activated. A passive is always active while conditions are met, or
    automatically triggers/reacts to an event. Player-selected activation is an active
    skill/ultimate/command, not a passive.
   Progression can add new mechanics, passives, skill branches or rule changes rather than only
    increasing multipliers.
   Alternate/evolved/corrupted forms may completely change role, moveset, model, stats and
    mechanics.

H.4 Stats, probability and damage
   Universal base stats include HP, ATK and DEF, with conventional special stats such as Speed, Crit
    Rate, Crit Damage, Crit Resistance, Dodge, Accuracy, penetration/resistance, effect
    accuracy/resistance, healing modifiers and future system-specific stats.
   Many special stats are percentages and interact with seeded RNG. Crit Rate is a probability of
    triggering a critical hit; Dodge and proc/debuff chances can work similarly. Builds materially alter
    those probabilities.
   At minimum the damage system supports Physical and True damage. Other worlds/power systems
    can introduce their own mechanics rather than being forced into one universal elemental wheel.
   There are no universal artificial damage or stat caps. Individual skills/rules may impose caps (for
    example, no more than 12% Max HP lost per turn), and other sufficiently specific effects may bypass
    those caps.
   Power/Existence Rank gaps can be enormous. A moderately stronger opponent may be beaten by
    exceptional preparation and counters; extreme rank gaps can be functionally impossible.

H.5 Effect architecture: families, evolutions and derivations
   Avoid combat tags that carry no meaningful mechanical information merely because of lore/source
    (for example 'Dragon' or 'Technological' as automatic debuff categories). Lore metadata may still
    exist for Codex/search/provenance without itself creating combat behavior.
   Effects should be recorded explicitly and organized into meaningful families/lineages where useful.
   Example progression: Burn -> Ignite -> Calcination can represent increasingly advanced/evolved
    members of one effect family.
   Derived variants can branch rather than merely rank up. Example: Dark Ignition may combine a
    strong Burn component with stat debuffs and distinct resistance/counter behavior.
   Elemental charts can produce common effect branches, but unique powers are allowed to create
    independent or derived effects outside that chart.
   Effects can carry potency/level, stacks, duration, source, dispel/resistance class, authority and other
    mechanically useful parameters.
   Substantial buff/debuff stacking and interaction depth is allowed. UI presentation must
    summarize/group effects rather than artificially limiting mechanics to keep icon counts small.


                                                   Page 32

<PARSED TEXT FOR PAGE: 33 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

H.6 Rule-breaking and extreme kits
   The combat engine explicitly supports rule-breaking abilities: immortality, revival, infinite or
    repeated respawn, execution/instant death, existence erasure, invulnerability, reflection, immunity,
    effect negation, damage caps, cap bypass, control immunity and immunity bypass, among others.
   Conflicting absolute-looking effects are resolved by a formal deterministic rule/authority system
    using explicit counters, effect authority/tier, specificity and other declared resolution rules rather
    than Blueprint execution order.
   Supports can manipulate rules themselves (for example suppressing enemy effects or increasing the
    authority/effectiveness of allied effects).
   Bosses should normally use resistance, diminishing effectiveness, phase rules or explicit lore
    immunity rather than generic immunity to every interesting mechanic.

H.7 Synergy and team construction
   Team synergy is central. Supported forms include effect combos, timeline combos, faction/team
    bonuses, predecessor-successor planning, reserve interactions, character-pair/group synergies and
    bespoke fused/combo attacks.
   Hard counters are allowed because the game is team-based rather than 1v1. Ordinary content should
    still avoid requiring exactly one specific character unless deliberately designed as special/puzzle
    content.
   The 18-character team should be understood as six deep succession chains: opening unit -> second-
    wave successor -> third-wave successor. The quality and order of those chains are part of build
    strategy.

H.8 Defeat, recovery and canonical death
   0 HP normally means defeated/incapacitated, not canonical death.
   A defeated character becomes invalid for deployment until fully healed/recovered unless a skill,
    revival rule or special mechanic changes that state.
   Canonical death only occurs when the encounter/world context actually allows lethal consequences.
   Characters may possess one-time, repeated or effectively infinite revival/respawn mechanics as part
    of their kits; other effects may counter resurrection.

H.9 World Mode active combat and dispatch separation
   World Mode active combat is protagonist-centric: the Ruler plus up to two selected characters form
    the immediate active party, using the previously agreed switching/QTE-style transition mechanics.
   Only the currently controlled actor normally needs to be physically active at once; assists, switch
    attacks, temporary overlap and combination attacks may briefly involve the others.
   A 12-character expedition belongs to Ruler Mode as a dispatch/assignment system. It is not the World
    Mode active-combat party.
   Dispatches can resolve exploration/resource/missions strategically and return rewards, injuries,
    discoveries and events according to their systems.
   Summoning is primarily relevant to active World Mode combat, where summoned entities/familiars
    can physically participate. Turn-mode summoning is exceptional and should not become a default
    subsystem unless a character/boss specifically requires it.



                                                  Page 33

<PARSED TEXT FOR PAGE: 34 / 175>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

H.10 Damage presentation and battle analytics
   Damage scale is large but conventional rather than hyper-exponential. Values such as 62B or 401Q
    are representative of late/high-power presentation.
   On-field damage text should remain visually compact, with roughly ten displayed digits/characters
    before abbreviated suffix/scientific-style formatting is used.
   Internal numeric representation must safely exceed 64-bit integer ranges without requiring exotic