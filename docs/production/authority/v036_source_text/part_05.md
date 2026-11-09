# v0.36 full source text — segment 5
Original: Offline_Adult_Gacha_RPG_Master_Architecture_Baseline_v0.36_checkpoint_RulerOverlord.docx (143 pages), uploaded 2026-11-01. Derived text extraction, not a replacement for original embedded figures/tables. SOURCE LINE WINDOW 4001–5000. Historical decisions may be superseded by 7★ overlays.

simply lets the Ruler operate at the appropriate scale.

AC.11 Difficulty implications
   There is no hidden equalization of military power. A superior force can crush a weaker one, and a
    weak force should avoid impossible battles unless it has asymmetric advantages.
   Difficulty comes from incomplete information, multi-front commitments, travel/logistics, persistent
    losses, strategic objectives, powerful commanders/Rulers, Domain defenses and the cost of retaining
    conquered territory.



                                                   Page 82

<PARSED TEXT FOR PAGE: 83 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   The player can commit atrocities or extreme strategic actions in the fictional setting without an
    external morality system, but those actions can still create rational in-world reactions and
    consequences.
   War therefore remains dangerous even for an Overlord because decisive victory and sustainable
    control are different problems.

AC.12 Feasibility
This warfare model is feasible through multi-resolution simulation: aggregate armies and logistics at
distance, promote local regions to physical World Mode near the player, and persist only meaningful
individual actors and state. The main engineering requirements are deterministic strategic resolution,
objective/state tracking, Domain-core capture/destruction separation, and robust synchronization
between strategic and local battle layers.




                                                 Page 83

<PARSED TEXT FOR PAGE: 84 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AD. UI, Ruler Mode and Information Architecture
This checkpoint defines the current player-facing interface philosophy. The UI must expose the
enormous simulation without turning the game into a spreadsheet. Ruler Mode is the control surface for
the same persistent world, while World Mode remains a minimal-HUD physical exploration/action
experience.

AD.1 Ruler Mode home/lobby
   Portrait Ruler Mode opens with the protagonist/Ruler as the primary on-screen character rather
    than a replaceable generic lobby character.
   Other owned characters can appear/interact around the Ruler contextually or semi-randomly,
    reinforcing that the roster physically belongs to the same world.
   The lobby is not a static illustration. Characters, outfits, injuries/states, time/context and ambient
    interactions should be animated or otherwise stateful where feasible.
   Ruler Mode Home remains the character/lobby presentation screen. Its bottom navigation uses five
    primary icons; Territory/Domain is one of those five. The other four primary destinations remain
    subject to the final UI grouping, with secondary systems nested beneath them rather than adding
    many permanent top-level buttons.

AD.2 Ruler overview and strategic map
   The Ruler interface should prioritize a visual strategic overview rather than a spreadsheet. A
    proposed default is the Main Territory/Domain strategic view showing the most decision-relevant
    current state.
   The Territory/Domain destination contains the compact Main Territory/Domain overview together
    with the strategic map and related territory controls. It is reached from its dedicated bottom-
    navigation icon rather than replacing the character-focused Home screen. The overview presents
    Domain/Core state, current crises, armies, construction, key resources, subordinate-Ruler requests
    and immediate opportunities, while the strategic map shows known geography/state.
   The strategic map uses actual known geography and can show borders, settlements, armies,
    subordinate Rulers, trade/logistics routes, known threats, Domains, construction and dimensional
    links.
   The strategic interface is not omniscient. Unknown information is absent; uncertain intelligence is
    shown as uncertain/estimated where appropriate.
   Map overlays can selectively show political, military, resources, logistics, Domain influence,
    intelligence, trade and dimensional-link information instead of rendering every layer
    simultaneously.

AD.3 Reports, notifications and urgency
   A unified Reports/Events inbox aggregates subordinate requests, construction completion, attacks,
    character development, discoveries, expedition results, banner changes and important world events.
   Reports have useful priority/severity rather than dozens of unrelated red-dot notification systems.
   Genuinely urgent world events may interrupt ordinary Ruler presentation when immediate
    response is materially relevant.
   No fake mobile-game urgency or arbitrary countdowns are introduced. Timers exist only when the
    underlying world event genuinely has a deadline.

                                                  Page 84

<PARSED TEXT FOR PAGE: 85 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AD.4 Character interface
   Character pages separate identity/current state, combat build, progression, skills, equipment and
    transformations/versions into clear subviews rather than one extremely long page.
   Relationship/context data is not a dedicated character-page subsystem unless a particular mechanic
    needs to expose it.
   Codex/history/reference information is accessed through the dedicated Codex interface rather than
    duplicated inside every character page.
   There is no separate Mature Content Archive tab on the character page. Scene/content access belongs
    to the broader character/content-gallery architecture when needed, without creating a
    romance/affection-style menu layer.
   A Character Identity view groups all owned Base/Awakened/Corrupted/Evolved/etc. Versions
    beneath one identity.
   Equipment preview updates the live character model immediately so visual changes can be
    evaluated before confirmation.

AD.5 Manifestation terminology clarification
   Manifestation is the internal term for a ruler-specific owned instance/development state of a
    summonable Character Identity when multiple rulers can possess that identity without exhausting a
    global unique pool.
   This is distinct from World Fantasm, the MR-unlocked personal reality-field ability defined in the
    v0.21 checkpoint. 'Manifestation' remains the internal term for a ruler-specific owned instance of a
    summonable Character Identity; World Fantasm is a separate combat/reality mechanic.
   Manifestation should remain mostly internal/lore terminology unless exposing it becomes necessary
    for player comprehension. The normal UI can speak in terms of Character Identity and Version.

AD.6 Builds, persistence and advanced information
   Manual build-preset management is not emphasized because the user does not normally use preset
    systems.
   Likewise, manual team-preset management is not a major UI pillar. The game should instead
    remember last-used configurations, previous-clear formations and context-relevant setups
    automatically where useful.
   Equipment/skill comparison does not need to expose calculated before/after effect simulations as a
    standard feature.
   Raw combat formulas are not exposed in normal UI. The game presents usable stats, effect
    descriptions and outcomes without requiring the player to inspect implementation equations.
   Debug/combat-log tooling may expose formulas internally during development, but this remains
    outside the normal player-facing interface.

