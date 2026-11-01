# v0.40 historical master — source extraction 5/10
Source: Offline_Adult_Gacha_RPG_Master_Architecture_Baseline_v0.40_FINAL_PRECODING_FREEZE_RELEASE.docx, 175 pages; uploaded 2026-11-01. Parsed original text lines 3601–4500. Historical source only: newer explicit instructions, verified latest code decisions and v0.41 (missing) may supersede conflicting clauses. Original DOCX remains higher-fidelity for embedded images and precise tables.

    intended mature presentation should be authored directly where the character/content is eligible.




                                                 Page 72

<PARSED TEXT FOR PAGE: 73 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AC. World Mode Exploration - Final Architecture
AC.1 Exploration philosophy
   Exploration is a core pillar and is valuable for its own sake; the world does not need an interactable
    object, chest, enemy or puzzle every few metres.
   Wilderness can contain long quiet stretches when ecology/geography warrants it. Discovery density
    should be meaningful but not artificially sparse: important discoveries occur often enough to sustain
    curiosity without turning the world into a checklist.
   Central rule: if the world rules physically permit an action, the game should try to allow it.
    Progression changes capability rather than granting arbitrary permission flags.
   Exploration is mandatory to the broader progression loop because it reveals routes, civilizations,
    alliances, secrets, repeatable dungeons, resources, hidden objects, Domain opportunities and other
    world knowledge.

AC.2 Map knowledge and discovery
   The map begins largely unknown and is filled through actual exploration, observation, reports and
    acquired cartographic/intelligence sources rather than a universal tower-reveal mechanic.
   Maps are world objects/information and can be bought, stolen, inherited, discovered, produced by
    factions or recovered from ruins.
   Map information can be incomplete, inaccurate or outdated because roads, borders, rivers,
    settlements and terrain can change through combat, disasters, construction and world events.
   Hidden locations are not automatically marked merely by proximity; observation, senses,
    intelligence, reports or relevant abilities determine discovery.
   The full map is available when known, while permanent minimap/compass presentation should be
    configurable rather than mandatory.
   Knowledge discovered by companions/subordinates can propagate to the faction and appear in
    Ruler Mode once credibly reported. A subordinate can discover a dungeon, route or settlement while
    the player is elsewhere.

AC.3 Physical-world requirement
   Dungeons, settlements, ruins, resource sites, towers, pocket dimensions, ships, giant corpses,
    underground complexes and other exploration content are physical/world entities directly reachable
    in World Mode rather than menu-only instances.
   Once discovered/explored, relevant locations become available in the Ruler interface for strategic
    management, orders, intelligence and repeat access.
   Dungeons can be claimed, conquered, altered, collapse, change owners, evolve, awaken Cores or
    become Domains when their nature supports it.
   Repeatable/farmable dungeons use an in-world regeneration explanation such as self-
    reconstruction, temporal looping, replenishing ecology or recurring dimensional phenomena rather
    than arbitrary calendar resets.

AC.4 Puzzles and obstacles
   Environmental puzzles exist when they make sense, but they should not become routine friction.
    Complexity is acceptable; annoyance and repetitive busywork are not.

                                                  Page 73

<PARSED TEXT FOR PAGE: 74 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Many obstacles support multiple solutions: operate the mechanism, find another route, climb, fly,
    teleport, phase, tunnel, break the wall, persuade someone to open it, use a species ability, or exploit
    another legitimate capability.
   Overwhelming power can brute-force some obstacles, but doing so can destroy information, loot,
    infrastructure or create other consequences.

AC.5 Stealth, senses and capability interaction
   Stealth is an optional capability rather than a separate mandatory stealth-game layer.
   Visibility, sound, disguise, concealment, assassination and detection can matter when useful.
   Species, equipment, skills and power systems can radically alter stealth and sensing: invisibility,
    scent masking, spiritual concealment, shapeshifting, phasing, suppressing presence, supernatural
    perception and equivalent mechanics.
   General rule: systems may affect one another when useful and computationally feasible rather than
    being artificially isolated.

AC.6 Gathering, ecology and automation
   Personal exploration can physically gather plants, minerals, relics, creature remains and other
    resources.
   Rare or delicate harvesting can require relevant expertise, tools, containment or preservation rather
    than a universal interaction button.
   Once a resource source is understood and secured, repeated extraction can be delegated through
    territory orders, subordinates or dispatch systems instead of manual farming.
   World populations/resources can be depleted locally and, where the ecology supports it, even driven
    to extinction. Reappearance can occur through migration, reproduction, dimensional reintroduction,
    resurrection, breeding, preserved stock, summoning or other valid world mechanisms rather than
    arbitrary respawn.
   Species can also be captured/subjugated/domesticated/enslaved where world mechanics allow,
    becoming populations, army units, summons, mounts or other faction assets rather than existing
    only as killable monsters.

