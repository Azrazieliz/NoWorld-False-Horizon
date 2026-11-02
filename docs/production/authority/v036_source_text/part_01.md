# v0.36 full source text — segment 1
Original: Offline_Adult_Gacha_RPG_Master_Architecture_Baseline_v0.36_checkpoint_RulerOverlord.docx (143 pages), uploaded 2026-11-01. Derived text extraction, not a replacement for original embedded figures/tables. SOURCE LINE WINDOW 1–1000. Historical decisions may be superseded by 7★ overlays.

<PARSED TEXT FOR PAGE: 1 / 143>

                                                  OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE




   OFFLINE ADULT GACHA RPG
           Master Architecture & Design Baseline
   Version 0.36 - Current Authoritative Baseline (mutable) + Ruler / Overlord sovereignty checkpoint



   Purpose: capture the complete current agreement before further refinement. This document is a
   baseline, not a final Game Design Document. Items explicitly marked OPEN remain intentionally
   undecided; they must not be silently invented during implementation.



                                           Authority basis
1. OFFLINE_ADULT_GACHA_RPG_ARCHITECTURE_STARTER_v3.md (governing starter where conflicts
                                                 exist)
           2. Current design-definition conversation and subsequent optimization decisions
 3. Older Offline_Gacha_RPG_Architecture_Starter.md only where it does not conflict with v3 or later
                                               decisions



                                           Design roles
Role                                                Responsibility
                                                    Concept, creative authority, requirements, priorities,
User / Creative Director
                                                    approvals, game direction, sole human beta testing.
                                                    Architecture, implementation, coding, tooling,
Engineering / production assistant                  automation, tests, CI/builds, maintenance, debugging,
                                                    data pipelines and technical documentation.
                                                    Autonomous in-game live-ops/world-management
World Director (software)                           subsystem operating strictly inside rules defined by the
                                                    Creative Director.




                                                Page 1

<PARSED TEXT FOR PAGE: 2 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

Document map
   1. Executive definition and design principles
   2. Decision status and current baseline
   3. Product identity and player fantasy
   4. Dual-mode interaction model
   5. Runtime and engine architecture
   6. World topology and spatial architecture
   7. Procedural/authored world production
   8. Territory, construction and reconstruction
   9. Destruction model
   10. Civilizations, factions and asymmetric development
   11. Population, demographics and generations
   12. NPC embodiment, behavior and simulation fidelity
   13. Knowledge, memory, rumors and dialogue
   14. AI/NPU policy
   15. Character ontology, contracts and roster scale
   16. Character growth and build divergence
   17. Combat architecture
   18. War and geopolitical simulation
   19. Difficulty, failure and persistence
   20. Gacha, banners and World Director
   21. Time, offline catch-up and endless history
   22. Persistence/database architecture
   23. Graphics and rendering strategy
   24. Performance and thermal strategy
   25. UI/orientation and Ruler Mode
   26. Animation, cinematics and audio
   27. Asset architecture and content packaging
   28. Storage and external archive policy
   29. GitHub, remote Unreal build and CI architecture
   30. QA, simulation testing and sole-beta-tester workflow
   31. Feasibility matrix and major risks
   32. Optimization decisions accepted
   33. Constraints and engineering invariants
   34. Intentionally open decisions
   35. Recommended refinement order
   Appendix A. Current system diagrams
   Appendix B. Preliminary data domains
   Appendix C. Decision log




                                                  Page 2

<PARSED TEXT FOR PAGE: 3 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

1. Executive definition and design principles
CURRENT BASELINE
The project is an offline-first, Android-first, persistent single-player RPG designed to create the
experience of a living online gacha/live-service world without requiring a public backend. The player is
not merely an adventurer collecting units; the player is a ruler who begins from essentially nothing,
develops personal power, recruits characters through an in-world contract/subordination system, builds
a territory, manages a faction, explores an effectively unbounded world, and lives inside a history that
continues to evolve even during long absences.
The intended product combines two equally important experiences: a portrait-oriented strategic Ruler
Mode and a landscape-oriented 3D World Mode. Both read and write the same authoritative world state.
The first focuses on roster, strategy, management, gacha and large-scale systems; the second focuses on
embodied exploration, active combat, traversal and physical interaction with the world.

    Primary design rule: simulate the consequences that matter, not every physical detail literally.
    The game should appear deeper than the hardware can brute-force because state, causality and
    history remain coherent across different simulation fidelities.