AD.7 Codex and Chronicle
   One unified Codex covers discovered/owned characters, species, factions, worlds, locations,
    monsters, equipment, skills/powers, Domains, treasures, titles and other reference knowledge.
   Codex knowledge is knowledge-bound: it reveals what the player/faction has actually learned, except
    information inherently known through ownership/contract or explicit mechanics.
   Historical events do not live in the Codex. They belong to the separate Chronicle/history system.


                                                 Page 85

<PARSED TEXT FOR PAGE: 86 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   The Chronicle presents eras, wars, rulers, disasters, discoveries, personal history and other historical
    events chronologically.
   Major lists support search and filtering from the beginning because the database may become
    enormous.
   The player can mark favorites and use personal organizational tags where helpful.

AD.8 Inventory and ownership presentation
   Bulk ordinary materials are shown as aggregated resources when individual identity does not
    matter.
   Unique weapons/relics are not dumped into a generic inventory list. Equipment is managed through
    the equipment system, while historically important treasures/relics are represented through the
    Codex/Treasures interface and their world ownership state.
   The UI therefore avoids a single universal inventory becoming an unreadable container for every
    persistent object in the game.

AD.9 Gacha presentation
   Gacha receives full banner presentation and summon animation despite being offline.
   Summon animations may communicate rarity/result clues before final reveal.
   Summon animation skipping/acceleration can be available after or during repeated pulls as a
    usability feature.
   Character reveal/presentation is not reserved only for first acquisition or only for high rarity.
    Repeated and lower-rarity results can still receive authored presentation appropriate to their
    identity/version, with intensity allowed to vary by result.

AD.10 World Mode transition and HUD
   Entering World Mode intentionally transitions the device from portrait Ruler Mode to landscape 3D
    play; returning to Ruler Mode returns to portrait.
   World Mode HUD is deliberately minimal and context-sensitive, using as few permanent icons as
    practical.
   Action-combat baseline HUD: HP/state display, two switch/QTE character icons similar in function to
    the already referenced three-character action structure, and the current character's skill controls.
   The environmental HUD may adapt to ground/sky/underwater/space/other environments where
    useful, but remains minimal rather than adding a separate dense instrument panel for every biome.

AD.11 Photo/free-camera mode clarification
   A photo/free-camera mode would mean a non-combat presentation tool that hides most HUD, allows
    camera composition/zoom/angles and optionally pauses or limits simulation where technically safe
    so environments/characters can be viewed or captured cleanly.
   A dedicated photo/free-camera mode is not required. It may be added later only if it is low-cost and
    does not displace higher-priority systems.

AD.12 Dynamic UI theme and accessibility
   Ruler Mode visual motifs can evolve with the faction/Domain rather than remaining one fixed
    generic UI skin forever.

                                                   Page 86

<PARSED TEXT FOR PAGE: 87 / 143>

                                                      OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Domain themes such as Draconic, Death or Time may subtly affect frames, motifs, ambient effects
    and presentation without reducing readability.
   Basic low-cost accessibility/usability options should exist: UI scale, subtitle size, camera sensitivity,
    motion blur, screen shake, effect-intensity controls and remapping where technically practical.
    These are implementation conveniences rather than gameplay systems.

AD.13 Development/debug UI
   A hidden Developer/Ruler Debug interface exists at least throughout development and testing.
   It can inspect IDs, seeds, world state, battle logs, simulation state, stuck events, package/content
    status and relevant database records.
   State-repair actions such as unsticking an NPC, teleporting out of broken geometry, restoring a
    broken reference or repairing a corrupted development state are acceptable development tools and
    should be logged so interventions are traceable.
   Whether repair functions remain accessible in the final personal build can be decided later; the
    instrumentation itself should remain available for maintenance.

AD.14 UI principles
   Show decisions, not simulation noise. Detail exists on demand, while default interfaces expose
    information useful to the current action.
   Ruler Mode is the control interface for the same physical persistent world, not a separate
    management game.
   Nothing important is represented only as an image when a live/stateful representation is technically
    reasonable; the game should favor animated/runtime character and world presentation over static
    menu art.

AD.15 Feasibility
This UI architecture is feasible and is necessary for the project scale. The primary requirement is strict
information hierarchy: large databases must be searchable and layered, strategic views must summarize
rather than mirror raw simulation state, and World Mode must remain visually light. Automatic
persistence of previous clears/last-used configurations is preferable to forcing the user into preset-
management workflows they do not use.




                                                    Page 87

<PARSED TEXT FOR PAGE: 88 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AD. Engineering Ownership, Technical Production and Scope
Governance
From this checkpoint onward, the user remains Creative Director and final authority over intended
player experience, content identity and major design goals. The assistant/engineering role owns
technical implementation decisions, production architecture, performance strategy, persistence,
build/deployment, QA, tooling and engineering-driven scope reduction.

AD.1 Decision ownership
   Creative direction: the user defines what the game should feel like, what experiences matter, and
    which content/design outcomes are desirable.
   Engineering direction: implementation technology, Unreal project structure, native Android
    integration, SQLite boundaries, package formats, streaming, caching, build automation, test strategy,
    migration/versioning, performance budgets and deployment are engineering-owned decisions.
   The user does not need to answer technical-production questionnaires unless a technical choice
    would materially change the player experience.
   When several implementations preserve the same experience, engineering chooses the simplest,
    most robust and most maintainable option.

AD.2 Value-preserving scope reduction
   Previously designed systems are not protected merely because they were discussed or accepted.
   A subsystem may be simplified, merged, reduced in simulation fidelity, deferred or removed when
    its implementation/content/maintenance cost is disproportionate to its player-facing value.
   No such reduction should materially damage the game's core identity: gacha, combat, mature
    content, exploration, persistent world/ruler fantasy and broad player freedom.
   Cuts should target invisible complexity, duplicate mechanics, low-value micromanagement and
    systems whose effects can be represented by a cheaper existing subsystem.
   When a proposed cut would noticeably change the intended experience, it returns to the Creative
    Director rather than being silently changed.