AC.7 NPCs and settlements
   Not every minor NPC needs deep interaction. Nearby ordinary NPCs can use lightweight contextual
    behavior/dialogue; important NPCs receive richer persistent interaction.
   Important instantiated NPCs physically travel, work, fight, visit places and follow their world
    routines when nearby rather than teleporting through invisible quest logic.
   Settlement services exist because the civilization actually supports them, not because every RPG
    town must have the same checklist.
   Services can disappear when buildings/NPCs are destroyed, captured, leave, die, become inaccessible
    or change faction.
   The player can physically take/loot objects when world mechanics permit it. Consequences remain in
    the existing lightweight faction/reputation/hostility layer rather than a detailed legal simulator.

AC.8 Field encounters
   Ordinary enemies primarily exist physically in the ecology/world rather than appearing as abstract
    random encounters.
                                                   Page 74

<PARSED TEXT FOR PAGE: 75 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Predators can hunt prey; patrols, caravans, monsters, migrations and armies can move through
    nearby regions independently of the player.
   Long periods without combat are valid in peaceful, empty or remote environments.
   Danger is communicated diegetically through tracks, corpses, environmental destruction, faction
    warnings, character knowledge, sensing abilities and other world evidence instead of arbitrary
    recommended-level barriers.

AC.9 Exploration progression and death
   Progression opens traversal through real capabilities: flight, pressure resistance, underwater
    breathing, dimensional travel, heat/cold tolerance, phasing, stronger destruction, environmental
    immunity and equivalent mechanics.
   The Ruler uses the previously defined canonical death/revival/regression system in World Mode;
    there is no ordinary checkpoint respawn that erases consequences.
   A defeated companion who is not canonically dead requires extraction/recovery/healing rather than
    automatically standing up after battle.
   Fast travel exists only through actual teleportation abilities, structures, vehicles, routes or relevant
    objects/capabilities.
   A sufficiently advanced faction can build a dense transport/teleport network and legitimately
    trivialize routine travel inside secure territory because that convenience was earned.

AC.10 Environment-specific exploration grammar
World Mode must not explain, navigate or present every environment through the same ground-travel
grammar. Movement physics, danger communication, UI cues, discovery logic and relevant resources
adapt to the medium.
   Ground/surface: terrain slope, roads, cover, tracks, weather, rivers, vegetation, settlements and
    landmarks dominate navigation and danger cues.
   Sky/aerial: altitude, wind, thermal currents, storms, cloud layers, aerial predators, airspace control,
    vertical landmarks and landing opportunities matter. Navigation is genuinely three-dimensional
    rather than a flat map with height added.
   Underwater: depth, pressure, currents, visibility, temperature, buoyancy, breathing/oxygen where
    relevant, sonar/senses, bioluminescence and underwater terrain/civilizations alter movement and
    information presentation.
   Underground: cave topology, vertical strata, collapses, air/heat, echoes/seismic cues, tunnels, buried
    structures and limited sight reshape navigation.
   Space: there is no assumed ground/up direction. Relative position/velocity, orbit, vacuum, radiation,
    propulsion, celestial bodies, sensor range and ship capabilities drive travel and danger. Space should
    not feel like “flying over a very large map.”
   Pocket dimensions/abnormal realms: the local rules of that realm can replace ordinary assumptions
    entirely (gravity, time, direction, visibility, geometry, causality, etc.).
   Environment-specific information should be explained diegetically whenever possible through the
    controlled character, companions, equipment, sensors and learned knowledge rather than one
    omniscient generic HUD.




                                                   Page 75

<PARSED TEXT FOR PAGE: 76 / 175>

                                                  OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AC.11 Feasibility and simulation resolution
The exploration target is feasible only through adaptive simulation fidelity. Nearby important entities
are physically simulated; distant ecology, armies, migrations, gathering and NPC routines are
summarized statistically/event-wise until they become relevant. Environment-specific traversal is
implemented through shared movement/interaction interfaces with medium-specific capability modules
rather than building unrelated engines for ground, sky, underwater and space.




                                                Page 76

<PARSED TEXT FOR PAGE: 77 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AB. Economy, Resources, Crafting and Trade
The economy is systemic enough to make production, ownership, scarcity, trade routes, crafting and
stockpiles matter, but deliberately stops short of becoming a detailed finance simulator. Resources exist
in the world first; menus only expose and manage that state.