1.1 Core experience goals
   An open world with minimal artificial barriers: if a location is physically reachable, traversal
    systems should normally allow it.
   A world that continues existing independently of the player: factions rise and fall, NPCs act, wars
    occur, territories change and reconstruction happens.
   A large and ultimately unbounded roster whose characters remain mechanically relevant through
    evolving progression rather than permanent rarity obsolescence.
   Gacha tension without real-money monetization, combined with an autonomous World Director so
    future banners/events can remain surprising to the Creative Director while playing.
   Deep strategy in Ruler Mode and satisfying WuWa-like active control in World Mode, sharing the
    same characters and combat rules where practical.
   Consequential failure without secret pity or rubber-banding: poor strategic decisions can produce
    lasting losses that must be recovered from legitimately.
   AI only where it adds value. Most behavior, demographics, economy, routine dialogue and combat
    logic should be deterministic/systemic and testable.
   Long-term maintainability through data-driven definitions, reusable technical rigs, automated
    validation and GitHub-controlled build/test pipelines.

1.2 Explicit non-goals
   No public live-service backend is required for core play.
   No requirement to support weaker Android devices; the current Samsung Galaxy S26 Ultra is the
    baseline device.
   No requirement for every NPC to run a language model, full AI planner or full physical actor
    simulation.
   No requirement for arbitrary Minecraft-style voxel destruction of the entire landscape.
   No requirement for a single physically infinite Unreal map.
                                                   Page 3

<PARSED TEXT FOR PAGE: 4 / 143>

                                                            OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

        No requirement for every minor character to receive expensive anime-length cinematics or full
         recorded voice acting.
        No requirement for Unity or Godot to ship as co-equal runtime engines.


2. Decision status and current baseline
CURRENT BASELINE
    Area                                Current baseline                               Status
                                        Unreal Engine as the only shipped game
    Game engine                                                                        CURRENT
                                        engine.
                                        Android-first; S26 Ultra as the single
    Platform                            performance baseline. Windows may              CURRENT
                                        exist later for tooling/testing.
                                        Portrait Ruler Mode + landscape World
    Orientation                                                                        CURRENT
                                        Mode in the same game and same save.
                                        Very large seamless regions linked into
    World                                                                              CURRENT
                                        an effectively unlimited world graph.
                                        Hierarchical fidelity: statistical/cohort ->
    Simulation                          lightweight individual -> named ->             CURRENT
                                        important -> embodied.
                                        Minimized, event-driven, reserved for
    AI                                  situations that benefit from free              CURRENT
                                        reasoning.
                                        Pool-based by default; generative
    Dialogue                            dialogue only for important                    CURRENT
                                        characters/situations.
                                        Tiered structural destruction; no
    Destruction                                                                        CURRENT
                                        arbitrary universal voxel destruction.
                                        Action + strategic turn-based + war
    Combat                                                                             CURRENT
                                        simulation; hybrid boss phases allowed.
                                        SQLite-centered world database,
    Persistence                         canonical history, versioned migrations        CURRENT
                                        and snapshots.
                                        ~100 GB maximum practical local
    Storage                             footprint; external archive is optional        CURRENT
                                        cold storage only.
                                        GitHub as control plane + remote
    Build workflow                                                                     CURRENT
                                        persistent Unreal-capable build runner.
                                        Rates, damage formulas, exact
    Detailed formulas/lore              calendars, species rules, economy              OPEN
                                        numbers, etc.




3. Product identity and player fantasy
CURRENT BASELINE

3.1 The player as ruler
The protagonist is a customizable and continuously growing ruler. Personal power matters because
ruling increasingly powerful subjects and surviving geopolitical conflict requires more than
administrative authority. The protagonist should be capable of direct exploration and combat while also
commanding a growing polity.

                                                          Page 4

<PARSED TEXT FOR PAGE: 5 / 143>

                                                                OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE
    individual -> tiny group -> camp -> village -> settlement -> town -> city -> territory -> kingdom -> potentially larger
    polity

The exact names and thresholds are open, but the progression fantasy is not: the player starts from
essentially nothing and can eventually become the center of a major civilization or empire. Growth
should be earned through world systems rather than granted because the game detected the player
falling behind.