AD.3 System triage categories
   KEEP: high player-facing value and acceptable implementation/maintenance cost.
   SIMPLIFY: valuable experience, but the internal simulation can be substantially cheaper.
   MERGE: two or more systems can share one underlying state/effect framework without losing their
    distinct presentation.
   LOWER RESOLUTION: detailed simulation only near/relevant to the player; aggregate or event-
    driven simulation elsewhere.
   DEFER: useful but unnecessary for the first playable architecture and safe to add later.
   CUT: low-value complexity whose removal does not materially reduce the overall game.

AD.4 Default technical-production direction
   Unreal Engine remains the sole shipped game engine, targeting Android/Vulkan and the Samsung
    Galaxy S26 Ultra as the primary real-device performance target.


                                                 Page 88

<PARSED TEXT FOR PAGE: 89 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   SQLite remains authoritative for persistent world state; Unreal actors are runtime representations
    rather than the source of truth.
   World simulation uses multiple resolutions: detailed local simulation, strategic regional simulation
    and compressed long-distance/offline evolution.
   Content is modular and versioned. Large worlds, characters, cinematics and optional high-resolution
    assets should be independently patchable/streamable rather than forcing monolithic application
    rebuilds.
   External production tools create/validate high-quality content packages. The shipped runtime
    consumes validated content and does not depend on runtime generative AI for production assets.
   The NPU is optional acceleration only; core gameplay must not depend on continuous local-AI
    inference.
   GitHub is source control/automation coordination; Unreal builds use suitable persistent or remote
    build compute rather than treating GitHub itself as the build machine.
   Automated tests cover deterministic combat, gacha, persistence/migrations, crash recovery, long
    simulations, world-event resolution, economy/logistics, war resolution, content-package validation
    and performance regressions.
   A development/admin interface remains available during production for inspection, state repair,
    deterministic replay and bug reporting.

AD.5 Content/package policy
   The base application contains core runtime systems, schemas, shared foundations and enough
    starting content to function independently.
   Additional characters, regions/worlds, cinematics and large content sets should be modular packages
    with explicit versions and dependencies.
   Hidden/future content may be installed but sealed from ordinary player-facing discovery until
    activated by the World Director.
   Removing a presentation/asset package must never delete the authoritative persistent state
    associated with that entity.
   Package activation requires validation of references, dependencies, data schemas, skills/effects,
    rigs/animations, eligibility metadata and storage compatibility.
   Shared rigs, animation foundations, modular outfits/material states and reusable effect primitives
    are preferred whenever they preserve character identity; bespoke assets remain available for
    exceptional characters/content.

AD.6 Performance and storage governance
   Phone feasibility is a hard architectural constraint, not a late optimization task.
   Performance budgets are enforced per system, content package and active scene. Systems that
    exceed budget must be simplified, streamed, aggregated or disabled at distance.
   The normal local footprint should target roughly 60-80 GB, with approximately 100 GB treated as a
    practical ceiling rather than a target.
   Cold/external storage such as TeraBox may be used manually for archived content, but normal
    runtime must not depend on it.
   Ruler Mode should suspend/unload expensive World Mode rendering/simulation presentation
    wherever possible while authoritative strategic simulation continues at appropriate resolution.



                                                 Page 89

<PARSED TEXT FOR PAGE: 90 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AD.7 Implementation gate
Before substantial implementation, engineering will perform a subsystem audit of the complete
architecture and assign each major feature a KEEP / SIMPLIFY / MERGE / LOWER RESOLUTION / DEFER /
CUT disposition, together with the player-facing reason. The purpose is to prevent the project from
preserving complexity for its own sake and to establish a buildable vertical slice.

AD.8 Governing principle
Preserve the experience, not the mechanism. If a cheaper implementation produces the same
meaningful player experience, use it. If a mechanism is expensive and does not materially improve the
game, remove it. If a compromise changes the core experience, surface that decision rather than silently
making it.




                                                 Page 90

<PARSED TEXT FOR PAGE: 91 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AE. Hard Simplification Pass - Minimal Simulation Doctrine
AUTHORITATIVE SUPERSESSION: this checkpoint deletes or simplifies the systems listed below
wherever older sections conflict. The game should preserve visible causality and player-facing
consequences without continuously simulating low-priority background processes. Minor systems
should default to numbers, tags, state transitions, orders or event results, and may require effectively
zero computation while nothing relevant is changing.

AE.1 Protected pillars - do not cut
   Gacha, character collection and summon presentation.
   Turn combat, World Mode action combat and the shared deep combat/rule engine.
   Mature content. This is protected from scope reduction and should receive substantial content
    investment rather than being treated as an optional side feature.
   Exploration and environment-specific traversal, including ground, sky, underwater, underground,
    dimensions and later space.
   Character progression. Level, Rank, Rarity progression, skills, equipment, Evolution, Awakening,
    Corruption, Transcendence/forms and character-specific development remain important, but should
    share common technical engines where possible.
   Ruler/Domain/Core progression, persistent world consequences and meaningful warfare.
   Blood, visible violence, equipment damage, armor/clothing destruction and other intended mature
    combat presentation.

AE.2 Systems deleted as independent gameplay/simulation domains
   Relationship simulation. No generic relationship meters, attraction/trust/respect/fear systems, social-
    network simulation or relationship-progression engine.
   Gift system. Gifts may exist as ordinary objects or authored events but do not have a dedicated
    progression loop.
   Law/crime simulation. No legal system, crime simulation, courts, law enforcement model, criminal
    procedure or legal-management gameplay.
   Finance simulation. No inflation model, investment/market simulator, interest-rate system,
    currency-collapse model, household finance or financial gameplay.
   Player-created currency as a gameplay system.
   Personal business/property simulation for characters as a general system.
   Taxation as a management system.
   Economic-policy sliders.
   Genealogy as a general system. Specific lineage/family facts may exist only when directly relevant to
    a character, story, succession or ability.
   Politics as an independent simulation domain: no parliament, political parties, ministries,
    bureaucracy, ideological-population modeling or policy-management layer.
   Faction-feeling/opinion systems. No hidden fear/respect/hate/trust matrices.
   Treaty system. Alliances/support/subordination and similar relations are direct faction states or
    actions, not legal treaty simulations.
   Internal-politics simulation. Coups, schisms, succession crises and rebellions may exist as explicit
    events without a continuously simulated political layer.
   Occupation-governance simulation.
   Settlement micromanagement.
                                                  Page 91