AB.1 Currencies and exchange
   Different civilizations/worlds may use different currencies. There is no universal mandatory gold
    standard.
   Some societies may use barter, favors, energy, souls, reputation or other exchange systems instead of
    conventional money.
   Exchange values can change according to availability, trade access, wars, scarcity and political
    conditions.
   The Ruler can establish a new currency once the territory has sufficient institutional/economic
    capacity.
   Detailed currency-collapse, inflation and monetary-policy simulation is outside baseline scope.
   High-level economic interference is allowed where it can be expressed through concrete game
    actions such as blockades, monopolies, counterfeiting or sabotage where lore permits, disrupting
    production, flooding a market with goods, controlling strategic resources, manipulating exchange
    access, or severing trade routes. It should not require a stock-market simulator.

AB.2 Merchants, inventories and local scarcity
   Merchants own finite inventories rather than infinite shop lists.
   Inventories are replenished through production, suppliers, caravans, trade routes, adventurers and
    world events.
   Buying out a scarce material can make it genuinely unavailable locally until resupply or new
    production occurs.
   Prices can vary substantially between regions based on local supply, demand, danger, transportation
    and politics.
   Some objects can be effectively priceless and require barter, favors, conquest, theft, diplomacy or
    special events rather than simple currency.
   Most transferable objects can be offered for sale, but NPCs can refuse worthless, unwanted or
    unusable goods.

AB.3 Crafting knowledge and invention
   Crafting requires appropriate knowledge/recipe understanding plus materials, tools and where
    relevant a suitable facility.
   Recipes can be learned from traditions, teachers, texts, reverse engineering and experimentation.
   Characters can invent new recipes/items when their skills, knowledge and circumstances support it.
   Crafting can fail or produce inferior outcomes when skill, tools, process or materials are inadequate.
   The same nominal recipe can produce substantially different results based on crafter, forge,
    materials, environment, techniques and special conditions.
   Unusual materials should alter item identity/mechanics, not merely attach small numeric bonuses.
   Exceptional crafting can create new named/unique equipment.


                                                  Page 77

<PARSED TEXT FOR PAGE: 78 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AB.4 Forging, production and delegation
   Forging/crafting consumes real world time and can require workshops, forges, Domain facilities or
    specialized environments.
   Once a process is sufficiently understood/mastered, routine production can be delegated to NPC
    craftsmen or territory orders.
   Important craftsmen can become strategically valuable characters worth recruiting, protecting,
    negotiating for or capturing where world rules permit.
   Crafting facilities and specialists are world entities whose destruction, capture or absence can
    interrupt production.

AB.5 Civilizational traditions and reverse engineering
   Civilizations/factions can possess distinct crafting traditions, magical systems and technologies.
   These traditions may initially be inaccessible outside their culture/faction.
   The player can learn, steal, merge, teach or otherwise acquire such traditions through appropriate
    world actions.
   Captured equipment can be reverse-engineered when the faction has sufficient knowledge and
    capability.
   Reverse engineering may fail or remain incomplete when essential requirements are missing, such
    as a unique material, species trait, divine authority, lost technique or special environment.

AB.6 Trade routes and strategic resources
   Strategically important/high-value cargo can move physically through caravans, ships, airships,
    portals or other world-appropriate logistics.
   Low-value bulk flows can remain aggregated for performance and usability.
   The player can establish monopolies over rare resources when the world state genuinely permits it.
   NPC factions can respond to monopolies by seeking substitutes, opening new sources, stealing
    knowledge, raiding logistics, negotiating, changing doctrine or going to war.
   Economic power therefore comes from control of real production and access, not abstract finance
    minigames.

AB.7 Territory economic policy
   Taxation exists only at broad strategic level; individual tax-law simulation is outside scope.
   The Ruler can set broad policies such as taxation intensity, military-production priority, food
    priority, strategic-export restrictions and trade encouragement.
   These policies affect aggregate production, availability, pressure and strategic behavior without
    requiring household-level accounting.
   Population-wide aggregate wealth is optional and not a core simulation requirement.

AB.8 Character ownership, property and debt
   Important characters can personally own meaningful equipment, property, workshops, businesses
    and wealth.
   Characters/factions can lend resources and create debts when useful to an event or strategic
    interaction.


                                                  Page 78

<PARSED TEXT FOR PAGE: 79 / 175>

                                                  OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   A deep universal loan/credit system is not required; debt remains an available mechanic rather than
    a major simulation pillar.