3.2 In-world gacha ontology
Gacha should exist as a coherent law/mechanic of the setting. The current direction is a contract of
subordination/allegiance: pulling a character does not need to create them ex nihilo; it establishes a
valid relationship that allows the ruler to recruit, deploy or otherwise bind that character to the faction.
NPC rulers participate in the same broad system.
The purpose of the dimensional contract/gacha preparation mechanism is now defined: dimensions
grant/provide it as preparation against extremely powerful true endgame enemies. The remaining lore
ambiguity is whether dimensions do this consciously or whether it is an automatic multiversal defensive
law. Recruitment status, physical world existence and playable access remain separate fields so
characters can exist canonically before being recruited.

3.3 Persistent history rather than replay loop
The game is not designed around repeated new games or seasonal resets. History accumulates.
Characters can die canonically, rulers can fall, settlements can be destroyed and rebuilt, relationships
can change and cultures can develop. Old state is not erased simply because the player was absent.


4. Dual-mode interaction model
CURRENT BASELINE

4.1 Ruler Mode - portrait
Ruler Mode is a first-class game, not a secondary menu. It should be comfortable to play for long sessions
without loading the 3D world. This is the Shoujo-Kaisen-like strategic/gacha layer.
    Roster and character management
    Gacha pulls and banner history
    Equipment, builds and progression
    6-character frontline + reserve turn battles
    War planning and large-scale strategic battles
    Territory management and construction directives
    Dispatches and assignments
    Guild/faction administration
    Diplomacy, reports, rankings and geopolitics
    World Director event/banners presentation
    Character relationships and appropriate interactions
    Economy, resources, shops and logistics




                                                              Page 5

<PARSED TEXT FOR PAGE: 6 / 143>

                                                               OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

While Ruler Mode is active, expensive 3D World Mode systems should be unloaded or suspended
whenever possible. This reduces heat and battery usage and allows heavy simulation/database tasks to
run without competing with open-world rendering.

4.2 World Mode - landscape
World Mode is the embodied open-world layer. The player explores as the dedicated protagonist or a
selected expedition character according to the switching rules defined below. The world should
prioritize physical causality over invisible barriers.
       Running/sprinting
       Climbing
       Swimming and diving
       Flying where capability permits
       Mounts
       Teleportation/fast travel where unlocked
       Entering/exiting structures
       Breaking eligible doors/gates/walls
       Exploration across large wilderness areas
       Active real-time combat
       Interaction with embodied NPCs
       Travel between settlements and regions

4.3 Orientation as a mechanic
Switching portrait/landscape is accepted as an intentional part of the product. Ruler Mode should feel
like a dedicated vertical strategy/gacha RPG; World Mode should use landscape for camera control and
exploration. The same world state remains authoritative across both orientations.

4.4 Expedition preparation
The full roster can be effectively unbounded, but the player prepares a limited field/expedition roster
before leaving controlled territory. If the player has not returned, the previously saved expedition
configuration remains active. This avoids keeping hundreds of character packages resident in memory
while preserving strategic roster freedom.
    Full roster -> territory management -> expedition setup -> leave territory -> field roster locked until legitimate
    reconfiguration point



5. Runtime and engine architecture
CURRENT BASELINE
Unreal Engine is the single shipped game engine. Native Android/C++ services are allowed beneath it
when they materially improve AI/NPU access, background jobs, cache/package handling, notifications or
platform integration. SQLite and the deterministic simulation core remain conceptually independent
from rendered actors.
    ANDROID GAME
    |
    +-- Unreal Engine
    | +-- World Mode / rendering

                                                             Page 6

<PARSED TEXT FOR PAGE: 7 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE
  | +-- Ruler Mode UI
  | +-- animation / VFX / cinematics
  | +-- embodied actors
  |
  +-- Authoritative game core
  | +-- world simulation
  | +-- economy / gacha
  | +-- characters / combat rules
  | +-- factions / demographics / history
  |
  +-- Native services
      +-- SQLite persistence
      +-- NPU/AI bridge
      +-- Android scheduling / notifications
      +-- content package and cache manager


5.1 Why not multiple game engines
Mixing Unity, Godot and Unreal as simultaneous runtime peers would duplicate renderers, lifecycle
management, audio, asset systems, build systems and debugging burden. The desired advantages are
better achieved by one visible engine plus specialized native services. Unity/Godot may still be used later
as offline tooling if they solve a specific production problem, but they are not part of the runtime
architecture.

5.2 Authoritative simulation versus presentation
      Unreal actors are representations of world entities, not the source of truth. A kingdom, NPC
      account or distant army can exist and evolve without having any Unreal Actor spawned.