<PARSED TEXT FOR PAGE: 92 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   NPC task/work simulation. No persistent per-NPC job queue, worker scheduling, daily task graph or
    work-shift model.
   Mount breeding as a general system.
   Aging as a continuously simulated character system.
   Detailed historical simulation. The Chronicle stores important events and concise summaries, not
    exhaustive historical detail.

AE.3 Alignment, culture and religion
   Alignment, when used, is only a descriptive character trait such as Lawful/Neutral/Chaotic. It is not a
    morality engine and does not require simulation.
   Religion and culture are primarily lore/identity identifiers. They can select architecture, clothing,
    dialogue pools, visual motifs, factions, powers, rituals, equipment or authored events.
   There is no generic religion-spreading or cultural-diffusion simulation.
   Assimilation is primarily a Territory/Core/faction-state outcome when relevant. It does not require
    person-by-person demographic simulation.

AE.4 Population and demographics - number-first
   Population is fundamentally an aggregate number, not a simulated collection of lives.
   Birth and death are aggregate changes to that number unless a specific named character is directly
    involved.
   Species/culture composition may exist only as coarse counts/tags when it changes troops, visuals,
    resources, territory/Core behavior, available characters or other meaningful gameplay.
   Population can be reduced to zero/wiped out. The system does not need a detailed demographic
    pathway explaining every individual death.
   No ordinary genealogy, household, employment, age-pyramid, marriage or family simulation is
    required.
   Named individuals are persistent because they are characters, not because the population model has
    instantiated every citizen.

AE.5 Diplomacy - intentionally minor
   Diplomacy is a small strategic action/state layer whose purpose is mainly to determine wars,
    alliances, support, subordination and similar high-value outcomes.
   Typical actions can be simple commands/results such as Declare War, Form Alliance, Provide
    Support, Request Support, Break Alliance, Submit/Subordinate or Refuse.
   No relationship score, trust score, treaty-law model or long diplomatic simulation is required.
   Faction decision logic reads concrete state and facts: current alliance, enemies, prior attacks,
    resource conflict, leader/Domain status, requested support and major historical events.
   Dialogue may present diplomacy richly, but the underlying state remains small.

AE.6 Espionage and dispatch unification
   Espionage is not its own simulation game. It is one category of Dispatch alongside exploration,
    resource collection, reconnaissance, support and other assignments.
   An espionage dispatch has participants/capability, target, duration/risk and a result.
   The result may reveal knowledge, sabotage something, steal an object/technology, locate a target,
    support an event or fail.
                                                  Page 92

<PARSED TEXT FOR PAGE: 93 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   The game does not simulate irrelevant internal spy activity step-by-step or preserve minutiae of how
    the mission was performed unless an authored event makes that detail important.

AE.7 Construction, reconstruction and territory management
   Construction and reconstruction use the same Project/Order mechanism.
   There is no worker simulation or worker scheduling. Progress is derived from project requirements,
    available resources, relevant territory capabilities/facilities/characters and elapsed world time.
   Named characters may influence a project because their abilities or assignment matter, but ordinary
    labor is not represented as individual NPC tasks.
   Settlement state is summarized through useful values and completed/in-progress projects rather
    than a city-simulation dashboard.
   No settlement micromanagement is required beyond issuing meaningful high-level orders and
    responding to important events.

AE.8 Resources, economy and logistics - numbers and routes
   There is no finance layer. The useful economy consists of resources, ownership, availability/scarcity,
    crafting requirements, shops and strategically meaningful movement/access.
   Different lore currencies may still exist as items/values used by different civilizations, but there is no
    player-created-currency or monetary-policy gameplay.
   No taxes, economic-policy sliders, personal businesses, universal character wealth or debt engine are
    required.
   Stockpiles are not separately simulated objects. A territory simply has high or low resource totals;
    siege/isolation consumes those totals.
   Logistics is represented by route connectivity, capacity/availability, travel time, risk/blockade state
    and resource transfer results. There is no convoy timetable or ordinary-cargo simulation unless the
    player is physically interacting with an important shipment.
   Physical caravans/ships may appear in World Mode for presentation, interception or authored
    importance, without implying that every background shipment is individually simulated.
   Merchant inventory may remain finite and locally responsive, but restocking can be computed from
    local availability and elapsed time rather than tracing every supplier.

AE.9 NPC presence, behavior, knowledge and dialogue
   NPCs should not look static. Nearby NPCs can move, converse, sit, patrol, inspect objects, train, travel
    short local routes or perform other believable changing ambient actions.
   Those actions are presentation behavior, not a persistent daily-life/task simulation.
   There is no day planner, work schedule or per-NPC task queue to simulate while the player is absent.
   Specific important NPCs may have authored or event-driven movements/actions when those actions
    actually matter.
   NPC memory is deleted as a separate generic subsystem. Important NPCs retain only the
    knowledge/facts needed for their story, dialogue, decisions or abilities.
   Knowledge is therefore the authoritative lightweight mechanism: what an important NPC knows,
    believes when specifically useful, has witnessed, or has been told.
   Dialogue is primarily pool/context/event driven. Generated/free-text dialogue remains optional and
    should never require maintaining a full life simulation behind each NPC.


                                                   Page 93

<PARSED TEXT FOR PAGE: 94 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AE.10 Ecology - meaningful without food-web simulation
   Ecology remains important to exploration, species presence, resources, environments and world
    transformation.
   There is no generic food-web simulation, prey/predator simulation, invasive-species simulation or
    continuous ecosystem-agent model.
   Regions store only the ecological state needed for play: relevant species presence/abundance,
    important resources, habitat conditions and exceptional threats/events.
   Local World Mode can visually show predators hunting, creatures migrating or other believable
    ecology as presentation/encounter behavior without requiring those actions to be simulated globally.
   Extinction, reintroduction, migration, corruption, ecological collapse or recovery are state
    changes/events rather than consequences of continuously modeled individual animals.

AE.11 Roads, mounts and vehicles
   Important roads exist as map/world route state. There is no emergent road-creation simulation from
    individually tracked travelers.
   Mounts and vehicles share a common movement/vehicle capability framework where practical.
   Mount breeding is deleted. Individual special mounts may still have authored growth/evolution
    mechanics if the mount itself is important.
   Ships, airships, spacecraft and other vehicles become specialized implementations of shared
    travel/ownership/damage interfaces rather than unrelated full subsystems.