AB.9 Stockpiles, siege and endurance
   Territories and important settlements can maintain meaningful stockpiles of food, ammunition,
    materials, energy and other strategic resources.
   Stockpiles matter during siege, isolation, dimensional separation and disrupted logistics.
   A well-prepared isolated city can survive for long periods if it truly has the stored resources and
    infrastructure to do so.
   Stockpiles are managed at useful aggregate levels rather than requiring constant manual accounting
    of every crate.

AB.10 Resource-state principle
Core rule: resources exist because the world produces, stores, moves, owns, consumes or loses them.
Shop screens, crafting menus, trade interfaces and Ruler Mode panels are representations of that
underlying world state, not separate magical inventories.

AB.11 Feasibility / anti-overdesign rule
This economy remains feasible if local scarcity, important inventories, strategic trade routes and
stockpiles are simulated explicitly while bulk production and ordinary commerce are aggregated.
Detailed inflation modeling, household wealth simulation, stock markets and universal credit systems
are intentionally excluded unless a later specific mechanic genuinely needs them.




                                                Page 79

<PARSED TEXT FOR PAGE: 80 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AC. Factions, Diplomacy and War
This checkpoint defines the current geopolitical and warfare baseline. The strategic layer remains an
extension of the persistent RPG world rather than a reputation-score, legal, morality or grand-strategy
simulator.

AC.1 Faction ontology
   Any sufficiently organized group can function as a faction: kingdoms, clans, guilds, cults,
    corporations, monster societies, pantheons, pirate fleets, hive minds, dimensional coalitions and
    other world-specific structures.
   Characters may belong to several factions simultaneously when the world/lore permits it.
   Factions can contain subordinate factions as well as individual members.
   Factions can merge, split, collapse, reform, change leadership, ideology, culture or political form, and
    create successor states.
   A defeated faction can survive without territory as an exile government, wandering army, hidden
    organization, fleet, cult, resistance or other persistent form.
   Such factions can later be restored or regain territory.

AC.2 No universal reputation/morality/legal score
   There is no universal reputation score, morality meter, alignment reward track, legal-score system
    or real-world ethical judgment layer.
   Faction and character reactions arise from concrete interests, relationships, history, ideology, fear,
    respect, dependence, hostility, culture and strategic calculation rather than a single numerical
    reputation axis.
   Real-world legal, moral or ethical categories are not imposed as external gameplay restrictions on
    fictional world actions.
   Consequences remain fully systemic: factions and characters can retaliate, ally, fear, hate, exploit,
    resist or cooperate according to their own interests and values.
   This does not require the game to label actions as good/evil or lawful/unlawful from an external
    perspective.

AC.3 Diplomacy
   Diplomacy is driven primarily by actual interests rather than friendliness.
   Possible agreements include alliances, defensive pacts, non-aggression, trade, access rights,
    vassalage/subordination, tribute, guarantees and temporary coalitions.
   Treaties are breakable. Factions can violate agreements when they judge the consequence
    worthwhile.
   Treaties do not require detailed legal-document simulation; only strategically meaningful terms are
    represented.
   Leadership personality and capability matter. A change of Ruler can materially alter a faction's
    foreign policy.
   No universal diplomatic 'trust meter' is required, though actors retain relevant memories/history of
    promises, betrayals and prior conduct.




                                                   Page 80

<PARSED TEXT FOR PAGE: 81 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AC.4 Information and intelligence
   The player does not automatically know exact enemy army size, reserves, resources, alliances,
    internal instability, Ruler power or secret capabilities.
   Information is acquired through scouts, spies, trade, allies, captured documents/characters, magical
    or technological sensing, direct observation and other world-appropriate sources.
   Reports can be outdated or wrong, but this should be less common than accurate reporting when the
    player's intelligence network is competent.
   Espionage exists at strategic level rather than as a large individual-agent management simulator.
   Espionage actions may investigate strength, sabotage infrastructure, steal knowledge, support
    internal opposition, infiltrate organizations or target important characters.

AC.5 War initiation and objectives
   War does not require an artificial Casus Belli system. A faction can simply attack when its strategy,
    desires or circumstances lead it to do so.
   War objectives may include conquest, destroying or capturing a Domain Core, seizing a
    person/artifact/resource, rescue, tribute, subordination, liberation, extermination, containment,
    delay or other authored/systemic goals.
   Factions may contribute forces without formally joining the entire conflict through mercenaries,
    volunteers, covert support, expeditionary forces or similar arrangements.
   Conflicts can expand organically as allies, affiliates and subordinate Rulers intervene.