6. World topology and spatial architecture
CURRENT BASELINE
The world should feel effectively unlimited over the lifetime of the project. It should not be implemented
as one mathematically infinite Unreal landscape. Instead, the universe is a graph of very large seamless
regions, each capable of using World Partition/HLOD internally.
  World graph
  +-- Region A: large seamless continent
  +-- Region B: wuxia sector
  +-- Region C: prehistoric sector
  +-- Region D: eldritch sector
  +-- Region E: alien sector
  +-- ...future regions/packages

Distant regions may connect through travel mechanisms appropriate to lore: oceans, portals, gates,
teleportation, spacecraft or other transitions. The simulation may know that a region exists before its
full playable terrain package is locally installed.




                                                    Page 7

<PARSED TEXT FOR PAGE: 8 / 143>

                                                            OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

6.1 Freedom and physical restrictions
No arbitrary invisible barriers should be used where a physical solution is reasonable. A fortress can be
entered through a gate, breached, climbed over, flown over, tunneled into or bypassed by valid abilities.
Restrictions may exist when they are physically or narratively real: lethal environments, materials too
strong for current tools, sealed dimensions, insufficient flight capability, and similar conditions.

6.2 Density target
The world should favor exploration over constant crowd density. Ordinary villages/cities may contain
roughly 30 physically embodied characters, while major cities may reach around 100 nearby/visible
entities when performance permits. Wilderness may contain only a handful of characters or none.


7. Procedural/authored world production
CURRENT BASELINE - IMPLEMENTATION PHILOSOPHY
World production should be hybrid. Pure hand-authored construction cannot scale to effectively
unlimited space, while pure procedural generation risks repetition and loss of identity. The
recommended pipeline combines deterministic procedural geography/ecology/infrastructure with
authored or generated high-value anchors, culture-specific kits and hand-directed landmarks.
  deterministic seed -> terrain/rivers/ecology/resources -> culture/biome rules -> roads/settlements ->
  authored/generated anchors -> simulated history -> current world


7.1 On-device versus production-pipeline generation
The phone should not be responsible for generating entire premium-quality regions from scratch. Large
region packages should be produced in the external content/build pipeline. On-device generation is
better reserved for local variation, state changes, settlement expansion, population, reconstruction,
minor procedural content and deterministic regeneration of ephemeral detail.

7.2 Virtually infinite expansion
The world can grow by adding region packages. Only active/relevant regions need full local assets.
Unloaded regions retain lightweight simulation state, historical summaries and metadata.


8. Territory, construction and reconstruction
CURRENT BASELINE

8.1 Directive-based construction
The player should not be required to place every object manually. The preferred model combines
strategic organization with NPC execution. The ruler issues directives such as expanding housing,
strengthening the northern wall, placing military facilities in a sector, or prioritizing a commercial
district. NPC planners and workers implement these directives using culture-specific building grammars,
terrain, available labor and resources.
Manual intervention remains available for important layouts or aesthetic preferences, but micro-
placement is not the normal loop.


                                                          Page 8

<PARSED TEXT FOR PAGE: 9 / 143>

                                                                OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

8.2 Construction as visible world process
   ruin -> survey -> debris clearing -> material delivery -> scaffolding -> partial structure -> usable structure ->
   completed structure

Construction and reconstruction should consume simulated labor/resources and progress over time. The
player can physically observe intermediate stages. NPC factions use the same broad principles.
Destroyed major sites may be visibly reconstructed rather than snapping back to an intact state.

8.3 NPC civilization construction
NPC factions can found settlements, build roads, fortify territory, rebuild after war, expand borders and
decline. Their construction logic should be systemic and culturally constrained rather than generated by
unrestricted AI.


9. Destruction model
CURRENT BASELINE - OPTIMIZED
The original ambition of extensive destruction is preserved in gameplay consequence but reduced in
literal simulation cost. Destruction is tiered.
 Tier                                       Examples                                   Recommended behavior
                                                                                       Cheap full
                                            Crates, furniture, signs, fences, small
 A - trivial                                                                           destruction/reset/regeneration as
                                            vegetation
                                                                                       appropriate.
                                                                                       Modular sections, breaching, collapse,
 B - structural                             Doors, gates, walls, houses, bridges
                                                                                       functional consequences.
                                                                                       Authored structural states plus visible
 C - major                                  Castles, palaces, temples, fortresses
                                                                                       sectional destruction.
                                                                                       Selected
                                                                                       craters/collapses/landslides/event-driven
 D - terrain                                Cliffs, roads, caves, landscape
                                                                                       deformation; no universal voxel
                                                                                       excavation.