AE.12 Environment progression and time
   Underwater civilizations are a mid-game expansion target rather than initial-scope infrastructure.
   Space/stellar play is a mid/late-game expansion comparable to accessing additional
    dimensions/worlds.
   Nested time ratios remain supported as world/region/effect state, but the game does not simulate
    every independent clock continuously every second.
   Time is resolved when relevant: on travel, simulation steps, entering/leaving a region, event
    resolution, offline catch-up or explicit time abilities.
   Continuous biological aging is deleted. Age-related lore/state changes can occur through authored
    progression or explicit time events when required.

AE.13 Character progression - protected, unified technically
   Character progression is not cut.
   Different player-facing concepts such as Evolution, Awakening, Corruption, Transcendence and
    alternate forms may use one shared progression/state-graph engine internally while preserving
    distinct rules and presentation.
   Learned skills integrate into the character's own skill set/kit progression. They are not maintained as
    a separate inventory-like collection of disconnected skills.
   A character can accumulate very large development history, but runtime combat resolves the
    currently learned/active/integrated kit and relevant unlock graph rather than iterating every
    historical skill each frame.
   Character-specific exceptions and extreme rule-breaking abilities remain fully supported.



                                                  Page 94

<PARSED TEXT FOR PAGE: 95 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AE.14 Mature content, blood and damage presentation - protected
   Mature content is explicitly excluded from the pruning target. It remains a primary content pillar
    and can receive more, not less, content investment.
   Blood must be visibly present where appropriate in violent combat. It does not need to be
    permanent world state or physically simulated after it no longer matters.
   Injury, armor breakage, weapon breakage, torn clothing, exposed-body states and other visible
    damage remain supported.
   Reusable rendering/VFX technology is allowed, but body/clothing/injury state transitions must come
    from defined valid states rather than an uncontrolled generic gore-state generator.
   Where states differ by character, anatomy, species, outfit or equipment, those states are
    authored/declared accordingly.
   Adult sexual content and other mature-content production should not be reduced merely to save
    system complexity; the main constraint is asset/content production quality.

AE.15 Interiors and world embodiment
   Ordinary/random buildings remain enterable when physically appropriate, but their interiors do not
    need high bespoke design.
   Procedural/modular interior kits can provide functional rooms, navigation, loot/objects and basic
    visual identity at low content cost.
   Important buildings, characters, story locations and exploration landmarks receive bespoke design
    where the value is visible.
   Enterability is preserved as an exploration freedom goal without turning every ordinary house into
    a hero environment.

AE.16 Computation doctrine - default to almost nothing
   A minor system should consume no continuous computation merely because it exists in the design
    document.
   Use event-driven updates, cached values, coarse totals and lazy resolution. Recalculate only when an
    input changes, a timer/event resolves, the player queries the state, or an area becomes relevant.
   Offscreen ordinary NPCs do not think, work, eat, travel or schedule tasks individually.
   Offscreen ordinary populations do not reproduce/die individually.
   Offscreen ordinary ecology does not run animal-agent simulation.
   Offscreen ordinary trade does not move every item physically.
   Offscreen ordinary politics/relationships do not exist as hidden simulations.
   When the player arrives, presentation may instantiate believable local behavior from the current
    state without reconstructing every unobserved microscopic cause.
   The world must remain causally coherent at the level the player can perceive and affect; it does not
    need to compute invisible detail.

AE.17 Reduced core runtime model
After this pruning pass, most background world behavior should be expressible through a small number
of reusable engines rather than dozens of bespoke simulators:
   Entity/Character State Graph: character state, forms, injury/death, equipment/clothing state and
    progression transitions.

                                                 Page 95

<PARSED TEXT FOR PAGE: 96 / 143>

                                                      OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Combat/Capability/Authority Engine: turn combat, action combat, extraordinary rules,
    environmental capabilities and Domain interactions.
   World/Location State: regions, territories, discovered locations, roads, environment state and
    physical ownership.
   Resource/Project Engine: resources, construction/reconstruction, crafting, training, research-like
    projects and timed orders.
   Dispatch Engine: expeditions, gathering, reconnaissance, espionage, support and other delegated
    assignments.
   Faction/War State: rulers, alliances, wars, subordination, armies and high-level strategic decisions
    with minimal diplomacy.
   Knowledge/Event/Chronicle Layer: discovered information, important NPC knowledge, story/world
    events and concise history.
   Content/Gacha/Director Layer: identities, versions, banners, pity, sealed content, package activation
    and scheduled events.

AE.18 Governing rule
DEFAULT RULE: do not simulate a process merely because the real world would contain one. If the
player only needs the result, store or calculate the result. If a number, tag, state transition, order result
or authored event gives the same player-facing value, prefer it. Spend computation and content budget
on characters, combat, gacha, mature content, exploration, Domains, major events and the physical
situations the player can actually perceive or exploit.




                                                    Page 96

<PARSED TEXT FOR PAGE: 97 / 143>

                                                      OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AF. World Fantasm, Transcendence and Recent Clarifications
AUTHORITATIVE CLARIFICATION: this checkpoint supersedes older references to 'Fantasm
Materialization', multi-depth Transcendence ladders, and the unresolved World-Fantasm terminology
wherever they conflict.

AF.1 World Fantasm concept
   World Fantasm is acquired when a character reaches Current Rarity MR, regardless of that
    character's immutable Origin Rarity.
   It externalizes the character's inner world/perception as a real battlefield field. Activation must
    create visible environmental/field changes rather than functioning as an invisible stat aura.
   World Fantasm is essentially a non-Ruler-specific analogue of Domain Authority: Domain Authority
    is sourced from Ruler/Territory/Domain Core structures, while World Fantasm is sourced from the
    character's own existence and inner world.
   Domain Authority and World Fantasm have no categorical priority over one another. Their
    interaction is resolved as an ability/rule conflict using the relevant powers, counters, compatibility,
    strength and situation.
   When two or more World Fantasms are active, their fields may overlap, mix, conflict, form
    boundaries, fracture, overwrite local portions of one another, or visibly collide. The visual result
    should reflect the mechanical interaction.
   World Fantasm remains an ability, not an automatic victory rule. A lower-grade Fantasm may
    counter a higher-grade one in the right matchup; a stronger Fantasm may dominate; situational
    relevance and specific rules matter.
   Default persistence rule selected under engineering discretion: when a World Fantasm ends, the
    field/rule overlay itself collapses. Ordinary direct consequences that physically occurred (damage,
    injuries, destruction, displaced objects, etc.) may persist, while Fantasm-only terrain/entities/rules
    disappear unless that ability explicitly declares a persistent result.