AC.6 Armies and force composition
   Armies can contain ordinary troops, elites, monsters, constructs, summons, named characters and
    Rulers depending on faction identity.
   Defeated/captured/subordinated species can potentially become troops, summons, military assets or
    population groups under the player's faction when world mechanics permit.
   Ordinary soldiers are aggregated for persistence/performance; strategically important commanders
    and named units remain individual persistent characters.
   Named commanders materially affect tactics, morale, coordination and army effectiveness, but
    organized forces can continue without them at reduced capability.
   Army effectiveness depends on training, doctrine, equipment, morale, terrain, logistics,
    supernatural/technological capabilities, commanders and raw individual power rather than troop
    count alone.
   A small force of extremely powerful beings can destroy a vastly larger weak army when the power
    gap justifies it.
   Large armies can still threaten strong beings through specialized weapons, formations, attrition,
    barriers, numbers and logistics when the gap is not absolute.

AC.7 Strategic and physical battle resolution
   Distant wars resolve through strategic simulation rather than rendering every soldier.
   When the Ruler personally enters a battle, the nearby battlefield is promoted into World Mode while
    the broader battle continues at strategic simulation resolution.
   Player actions in the physical battle can immediately affect strategic state, including killing/capturing
    commanders or Rulers, destroying key infrastructure, breaking formations or seizing objectives.
   Battles can last hours or days rather than resolving as short isolated encounters.
                                                   Page 81

<PARSED TEXT FOR PAGE: 82 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Armies can retreat. Pursuit can cause additional losses.
   Named wounded/disabled characters require real recovery afterward.

AC.8 Siege, destruction and capture
   Cities, fortresses and Domains can be physically besieged. Walls, gates, barriers, stockpiles, supplies,
    terrain, defenders and Domain structures matter.
   Sieges can be bypassed using valid capabilities such as flight, teleportation, tunnelling, phasing or
    overwhelming destruction when defenders lack counters.
   Destroying a settlement is generally much easier than conquering and retaining it.
   Defenders may deliberately destroy strategic resources or infrastructure before losing them.
   A Domain Core reduced to zero durability is broken/destroyed, not captured.
   Capturing a Domain Core therefore requires preserving it while establishing control through
    occupation, Authority suppression/contest, ritual, sealing or other appropriate capture mechanics.
   Core durability is a physical/metaphysical failure threshold; capture is a separate objective state.

AC.9 War termination and aftermath
   There is no generic 'negotiate peace at any time' system that instantly ends a war.
   Wars end through actual strategic resolution: victory, defeat, withdrawal, collapse, objective
    completion, subordination, exhaustion, destruction, ceasefire-like event mechanics or other world-
    specific outcomes.
   Defeated factions can refuse disappearance and continue as exiles, remnants, insurgent groups or
    successor factions.
   Conquered populations and factions retain identity rather than instantly becoming generic members
    of the winner's empire.
   There is no generic occupation-morality/autonomy meter. Control outcomes depend on power,
    culture, contracts, leadership, resistance, logistics and explicit systems.
   Fictional populations may be killed, expelled, enslaved, subordinated, transformed or otherwise
    acted upon where game/world mechanics permit, with demographic, economic, military and
    geopolitical consequences rather than an external morality score.
   Captured named enemies may be imprisoned, recruited, contracted when mechanics permit,
    exchanged, released, executed or used by relevant abilities/rituals according to player/world choice.

AC.10 Core war principle
War is an extension of the same persistent world. Armies, resources, characters, territory,
infrastructure, destruction, capture and recovery are authoritative world state; the strategic interface
simply lets the Ruler operate at the appropriate scale.

AC.11 Difficulty implications
   There is no hidden equalization of military power. A superior force can crush a weaker one, and a
    weak force should avoid impossible battles unless it has asymmetric advantages.
   Difficulty comes from incomplete information, multi-front commitments, travel/logistics, persistent
    losses, strategic objectives, powerful commanders/Rulers, Domain defenses and the cost of retaining
    conquered territory.



                                                   Page 82

<PARSED TEXT FOR PAGE: 83 / 175>

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

<PARSED TEXT FOR PAGE: 84 / 175>

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

<PARSED TEXT FOR PAGE: 85 / 175>

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

<PARSED TEXT FOR PAGE: 86 / 175>

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

<PARSED TEXT FOR PAGE: 87 / 175>

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

<PARSED TEXT FOR PAGE: 88 / 175>

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

<PARSED TEXT FOR PAGE: 89 / 175>

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

<PARSED TEXT FOR PAGE: 90 / 175>

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

<PARSED TEXT FOR PAGE: 91 / 175>

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

<PARSED TEXT FOR PAGE: 92 / 175>

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

<PARSED TEXT FOR PAGE: 93 / 175>

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