Persistent destruction records meaningful structural state in the world database. Physics simulation is
presentation, not historical truth.

9.1 Reconstruction after destruction
Major damage can trigger repair/reconstruction jobs, which interact with logistics, worker availability,
materials and faction priorities. This transforms destruction from a temporary visual gimmick into a
geopolitical/economic consequence.


10. Civilizations, factions and asymmetric development
CURRENT CONCEPT; DETAILS OPEN
There is no single universal technology ladder. Different world sectors and civilizations may follow
radically different development dimensions: cultivation, alien technology, magic, biology, eldritch
transformation, spiritual systems, industrial development, social institutions or combinations.
   Civilization development vectors may include:
   - military


                                                              Page 9

<PARSED TEXT FOR PAGE: 10 / 143>

                                                              OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE
   - technological
   - magical
   - cultivation
   - spiritual
   - biological
   - eldritch
   - economic
   - social
   - architectural
   - lore-specific dimensions

A modern wuxia civilization can coexist with a prehistoric society, alien technology, immortals and
eldritch factions. Development proceeds according to local rules rather than forcing every faction
through the same eras.

10.1 Geopolitical rise and fall
Factions obey economic, territorial and military rules. They can expand, lose wars, change leadership,
form alliances, collapse or be replaced. No hidden player-targeting rubber-banding should manufacture
strength. Success and failure should follow the same causal system for player and NPC factions, subject
to lore-specific asymmetries.


11. Population, demographics and generations
SUPERSEDED IN DETAIL BY AE.4 - AGGREGATE POPULATION MODEL
Demography should be systemic, faction/species-specific and mostly non-AI. An island of immortals may
have almost no birth or natural death; rapidly reproducing species may have very high turnover;
constructs may be manufactured; other beings may follow unique rules.

11.1 Cohort simulation
Most large populations should not exist as fully individualized records. They can be represented as
cohorts by age/species/role/region. Individuals are instantiated when they become socially, historically
or mechanically relevant.
   population cohort -> event/need -> instantiate individual -> assign consistent identity/history -> persistent NPC


11.2 Genealogy
Important families can be tracked individually. Ordinary populations may use compressed demographic
state until promotion is necessary. This keeps centuries of history manageable without making every
inhabitant computationally expensive.


12. NPC embodiment, behavior and simulation fidelity
CURRENT BASELINE
NPC simulation is explicitly multi-fidelity. The majority of entities are not full Unreal actors and do not
run generative AI.
 Fidelity                                  Representation                            Typical use
 0 - aggregate                             Population/economic cohorts               Very large background populations.


                                                            Page 10

<PARSED TEXT FOR PAGE: 11 / 143>

                                                              OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE
 1 - lightweight individual               Persistent identity + limited state           Ordinary residents, soldiers, workers.
 2 - named individual                     Goals, relationships, history, richer state   Recognizable NPCs, officers, merchants.
                                          Detailed
                                                                                        Rulers, companions, major rivals, key
 3 - important NPC                        memory/knowledge/personality,
                                                                                        diplomats.
                                          possible AI escalation
                                          Full Unreal actor, animation, perception,
 4 - embodied                                                                           Entities currently near/visible to player.
                                          local behavior




12.1 Faction/world-specific behavior packages
Routine behavior should use explicit packages by species, faction, world sector and role. A prehistoric
predator, merchant caravan, wuxia disciple and eldritch cultist should behave differently because their
authored systems differ, not because an LLM invents behavior on every tick.

12.2 Promotion/demotion
A background entity can become important and be promoted to richer persistence if events make them
relevant. A previously important entity can later be compressed while retaining important history. This
keeps the living world responsive without permanently paying maximum simulation cost.

12.3 Canonical death
Death changes status; it does not delete historical identity. Dead characters remain available to
genealogies, records, memories, wars and history. Resurrection is a separate lore-valid state transition
requiring actual mechanics/resources where applicable.


13. Knowledge, memory, rumors and dialogue
CURRENT BASELINE

13.1 Non-omniscient knowledge
NPCs should normally know only information they could plausibly have received. News propagates
through witnesses, travelers, communications, magic, institutions, abilities and rumors. Characters with
explicit special abilities may bypass normal limits.