AF.2 World Fantasm grade by immutable Origin Rarity
   R Origin Rarity -> World Projection.
   SR Origin Rarity -> World Projection.
   SSR Origin Rarity -> World Alteration.
   UR Origin Rarity -> World Materialization.
   LR Origin Rarity -> World Manifestation.
   MR Origin Rarity -> World Negation.
   These are direct grades obtained at MR, not sequential stages. An Origin-MR character reaching
    Current Rarity MR obtains World Negation directly; it does not first pass through Projection,
    Alteration, Materialization and Manifestation.
   Moving from MR to Transcendence may improve the character's existing World Fantasm - for
    example scale, stability, control, reach, efficiency, collision strength or rule expression - without
    automatically changing its World Fantasm grade.

AF.3 Transcendence
   Transcendence is the universal rarity/state immediately after MR. Every character can reach a
    Transcendence state once the required breakthrough is achieved.

                                                   Page 97

<PARSED TEXT FOR PAGE: 98 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   A character receives one Transcendence grade/state rather than sequentially climbing through all
    five names.
   The five Transcendence grade names, in ascending scale, are: Ascendant, Unbound, Exalted,
    Transcendent, Absolute.
   The intended Origin-Rarity banding mirrors the five-band World-Fantasm scale: R/SR -> Ascendant;
    SSR -> Unbound; UR -> Exalted; LR -> Transcendent; MR -> Absolute.
   Transcendence breakthrough is not a simple XP threshold. The general structure is: reach/break a
    power ceiling and satisfy a small set of character-specific breakthrough proofs/conditions. Three solo
    conditions are a representative pattern, not a mandatory universal formula; characters may use
    other equivalent bespoke requirements.

AF.4 Dimensional preparation purpose of gacha/contracts
   The dimensional contract/gacha preparation system exists because dimensions provide a means to
    gather, develop and organize power in preparation for extremely powerful endgame
    monsters/enemies.
   Ordinary world conflicts, Ruler progression, Domains, roster growth and character development
    therefore also function as preparation for the true late-game threats.
   Whether dimensions act consciously or whether this preparation system is an automatic multiversal
    defensive law remains a lore-detail question and is not required for current implementation.

AF.5 Additional resolved clarifications
   Percentage-of-Max-HP offensive damage is not a normal direct-attack formula; it belongs to explicit
    afflictions, debuffs or other specially authored mechanics.
   Ruler Commands are condition/encounter based rather than governed by one universal command-
    point resource.
   Subordinate Rulers may use both their own and superior-Ruler-provided gacha resources; their
    acquisitions ultimately enter the player's wider controlled roster ecosystem.
   Login/daily/weekly/return rewards, when present, are automatically claimed/credited rather than
    requiring a manual claim loop.
   Surrender is contextual and uncommon, never a generic low-HP behavior, and never obliges the
    player to spare the surrendering enemy.
   Ruler Mode Home stays character/lobby focused. Territory/Domain is one of the five bottom-
    navigation destinations and contains the territory overview and strategic map.
   Dedicated photo/free-camera mode is not required.




                                                 Page 98

<PARSED TEXT FOR PAGE: 99 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AG. World Fantasm Grade Correction and Design-State Audit
AUTHORITATIVE CORRECTION: LR Origin Rarity has World Manifestation only. World Negation is
reserved for MR Origin Rarity. This supersedes the v0.21 wording that allowed LR to receive either
World Manifestation or World Negation.

AG.1 Final current World Fantasm grading
   R Origin Rarity -> World Projection.
   SR Origin Rarity -> World Projection.
   SSR Origin Rarity -> World Alteration.
   UR Origin Rarity -> World Materialization.
   LR Origin Rarity -> World Manifestation.
   MR Origin Rarity -> World Negation.
The progression is therefore deliberately one distinct step per rarity band: Projection, Projection,
Alteration, Materialization, Manifestation, Negation.




                                                  Page 99

<PARSED TEXT FOR PAGE: 100 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AH. World Mode Action-Combat Checkpoint
STATUS: CURRENT AUTHORITATIVE BASELINE.

AH.1 Mobile movement, attack and targeting
   Default mobile locomotion uses a permanent left virtual movement stick and right-side camera drag.
   Jump is a permanent input. Aerial attacks are supported whenever the character's movement/state
    permits them.
   The normal attack input may support character-specific tap chains. There is no universal rule that
    every character has a hold/heavy attack; heavy/charged/stance/alternate basics are authored per
    character and may use hold or another fitting input.
   Basic attack structure is character-specific and may vary in hit count, combo length, timing,
    branching, ranged/melee behavior, charge behavior, stance logic and other authored properties.
   Soft auto-targeting and manual hard lock-on both exist. To switch a hard-locked target on touch, the
    player can touch the other enemy directly.
   Appropriate ranged skills/weapons may support precise manual aiming.
   Large enemies may expose multiple lock-on body parts/weak points.
   The game may display subtle preparation/readability cues for dangerous/boss attacks, but does not
    rely on conspicuous difficulty-lowering warning markers. A brief roughly 0.5-1 second readable
    preparation window is acceptable where appropriate.
   Parryable attacks are learned from animation/readability rather than a universal bright parry
    indicator.

AH.2 Dodge, parry, guard and defensive interaction
   Dodge is a baseline playable-character action unless an explicit state/exception removes it.
   A successful perfect dodge primarily succeeds by avoiding damage. It grants no universal bonus
    resource/reward.
   A perfect dodge may produce a short approximately 2-3 second perception/time-slow presentation.
    Any additional benefit exists only when an explicit character, condition, effect or encounter rule
    grants it.
   Generic parrying is primarily timing an attack or skill into the enemy attack rather than pressing a
    dedicated universal parry button.
   Characters may nevertheless possess dedicated guards, parries, counters or defensive stances as part
    of their authored kits.
   Blocking/guarding is character-specific rather than universal.