13.2 Wrong beliefs are allowed
The authoritative history stores what actually occurred, while NPC belief state can be incomplete,
distorted or false. This supports rumors, propaganda, bias and misunderstanding without corrupting
factual world state.

13.3 Memory model
Important conversations and events can remain meaningful for years of simulated history, but raw
dialogue is not loaded indefinitely into AI context. Memory should be consolidated into structured
episodic and semantic records, with importance scores and relationship effects.
   interaction -> episodic record -> importance -> structured memory -> consolidation -> durable belief/relationship
   state



                                                           Page 11

<PARSED TEXT FOR PAGE: 12 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

13.4 Dialogue tiers
Dialogue is pool-based by default. Context filters choose lines based on relationship, location, recent
events, faction, mood and knowledge. Generative dialogue is reserved for important characters, unusual
free-text input, high-value relationship scenes, diplomacy and geopolitical tension.


14. AI/NPU policy
CURRENT BASELINE - MINIMIZE AI
The NPU is an optional accelerator, not the core game brain. Deterministic/systemic logic should solve
routine decisions because it is faster, cheaper, reproducible and easier to test.

14.1 Appropriate AI uses
   Important free-text conversations
   Complex diplomatic/geopolitical conversations
   Memory summarization/consolidation
   Contextual wording of major reactions
   Semantic retrieval across important memories
   Optional future local voice or specialized inference tasks

14.2 Inappropriate AI uses
   Basic movement/navigation
   Routine combat decisions
   Every NPC economic action
   Population updates
   Simple faction behavior
   Routine dialogue selection
   Gacha/pity logic
   Damage calculations
   Construction scheduling

14.3 Event-driven inference
AI should run when an event justifies reasoning, not continuously. Results should be cached. During
free-text dialogue the world may enter slow-time so the user is not punished for waiting on inference. A
few seconds of latency is acceptable; routine ten-minute waits are not.


15. Character ontology, contracts and roster scale
CURRENT ARCHITECTURE; LORE IS CONTENT-SPECIFIC

15.1 Effectively unbounded roster
The database and content architecture should not impose a small conceptual roster ceiling. However,
local memory and installed assets are bounded through expedition preparation, streaming, package
caching and shared rigs.


                                                  Page 12

<PARSED TEXT FOR PAGE: 13 / 143>

                                                             OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

15.2 Technical rig families
Ordinary mobs may use nearly fully generic visual/technical templates. Playable/important characters
should share technical rig families wherever possible while preserving unique presentation. Example
rig families include humanoid size classes, winged humanoids, quadrupeds, serpentine bodies and
special custom rigs.

15.3 Female NPC treatment
Female NPCs may receive richer individual visual/interaction presentation when desired. Ordinary
gameplay interactability is separate from explicit-content eligibility. Explicit adult-content systems must
use an independent data gate tied to canonically adult characters; gender or appearance alone is never
sufficient.

15.4 Contract/recruitment state
Physical existence, faction membership, contract status, playable access and relationship state are
distinct data concepts. Pulling/recruiting generally establishes a subordination/allegiance relationship
rather than creating an entity from nothing.

15.5 Alternate forms
Alternate versions use an identity graph rather than one universal rule. Normal, awakened, corrupted,
event, historical or timeline versions may be transformations or separate coexisting entities depending
on lore. The engine must not hard-code a single interpretation.


16. Character growth and build divergence
CURRENT PHILOSOPHY; NUMERICAL FORMULAS DEFERRED TO TUNING
Characters grow through branching, seeded progression rather than a single fixed ladder. Randomness
and adaptive development are allowed, but should occur inside bounded evolution grammars so the
system remains testable and balanceable.
  base archetype -> branch choices/events -> specialization -> rare divergence -> highly specific/original end abilities

Later progression may produce extremely distinctive special abilities. Two characters with similar
origins need not converge on identical builds. Experiences, training, equipment, relationships,
transformations, faction context and rare events may affect development.

16.1 Anti-obsolescence
Old and low-rarity characters should remain capable of becoming genuinely competitive through
evolution, awakenings, reworks, equipment, story development and adaptive skill growth. Rarity may
influence acquisition or initial state but should not permanently condemn a favorite character to
uselessness.

16.2 Automated balance validation
Large-scale AI-vs-AI/system-vs-system simulations should detect broken loops, useless units, runaway
synergies and outliers. The Creative Director will later provide specific requirements for strategy, skill
sets, damage and related combat design.

                                                          Page 13

<PARSED TEXT FOR PAGE: 14 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

17. Combat architecture
CURRENT BASELINE - DETAILED BY Q AND AH CHECKPOINTS
The game uses a shared combat rules core with multiple execution modes. Stats, statuses, equipment
and effect definitions should be shared where practical; input/timing/resolution differs by mode.
  Combat rules core
  +-- Action executor
  +-- Strategic turn executor
  +-- War simulator
  +-- Hybrid boss phase controller


17.1 Action combat
Used primarily for embodied World Mode encounters. Target feel remains WuWa-like third-person
active control, but the detailed mobile grammar is now defined in checkpoint AH: permanent jump,
dodge, attack/skill parry timing, character-specific active skill sets, protagonist + up to two companions,
switch/QTE overlap, seamless world combat, aerial/underwater/flight/mounted continuity, and shared
combat-rule authority with turn combat.

17.2 Strategic turn combat
Intelligent-character conflicts use a Shoujo-Kaisen-like formation with six frontline characters and a
reserve line. When a frontline combatant dies or is removed, the next reserve enters until no reserves
remain. Total participation may reach roughly 12-18 characters. Auto mode is required.

17.3 Hybrid bosses
Bosses may switch between action and turn-based phases within one encounter. Highly bespoke
mechanics are permitted, including temporary control changes, terrain effects, aerial phases, custom
resources and rule-breaking abilities.

17.4 Field roster
The player selects a bounded expedition roster before leaving controlled territory. A smaller quick-
switch subset should be preloaded for instant action combat. Exact numbers remain OPEN and should
be set after memory profiling.


18. War and geopolitical simulation
CURRENT BASELINE
Wars can involve very large abstract forces without rendering every soldier. Distant wars resolve
through the same underlying strategic rules - commanders, unit composition, terrain, logistics, morale,
technology, abilities and important-character interventions - but use aggregate simulation.
When the player is nearby, the game can instantiate a representative local battlefield and allow direct
intervention. The wider strategic conflict continues around it. If the player is absent, the player may
later encounter survivors, ruins, territorial changes and aftermath rather than watching every battle.




                                                   Page 14

<PARSED TEXT FOR PAGE: 15 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

18.1 Fairness
NPC factions should not secretly receive anti-player bonuses. If an enemy grows stronger, that strength
should come from legitimate world causes. Difficulty settings may alter explicit known rules, but hidden
rubber-banding that invalidates world causality is rejected.


19. Difficulty, failure and persistence
CURRENT PHILOSOPHY
Failure is intended to matter. If the player chooses a disastrous strategy and loses most of an army, the
game should generally respect that consequence. Recovery is through rebuilding, diplomacy, new
recruitment, development or valid resurrection mechanics rather than automatic rollback.

19.1 Protected continuity
The save itself should remain technically recoverable even after catastrophic in-world failure. This does
not mean outcomes are forgiven; it means database corruption or impossible dead-end states should be
prevented by engineering safeguards rather than by secretly weakening opponents.


20. Gacha, banners and World Director
CURRENT MACRO; NUMERICAL ECONOMY/TUNING REMAINS

20.1 World Director role
The World Director is not the Creative Director. It is a bounded autonomous live-ops/world-management
system. It can schedule banners, reruns, events, challenges, releases and other surprises inside a
constitution defined by the Creative Director.

20.2 Constitution
   May schedule/select approved content and future packages
   May create or select events inside explicit rules
   May react to world state and faction development
   May not secretly counter the player simply for succeeding
   May not break pity/guarantee rules
   May not spawn arbitrary resources to force outcomes
   May not rewrite established history without an explicit lore mechanic
   May not change foundational game rules without a versioned design update

20.3 Future content surprise
Future characters/content can be prepared, validated and stored as opaque/sealed packages. The World
Director later reveals them. The Creative Director does not need to inspect package contents, preserving
genuine surprise without sacrificing pre-release QA.




                                                  Page 15

<PARSED TEXT FOR PAGE: 16 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

20.4 Gacha philosophy
The current intent is relatively accessible character acquisition with greater difficulty concentrated in
upgrading/development. Carry-over pity and a defined minimum pull-rate floor are desired. Exact rates,
currencies, guarantees and duplicate systems remain OPEN.