AH.3 Action commitment and cancel policy
   Animation canceling means interrupting the recovery/commitment of an attack or skill with another
    action such as dodge, switch, jump or another skill before the original animation would naturally
    finish.
   Animation cancel behavior is character/action-dependent. Each attack, skill and state defines its own
    valid cancel windows and destinations. Fast/agile actions may permit extensive
    dodge/switch/jump/skill cancels; highly committed actions may lock the character until their
    authored release point unless an explicit mechanic overrides that commitment.


                                                Page 100

<PARSED TEXT FOR PAGE: 101 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AH.4 Party, switching, QTE and defeat flow
   The immediate World Mode party remains protagonist/Ruler + up to two selected companions. The
    protagonist remains a member of the immediate party even while a companion is controlled.
   Switching is near-instant unless the current state explicitly prevents it.
   The outgoing character normally leaves the active field after their exit/switch animation;
    companions do not remain persistently co-simulated on field after control switches away from them.
   Entry attacks, exit attacks and switch effects are character-specific.
   QTE availability is driven by character/skill-specific conditions rather than one universal team
    meter.
   QTEs may create temporary two- or three-character overlap and may support authored
    common/coordinated QTEs comparable in structure to multi-character combination sequences.
   A controlled companion's defeat transfers control to another available party member.
   If the protagonist is defeated while an immediate revival/return mechanic is valid, combat may
    continue through companions until that mechanic resolves, or the protagonist may return directly
    without a forced party-state rewrite if the mechanic says so.
   If the protagonist truly dies with no immediate valid continuation mechanic, canonical
    death/Regression handling applies. A bespoke encounter may delay the transition only if an explicit
    mechanic permits companions or another effect to resolve the immediate state.

AH.5 Skills, resources, Ultimate and World Fantasm
   Only part of a character's broader kit is presented as directly selectable active buttons in World
    Mode, consistent with the already-defined turn-combat rule that selectable active-skill count is
    character-dependent.
   The visible active set is presented as a compact set of buttons. Characters may have different
    counts/layout needs where their kit requires it.
   Combos and QTE behaviors are inherent authored behaviors/triggers of skills and actions; they are
    not separate skill entries added to the character's active-skill count.
   Character-specific resources receive bespoke HUD meters rather than being normalized into one
    universal action-combat resource.
   Ultimate state is shared with the canonical character state used by turn combat. The normal
    activation threshold remains 100 percent unless that character's rules define otherwise.
   For evolved/multi-threshold Ultimates such as 200/300 percent states, action combat must not
    require a slow menu. Fast inputs such as tap-count/double-tap/hold or another character-
    appropriate direct gesture select the intended tier.
   Transformations/forms do not receive a universal extra context button; their activation is handled
    through the character's actual authored inputs/skills/states.
   World Fantasm activation is part of the character's own kit rather than a universal extra button
    shared identically by every character.
   World Fantasm presentation uses a short, non-disruptive character-specific activation cue/cut-in
    followed by the live field transformation. Repeated uses may use an abbreviated presentation so
    combat flow is not repeatedly interrupted.
   Ruler Commands are not available in ordinary World Mode combat because the Ruler is fighting
    directly there.
   Consumable inventory items are not usable during action combat.



                                                Page 101

<PARSED TEXT FOR PAGE: 102 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AH.6 Encounter flow, environment and hostility
   Ordinary action encounters begin seamlessly in the world without an arena/loading transition.
   Enemies can pursue across meaningful physical distances.
   Disengagement is governed by physical/situational logic - distance, line of sight/concealment,
    mobility, enemy motivation, orders, territory and encounter design - rather than a tiny universal
    leash radius.
   The player may attack physically present creatures/characters without requiring them to have been
    pre-labeled hostile. Consequences derive from the world state and the target's/faction's response.
   Environmental causality remains live during combat: cliffs, water, eligible destructible structures
    and hazards can matter.
   Friendly fire is off by default unless an explicit ability, rule or world condition enables it.
   Ordinary pause completely pauses offline single-player action combat.

AH.7 Traversal-combat continuity
   Underwater combat remains action combat with movement modified for underwater traversal
    rather than switching to a separate reduced combat minigame.
   Characters capable of flight may fight in free three-dimensional movement.
   Mounted combat remains available where the mount, character and ability are physically
    compatible; combat does not universally force dismounting.
   Combat may continue while airborne, swimming, underwater, flying, climbing or mounted when the
    involved capabilities/animations allow it.

AH.8 Bosses, hybrid encounters and shared rule core
   Hybrid action/turn phase changes are driven by boss/encounter mechanics rather than a universal
    player-controlled mode switch.
   HP, statuses and relevant character resources persist across action/turn phase transitions unless an
    explicit boss mechanic transforms them.
   Action combat and turn combat resolve through the same authoritative effects, conditions,
    Authority, World Fantasm and character-state rule core. Presentation/execution timing differs, but
    the underlying authored effects are not duplicated into unrelated systems.
   No separate action-combat assist mode is required.

AH.9 Input/readability constraints
   The action HUD should stay compact despite deep kits; fast direct inputs are preferred over combat
    menus.
   Character-specific exceptions are allowed where they preserve the authored kit without forcing
    every character into one template.
   Difficulty/readability should come primarily from animation, timing, spatial state and player
    knowledge rather than highly conspicuous warning UI.




                                                 Page 102

<PARSED TEXT FOR PAGE: 103 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AI. Starting World, Opening, Recruitment and Save Continuity
STATUS: CURRENT AUTHORITATIVE BASELINE. This checkpoint records answers 56-100 and related
clarifications. Where the Creative Director delegated a choice, the implementation/narrative choice
below is adopted unless later overridden.

AI.1 Starting-world generation and tone
   The starting region/world is seed-generated rather than one fixed authored biome. Generated
    geography becomes canonical for the one persistent world once created.
   The opening region should be visually beautiful while carrying a mild but credible sense of danger
    rather than presenting immediate apocalyptic spectacle.
   The first world/dimension is broadly historical-medieval in technological and visual character. As
    the protagonist reaches stronger worlds/dimensions, settings may trend progressively farther into
    high fantasy, post-apocalyptic, cyber/technological and other increasingly extreme genres. This
    progression is not a rule that the same starting region transforms into those genres.
   The protagonist begins in plain wilderness. There is no mandatory dramatic object, ruin, battlefield
    or cosmic set piece at the spawn point; a few ordinary local monsters/wildlife may be present.
   The protagonist begins alone.
   The opening may use a short world-establishing cinematic/presentation, after which control is
    handed to the player. There is no conventional forced tutorial sequence.
   The protagonist begins with no autobiographical memory of his past/origin. Functional knowledge
    needed to act, communicate and understand immediate reality is retained unless later story content
    establishes otherwise. Whether he came from another place/world is initially unknown.
   Early play should allow exploration before combat.

AI.2 Early danger, landmark and encounter policy
   The first hostile encounter should communicate vulnerability rather than serve as a combat tutorial.
    Since the protagonist awakens unarmed, the first meaningful threat should be avoidable and
    readable enough that evasion is a valid response.
   The opening region does not showcase late-game entities merely for spectacle. Late-game power
    belongs to later/higher dimensions and need not exist in the starting dimension.
   The first serious boss/elite encounter is preferably an optional territorial monster/creature
    generated or authored as part of the local ecology, not a compulsory tutorial boss.
   Nothing in the opening is forced solely to teach a system. Serious encounters, dungeons, territories
    and faction conflicts may be ignored, bypassed or encountered in a different order.
   Because the starting biome is randomized, the main orientation landmark is also seed/context-
    dependent: use a strong natural or historical-medieval landmark appropriate to the generated
    region rather than one universal landmark.
   The initial region should not be divided into many artificial sub-biomes for variety. It should read as
    one coherent regional environment.
   A settlement becomes discoverable after some exploration rather than being presented immediately
    at spawn.
   Time until the first meaningful intelligent-NPC contact depends on where and how the player
    explores.



                                                 Page 103

<PARSED TEXT FOR PAGE: 104 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AI.3 First factions, territory and dungeon
   The starting area is designed around three relevant factions.
   Faction relationships should be state-driven rather than a fixed mandatory opening war. Baseline
    generation may include one active tension/conflict relation while leaving the third faction differently
    aligned, but simulation may alter this.
   At least one early faction may be led by/contain a low-level Ruler controlling ordinary Territory. It
    does not begin with a Domain.
   The beginning does not immediately foreground ancient high-technology/advanced-civilization
    ruins. Dimensional anomalies also appear later rather than at spawn.
   The first discoverable dungeon is selected to fit the generated region and historical-medieval layer:
    natural caves, mines, crypts, forts/ruins or hybrids are valid. It is not a dedicated tutorial dungeon.
   A dungeon may be discoverable long before the protagonist is capable of clearing it.
   A first claimable Territory may arise from multiple valid routes: developing an
    unclaimed/abandoned holding, conquest, transfer or another world-valid method.
   There is no scheduled 'first Territory' milestone. The player may remain territoryless for a very long
    time, potentially a large fraction of the game.
   Awakening a first Domain Core is a later major progression event, never a tutorial checkbox.
   The protagonist's eventual Main Territory can be anywhere. The initial spawn region is not
    privileged as the inevitable capital.

AI.4 Production philosophy after core playability
   There is no small 'vertical slice content count' that defines the intended game. Once the core game is
    certified playable, production shifts heavily toward designing a large amount of characters, enemies,
    bosses, locations, equipment and other content before the Creative Director plays the full experience.
   Therefore exact launch-slice counts for bosses, enemy families and settlement-interior clusters are
    not treated as current creative limits.

AI.5 Ruler access, gacha and first recruitment
   The canonical gacha/contract-pull interface does not unlock immediately on first claiming territory.
    It becomes available after the protagonist has successfully kept a territory under continuous control
    for more than one in-game month. There is no subordinate-character requirement for Ruler status.
   The first pull is not a scripted fixed-character pull and does not use a restricted introductory
    character pool. It uses the ordinary eligible pool/rules available at that time.
   There is no tutorial gate. Once the territory-duration condition is satisfied and gacha becomes
    canonically available, normal banners follow the actual world/system conditions.
   The initial pull resource is supplied as a small first-Ruler dimensional allocation/initial contract
    resource generated by the canonical preparation mechanism rather than an out-of-world tutorial
    gift.
   First acquisition of a Character Identity may use a substantial summon/acquisition presentation;
    repeat/duplicate-equivalent acquisitions may use shortened presentation.
   A summoned Manifestation may be confused or lack full understanding of the metaphysical
    acquisition situation when that fits the character/story.
   The game does not pre-grant two opening companions before normal gacha. Party growth follows
    actual acquisition.


                                                 Page 104

<PARSED TEXT FOR PAGE: 105 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   All gacha/pullable Character Identities are intended to be female. This does not imply that all NPCs,
    enemies, faction members or Rulers in the world must be female.

AI.6 Narrative authorship and spoiler policy
   The engineering/narrative assistant owns construction of the actual story rather than revealing a
    predesigned plot outline to the Creative Director.
   Story design should use the already-established preference calibration: FGO, Honkai Impact 3rd Part
    1, NieR:Automata and Umineko qualities; long mysteries; recontextualization; metaphysical layers;
    dense meaningful information; tragedy; morally unclean conflicts; coherent antagonists;
    Ruler/protagonist POV; authored arcs that remain subordinate to persistent world history.
   The Creative Director may be asked extensive preference/world-rule questions, but future plot
    solutions, twists, hidden identities and story answers should normally be kept secret so the Creative
    Director can experience them as a player.
   The exact point at which the protagonist learns the metaphysical truth behind the colorful
    gacha/contract UI is therefore a narrative decision, not a system-design question requiring advance
    disclosure.

AI.7 Persistent-world save model and uninstall-safe recovery
   The game has one canonical persistent world/save from the player's perspective. There are no
    routine parallel save slots.
   Clear Data / explicit new-world reset is the operation that destroys that canonical history and starts
    over. It must require strong confirmation and should never be conflated with ordinary settings
    cleanup.
   Live authoritative mutable state remains in the versioned SQLite-centered save architecture.