21. Time, offline catch-up and endless history
CURRENT ARCHITECTURE; WORLD-SPECIFIC CALENDAR VALUES REMAIN CONTENT
The game does not need to map directly to real-world dates. World time may advance through
exploration, travel, sleep, Ruler Mode decisions and offline catch-up. Long absences should genuinely
advance the world, including the possibility of returning to dramatically changed civilizations.

21.1 Multi-resolution catch-up
 Absence scale                                        Resolution
 Short                                                Relatively detailed simulation/event steps.
                                                      Aggregate routine behavior while resolving important
 Hours/days
                                                      events explicitly.
                                                      Daily/weekly blocks for economy, politics, demography
 Weeks/months
                                                      and construction.
                                                      Historical/epoch simulation; preserve major causal events,
 Years/decades
                                                      compress routine detail.


This makes scenarios such as leaving a prehistoric society and later returning to a much more advanced
state computationally practical.


22. Persistence/database architecture
CURRENT DIRECTION
The project should be treated as a persistent world database, not a conventional single SaveGame blob.
SQLite remains the preferred center for mutable state, with explicit schema versions, migrations,
transactional writes, snapshots, backups and corruption checks.

22.1 Historical event model
Important history should be event-sourced or event-like: death, war, territory change, contract, major
relationship change, ruler succession, destruction, reconstruction, discovery and progression milestone.
Routine ancient details are consolidated into summaries to prevent unbounded database growth.

22.2 Selective persistence
Persist meaningful consequences, not meaningless simulation noise. A destroyed bridge, depleted mine
or dead ruler matters. The exact angle of a bent grass blade does not. Ephemeral detail should
regenerate deterministically where possible.


23. Graphics and rendering strategy
CURRENT BASELINE; PROFILE-DRIVEN MARGINS ALLOWED

                                                 Page 16

<PARSED TEXT FOR PAGE: 17 / 143>

                                                         OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

The S26 Ultra is the only mandatory performance target. The initial rendering baseline should favor
Unreal mobile/Vulkan paths rather than forcing desktop rendering features. Visual budget should be
spent where the player notices it most: characters, faces, hair, animation, nearby lighting, combat effects
and important architecture.

23.1 Perceptual allocation
      High priority: hero characters, faces, combat readability, close environments, important interiors,
       signature VFX.
      Medium priority: ordinary nearby NPCs, settlement detail, vegetation around the player.
      Low priority: distant crowds, far vegetation, tiny clutter, non-interactive background detail.

23.2 HLOD/streaming
Distant settlements, forests and structures should collapse to cheaper representations. The player should
perceive a rich horizon without the device rendering every detailed object individually.


24. Performance and thermal strategy
CURRENT BASELINE
Performance should use explicit 30/60/120 FPS profiles rather than one universal target. 60 FPS is the
normal target; 120 FPS is an optional high-performance mode where workloads allow it; 30 FPS is an
acceptable heavy/thermal-safe fallback.
    Profile                            Purpose                                   Typical compromises
                                       High-performance action/traversal         More aggressive dynamic resolution,
    120 FPS
                                       where sustainable                         reduced distant effects/crowds/shadows.
    60 FPS                             Default intended experience               Balanced quality/performance.
                                                                                 Higher visual quality possible; reduced
                                       Heavy scenes, thermal fallback, battery
    30 FPS                                                                       simulation/animation update budgets
                                       saving
                                                                                 where safe.


Further margin is explicitly allowed if profiling shows a feature is not worth its thermal or stability cost.
The architecture should degrade gracefully rather than overheat in pursuit of a screenshot-quality
target.


25. UI/orientation and Ruler Mode architecture
CURRENT BASELINE
Ruler Mode should be optimized as a portrait-first strategic interface, not merely a rotated version of the
landscape HUD. It can borrow the dense, efficient interaction philosophy of vertical mobile gachas while
retaining original UI/terminology.

25.1 World suspension
When entering Ruler Mode, the open world should unload/suspend aggressively. The simulation core,
UI, database and necessary audio remain active. This is one of the most important battery/thermal
optimizations and should be treated as a core architecture feature.



                                                       Page 17

<PARSED TEXT FOR PAGE: 18 / 143>

                                                      OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

25.2 War control
Large wars are primarily planned and managed from Ruler Mode. The player may enter relevant
physical fronts in World Mode, but strategic scale should remain accessible without requiring the 3D
world to be loaded.


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

<PARSED TEXT FOR PAGE: 19 / 143>

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

<PARSED TEXT FOR PAGE: 20 / 143>

                                                            OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE