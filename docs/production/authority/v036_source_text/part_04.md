# v0.36 full source text — segment 4
Original: Offline_Adult_Gacha_RPG_Master_Architecture_Baseline_v0.36_checkpoint_RulerOverlord.docx (143 pages), uploaded 2026-11-01. Derived text extraction, not a replacement for original embedded figures/tables. SOURCE LINE WINDOW 3001–4000. Historical decisions may be superseded by 7★ overlays.

Fusion
This checkpoint completes the current Ruler/Domain foundation. It supersedes any earlier implication of
a finite contract-slot cap or a separate per-contract strength statistic. Domain Cores are conceptual
territory anchors rather than elemental-only systems.

Y.1 Contract scale and hierarchy
   There is no universal finite Contract Capacity. A sufficiently powerful Ruler can sustain arbitrarily
    large numbers of contracts.
   Characters do not consume abstract contract slots based on their power.
   Contract enforcement does not have a separate generic Contract Strength statistic. Its practical
    strength derives primarily from the Ruler's own Authority/power and any explicit special mechanics.
   As the Ruler grows stronger, their contracts become correspondingly harder to interfere with
    without needing a separate leveling system.
   To forcibly take or overwrite another Ruler's contracted character, an attacker normally must first
    defeat/kill/neutralize the Ruler or otherwise sever the Authority link, unless a specific exceptional
    ability explicitly bypasses this rule.
   When a Ruler dies, existing succession, dynasty, absorption and neutralization rules still determine
    what happens to territory and contracts.

Y.2 Subordinate Rulers and delegated Authority
   An Overlord appoints subordinate Rulers from important roster characters and grants them a
    portion of Overlord Authority.
   The delegated Authority allows the subordinate Ruler to govern an affiliated territory and operate
    Ruler-level systems appropriate to the grant.
   Subordinate Rulers may request reinforcement from the Main Territory, including literal temporary
    deployment of characters from the Overlord's roster.
   Subordinate Rulers can potentially access their own gacha/contract system using delegated Authority
    and their own or allocated resources. This is a valid strategic growth path rather than an error
    condition.
   The Overlord can still directly command subordinate-Ruler contractants because the delegated
    Authority remains nested under the higher hierarchy.
   Permissions such as independent war declaration, expenditure ceilings or access to high-order
    Domain powers can be configured broadly without turning the system into a bureaucratic
    permission simulator.

Y.3 Domain Reserve and power-dependent costs
   Mature territories/Domain Cores generate a reusable Domain Reserve over time. This is the general
    energy/capacity reservoir behind many Ruler and Domain actions.
   There is no rule that every Ruler power costs the same resource amount or even only Domain
    Reserve. Costs are power-dependent and may additionally require mana, materials, sacrifices,
    infrastructure, charge time, cooldown-like recovery, specific structures or other authored conditions.
   Affiliated territories can transfer part of their generated Domain Reserve/resources toward the Main
    Territory.
                                                     Page 60

<PARSED TEXT FOR PAGE: 61 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Extraction is not free: transferring too much weakens the affiliate's local defenses, regeneration,
    construction, projection or other Domain functions while the power is absent.
   A Ruler may deliberately overdraw a Domain for a short-term burst of power. Overdraw can
    damage, destabilize, exhaust or temporarily reduce the Core afterward.

Y.4 Core evolution and Ruler/Core mismatch
   A Domain Core can progress and evolve independently from its current Ruler.
   Ruler strength and Core strength can therefore become mismatched.
   A powerful Ruler may inherit or control an underdeveloped Core; conversely, a weak successor may
    inherit a Core far beyond their safe control capacity.
   A weak successor can suffer reduced access, instability, backlash, partial lockout or dangerous Core
    behavior until sufficient control is established.
   This mismatch is intentional and allows inherited high-level territories to remain strategically
    dangerous rather than becoming free power.

Y.5 Core traits and historical crystallization
   Domain Cores do not need to store every historical event as a gameplay modifier. The world-history
    database already preserves important events separately.
   A Core may, however, crystallize a small number of mechanically meaningful Core Traits/Aspects
    from repeated or extraordinary territorial history.
   Examples: centuries dominated by dragons may create a Draconic Aspect; repeated temporal
    disasters may create a Temporal Scar; a territory built around resurrection and death rites may
    develop a Death Aspect.
   These traits affect Domain evolution/fusion and territory-scale abilities without turning history into
    a life-simulation spreadsheet.
   Core Traits can be innate, acquired, lost, suppressed, corrupted or transformed by major events.

Y.6 Sentient Cores and character emergence
   A Domain Core can become intelligent or sentient in unusual cases.
   Sentience does not automatically remove the Core from its territorial function.
   A sufficiently developed sentient Core may create or acquire a playable Core Avatar/Incarnation and
    therefore become a character while the Core remains the territory anchor.
   If a specific mechanic fully extracts/incarnates the Core itself, territorial consequences must be
    explicit because removing the anchor can weaken, destabilize or transform the Domain.
   Core-born characters inherit identity/history from the Domain rather than being unrelated
    generated characters.

Y.7 Conceptual Domains - not elemental-only
   Domain Cores are defined by Concepts/Aspects rather than an elemental-only chart.
   Valid themes can include conventional elements (Ice, Fire, Time) but also Draconic, Death, Clown,
    Labyrinth, Dreams, Blood, War, Memory, Void, Music, Machinery, Hunger, Stars or any other
    sufficiently defined concept.
   A Domain can hold several Aspects, and one Aspect may itself contain subrules/abilities rather than
    behaving like a simple damage element.


                                                  Page 61

<PARSED TEXT FOR PAGE: 62 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Domain identity therefore describes what the territory fundamentally is/does, not merely what
    elemental damage it buffs.

Y.8 Core fusion and emergent concept synthesis
   Conquered/absorbed Cores preserve conceptual identity during fusion instead of dissolving into
    generic power.
   Fusion can produce emergent abilities from interactions between Aspects. Example: Time + Ice may
    synthesize an Absolute-Zero/Time-Freeze family of Domain abilities.
   The result is not restricted to elemental combinations. Draconic + Death might create a death-
    dragon/ancestral-necrotic Domain expression; Clown + Time might produce temporal
    absurdity/looping-performance effects; Labyrinth + Memory might create spaces that reconstruct
    remembered layouts.
   Fusion outcomes can be asymmetric: Core A absorbing Core B does not have to equal Core B
    absorbing Core A.
   Fusion can preserve some original abilities, transform others, create new synthesis abilities,
    introduce incompatibilities or destabilize the resulting Core.
   To keep this scalable, the engine should use a data-driven Concept Synthesis Grammar: authored
    Aspect properties + interaction rules + bounded procedural combinations, rather than hand-
    authoring every possible pair of Domains.
   Rare/high-value combinations can later receive bespoke authored upgrades, VFX, names and
    character/story content after the systemic fusion already works.

Y.9 Difficulty implications
   Unlimited contracts do not imply unlimited active intervention. Difficulty is preserved through
    Authority projection cost, travel, deployment limits, multi-front threats, recovery, Core vulnerability
    and the inability to personally solve every crisis.
   Delegating Authority creates capable subordinate Rulers, but it also creates additional strategic
    actors that can consume resources, request reinforcement and generate commitments.
   Core evolution/fusion increases maximum power but also increases the value of the territory as a
    target and the consequences of instability or loss.
   The player can become absurdly powerful without the world secretly matching their level; instead,
    higher power exposes higher-rank worlds, enemies and geopolitical stakes.

Y.10 Feasibility
This system remains feasible if Concept fusion is implemented as structured data and rule synthesis
rather than runtime generative invention of arbitrary game code. The runtime combines predeclared
effect primitives, Domain rules, condition templates and authored Concept interactions.
Asterion/external production can later turn especially important emergent combinations into bespoke
assets without being required for the simulation to function.




                                                  Page 62

<PARSED TEXT FOR PAGE: 63 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

Z. Dynamic Story, Narrative Structure and World Events
This section consolidates the current narrative architecture and the narrative-preference calibration.
Story is authored deeply but must adapt to one persistent simulated history rather than force the world
back onto a predetermined route.

Z.1 Narrative backbone and player agency
   Narrative model is hybrid: high-quality authored arcs exist, but the simulation determines when,
    how and whether they occur and how they resolve.
   The opening can be relatively authored before the world becomes highly systemic.
   A main mission/objective may exist like in a conventional RPG, but the objective is not a mandatory
    canonical outcome. The player may satisfy it, invert it, save the intended target, kill someone
    intended to be saved, support the opposing faction, ignore the problem, or produce another valid
    world-state outcome.
   Major authored arcs can fail, expire, be bypassed or be resolved by other actors rather than waiting
    indefinitely for the player.
   Simulation/history wins when a planned story beat conflicts with established world state. Narrative
    adapts rather than rewinding or overriding history.

Z.2 Persistent history rather than save-branch assumptions
   The game uses one persistent canonical world/history rather than player-facing parallel save slots.
    There is no ordinary save-selection structure for exploring alternate branches. Starting over is an
    explicit destructive Clear Data / new-world action, not a routine save-slot choice.
   Character stories and later events vary according to the actual history that occurred in this persistent
    world.
   Important NPC death, destruction, allegiance changes and territorial outcomes are respected by
    subsequent story logic; scripts do not restore a character merely because an authored scene
    originally expected them.

Z.3 Discovery, quests and consequence
   Stories are commonly discovered through exploration, NPC interaction, geopolitical reports, rumors,
    Codex knowledge, events and world observation rather than only a quest-board interface.
   Quest guidance is knowledge-dependent. Exact waypoints are used only when the
    protagonist/faction actually knows the location; otherwise approximate areas, directions, clues or
    rumor-based information are used.
   Side quests can expire, fail or resolve without the player.
   Ignoring a situation changes the world rather than merely displaying QUEST FAILED. A threatened
    settlement can be conquered, evacuated, destroyed, saved by allies/subordinates or transformed by
    whatever actually happened.
   Subordinates may act in the player's stead of their own volition when their authority, knowledge
    and circumstances make that plausible.

Z.4 Systemic and authored events
   Ordinary generated objectives should arise from real world-state pressures: raids, logistics,
    shortages, wars, territorial needs, monster activity, discoveries, political requests and similar causes.
                                                   Page 63

<PARSED TEXT FOR PAGE: 64 / 143>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Artificial filler such as arbitrary kill-10-wolves tasks is avoided unless the world state gives a real
    reason for that objective.
   The World Director may seed opportunities and initial conditions but does not dictate outcomes.
    Factions, characters and simulation determine what follows.
   Small events occur routinely; major events are rarer and should matter.
   Multiple major events may occur simultaneously, including outside the player's location.
   The player may arrive after an event has ended and encounter only survivors, ruins, reports,
    changed borders, corpses or other consequences.
   Exact historical events are unique, while similar systemic event patterns can recur elsewhere when
    prerequisites recur.
   Small event chains can escalate organically into major crises; high-quality authored chains may also
    branch according to real state.
   Unrelated systems can derail authored events if the world logically changes first.
   Events can occur even when they are far above the player's current ability to solve.

Z.5 Multi-resolution event framework
   One common event architecture supports scales from an individual missing person to local attacks,
    civil wars, dimensional invasions and multiversal catastrophes.
   The framework shares triggers, participants, known information, state transitions, consequences,
    escalation links and historical records, but simulation fidelity changes with scale.
   Individual-scale events may track exact NPCs and routes; wars operate mostly on
    armies/factions/settlements; dimensional crises operate on worlds, Domains, junctions and major
    forces. Nearby/important portions can promote into higher-detail simulation.

Z.6 Narrative depth target
   Narrative target favors long-horizon mysteries, strong recontextualization, layered truths and
    metaphysical questions.
   Multiple competing interpretations are desirable when each follows coherent evidence rather than
    arbitrary ambiguity.
   Unreliable information is strongly desirable when it comes from limited knowledge, manipulation,
    memory, propaganda or interpretation rather than the game lying without rules.
   Character identity questions involving manifestations, memories, alternate versions, corruption,
    reincarnation or transformations are strongly compatible with the setting.
   Antagonists should have substantial motives and internal logic. Personal suffering alone is not
    treated as sufficient motivation for absurdly large goals such as destroying the world unless the
    causal/motivational structure genuinely supports it.
   Conflicts without a morally clean solution are strongly preferred.
   High information density is acceptable and desirable when terminology, metaphysics, factions and
    history have purpose and are supported by Codex/reference tools.
   Slow-burn mysteries and delayed explanation are strongly preferred.
   Apparent genre/cosmology shifts that later recontextualize earlier material are welcome when they
    are genuine revelations rather than arbitrary replacement of the setting.
   Tragic outcomes should remain meaningful even when resurrection exists; recovery does not erase
    consequences.
   Characters may be wrong while still reasoning intelligently from incomplete information.


                                                  Page 64

<PARSED TEXT FOR PAGE: 65 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Later arcs do not need to be simple stronger-villain escalation, but major threats still need enough
    practical danger/complexity to remain challenging relative to prior high-end content.
   The preferred reference direction is closer to the depth/recontextualization/metaphysical complexity
    liked in Fate/Grand Order, Honkai Impact 3rd Part 1, NieR:Automata and Umineko than to simple
    villain-of-the-week escalation.

Z.7 Perspective policy
   The playable narrative perspective remains fundamentally the Ruler/protagonist.
   Other factions and characters can run parallel arcs and become central to the world, but the game
    does not routinely replace the protagonist with temporary playable protagonists for long authored
    arcs.
   Perspective information can still be conveyed through reports, scenes, discovered records,
    communication, observation or limited cinematic framing without changing the player-character
    identity.

Z.8 Rewards from narrative outcomes
   Rewards emerge from what actually happened rather than standardized quest-completion packages.
   A godly Overlord's defeat, for example, should yield consequences appropriate to that entity:
    territory, Domain/Core opportunities, equipment, knowledge, political change, characters, resources
    or other real spoils - not a trivial generic coin payout.
   Some outcomes may have little immediate reward but create highly valuable later opportunities.
   World Director events may surprise the player without advance notice, provided their world-state
    prerequisites and constitution rules are respected.
   Ruler Mode tracks urgent known events, rumors, unresolved crises and resolved history without
    granting omniscient knowledge.




                                                 Page 65

<PARSED TEXT FOR PAGE: 66 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AA. Gameplay Rewards, Exploration Value and Repeatable
Progression
This section defines why the player explores, fights, rules, dispatches characters and participates in
events. Rewards should primarily be consequences of actual world objects, creatures, knowledge,
territory and activities rather than disconnected mobile-game reward boxes.

AA.1 Logical/world-grounded rewards
   Useful rewards primarily originate from the activity/world object that produced them rather than
    abstract completion payouts.
   Enemies can drop the equipment/inventory they actually possess. Important on-screen entities use
    real inventories and body/material composition.
   Creature materials are logically derived from species/body composition and the condition of the
    remains.
   Destructive methods can destroy recoverable loot; precise/clean methods can preserve valuable
    organs, components or equipment.
   Boss rewards primarily come from what the boss possesses/is and from the geopolitical/world
    consequences of defeating it rather than a universal boss token.

AA.2 Abstraction for scalable loot simulation
   Generic loot tables are permitted only as a simulation abstraction where instantiating every object
    would be wasteful: large offscreen battles, low-detail cohorts, mass hunts or ordinary background
    encounters.
   Those tables must still be derived from the actual archetype, equipment pool, local ecology/resources
    and world state. They do not justify unrelated random loot.
   When an entity becomes important or physically instantiated at high fidelity, its concrete
    inventory/body state becomes authoritative.

AA.3 Exploration as a core progression pillar
   Exploration is mandatory/core rather than cosmetic map completion.
   Discovering regions, civilizations, species, phenomena, ruins, Domains and dimensional routes
    grants meaningful knowledge and opportunity.
   Exploration is a major route to alliances, secrets, farmable dungeons, hidden objects, resources and
    unusual progression opportunities.
   Knowledge itself can unlock crafting, enemy data, technologies, skills, Domain synthesis possibilities,
    routes, diplomacy options and other systems.
   First discovery can grant especially strong value while revisiting remains useful through resources,
    evolving world state, dungeons, factions or changing events.

AA.4 Physical caches and world persistence
   Conventional floating treasure-chest spam is avoided. Loot is preferably found as actual caches,
    ruins, vaults, bodies, faction stores, hidden rooms, relic deposits or other sensible physical sources.
   Renewable resources and ordinary populations can regenerate/repopulate logically. Unique
    treasures remain taken unless a genuine mechanism recreates/replaces them.

                                                   Page 66

<PARSED TEXT FOR PAGE: 67 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Species can suffer local or even major depletion/extinction pressure if the world state supports it, but
    content can re-enter through migration, dimensional access, breeding, preservation, resurrection,
    summoning, reintroduction or other valid mechanics rather than silently respawning by schedule.
   Unique canonical beings do not simply respawn on a weekly timer. Recurrence requires a real
    resurrection, species replacement, manifestation or other world rule.

AA.5 Species capture, domestication and military use
   Creatures/species are not limited to being killed for loot. Where lore/mechanics allow, they may be
    captured, enslaved, domesticated, bred, contracted, recruited, summoned or integrated into armies
    and territory systems.
   The exact moral/political consequences are world-dependent rather than represented by a universal
    morality meter.

AA.6 Farming, orders and solved-content automation
   Repeatable farming exists through logical sources such as hunting, resource zones, dungeons,
    expeditions, dimensional phenomena, factions and territory activities.
   Routine farming should not require repeated manual clearing after the player has solved the
    activity.
   Orders/delegation replace skip-ticket-style design: the player assigns characters, subordinates,
    expeditions or territory systems to perform already-understood repeatable work.
   Previous-clear systems can automate/reproduce solved combat configurations where appropriate.
   Automation preserves difficulty for new/unmastered problems rather than turning solved content
    into repetitive labor.

AA.7 No stamina gate; world-time gates instead
   There is no mobile-gacha stamina/energy system restricting how much content the player may play.
   Actual world time can gate construction, recovery, training, expeditions, regeneration and resource
    cycles.
   World-time gates can themselves be altered by legitimate powers, technology, Domain effects, time
    ratios or infrastructure.

AA.8 Risk, reward and non-scaling loot
   Dangerous regions can be entered early. If the player somehow survives/acquires something there,
    they can obtain high-end rewards early.
   Loot does not scale down simply because the protagonist is low level. A legendary weapon found at
    Level 3 remains the same legendary object.
   Rare rewards are primarily rare because their source, prerequisite, state or world condition is rare
    rather than because every ordinary enemy has a tiny arbitrary jackpot chance.
   Very low probability variants can still exist where they represent genuine rare
    mutations/phenotypes/material states - analogous to a shiny encounter in rarity, but potentially
    mechanically/biologically meaningful rather than merely a palette swap.




                                                  Page 67

<PARSED TEXT FOR PAGE: 68 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AA.9 Events, achievements and long-term availability
   Events primarily create new opportunities/content and world situations rather than permanently
    missable mandatory power.
   Important missed characters/equipment should generally have later routes back into circulation
    through reruns, world systems, recovery, rediscovery or equivalent mechanisms.
   Achievements record noteworthy accomplishments/history and can grant titles, specific rewards or
    recognition, but are not mandatory chore tracks.

AA.10 Ruler rewards and influence
   Territorial expansion rewards itself organically through resources, Domain power, infrastructure,
    armies, characters, access, logistics and influence rather than a separate territory-level reward track.
   Offline rankings represent influence/status among simulated rulers, factions and world records
    rather than online players.

AA.11 Challenge content and endless progression
   Explicit game-like challenge modes such as boss towers, survival and escalating combat are allowed
    with an appropriate lore/system wrapper.
   There is no single static endgame. Endgame continuously shifts toward higher-level worlds,
    Domains, Rulers, events, character development and challenges.

AA.12 Catch-up for late acquisitions
   A newly obtained character late in progression should not require manually replaying the entire
    early-game grind.
   Catch-up is provided through already-earned infrastructure and progression knowledge: training
    facilities, mentors, territory resources, stored materials, accelerated low-rank training, previous-
    rank promotion resources, dispatches and other systems the player has legitimately built.
   Catch-up accelerates solved progression bands but does not automatically grant the new character
    high-end unique development, rare skills, Awakening, Evolution branches or experience they have
    not earned.
   This preserves the value of acquiring Character #387 without making every new unit an instant
    clone of the established roster.

AA.13 Feasibility
The reward architecture is feasible because high-fidelity physical loot is reserved for
instantiated/important entities while offscreen cohorts use world-grounded abstraction. Exploration
knowledge and orders are primarily database/state systems. The absence of stamina/skip-ticket
economies simplifies balancing and shifts progression pressure toward world time, logistics, risk and
actual content.




                                                  Page 68

<PARSED TEXT FOR PAGE: 69 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AA. Adult Content Architecture
Adult content is a core pillar of the game, alongside gacha, combat, exploration and the project's broad-
content philosophy. It is not designed as an optional romance minigame or a detachable side gallery. The
mechanical architecture below keeps it integrated with the persistent world, character states and
presentation while restricting explicit sexual content to canonically adult characters.

AA.1 Pillar status and eligibility
   Adult content is a first-class gameplay/content pillar rather than optional side content.
   Explicit sexual content is restricted to characters who are canonically adult in lore/data. Appearance
    alone is never sufficient.
   Adult eligibility is an explicit character-data property and remains separate from rarity, power,
    gender presentation, attractiveness and ordinary interactability.
   The protagonist/Ruler is canonically adult.
   There is no global affection, romance-level or story-completion requirement for adult-content access.
   The Ruler may initiate adult-content opportunities with any canonically adult eligible character
    without needing to grind relationship meters, gifts or romance routes.
   Sexual scene participation itself must be consensual/voluntary; contracts, Ruler Authority and
    supernatural obedience do not substitute for sexual consent.

AA.2 Adult content is not romance
   Adult content and romance are separate systems. A sexual interaction does not imply romance,
    exclusivity, marriage or a permanent relationship state.
   Romance may exist for some characters when appropriate, but it is not the default gateway to sexual
    content.
   Adult content between NPCs is not a required game feature. NPCs can form relationships with each
    other, but explicit adult-content production is centered on the protagonist/Ruler.
   Characters may have relationship states, rivalries, alliances, friendships, loyalties or romances for
    story/world purposes without those states serving as universal sexual-access gates.

AA.3 No affection-meter subsystem
   There is no universal Affection 0-100 grind.
   There is no generic gift-to-affection progression loop.
   Gifts can exist as ordinary contextual objects/events when they make sense, but they are not a
    mandatory adult-content unlock currency.
   Trust, attraction, loyalty, fear, rivalry, respect and similar character-state information may exist
    when useful for simulation/story, but they are not required as a generic meter stack.
   Relationship states can improve or deteriorate through world events and choices.

AA.4 Character-specific consequences
   Adult content does not grant a universal stat bonus.
   Character-specific mechanics may legitimately connect sexual interaction to that character's nature
    or development.



                                                   Page 69

<PARSED TEXT FOR PAGE: 70 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Examples include a succubus gaining power through sex, a saint/angel corruption route influenced
    by sexual experiences, or adult-content events acting as one possible catalyst for Corruption,
    Awakening or Evolution.
   Such consequences are authored/data-driven character mechanics, not a global rule that sex
    increases combat stats.
   Adult-content events can affect persistent biography/history when the event is narratively or
    mechanically significant.

AA.5 Integration with combat and character presentation
   Adult-content integration can appear directly in combat presentation and persistent character state
    when appropriate.
   Clothing is a stateful visual system rather than an invulnerable costume overlay. Damage can tear,
    destroy or remove clothing progressively, including exposing the body on canonically adult
    characters.
   Clothing damage must correspond to actual equipment/outfit state and can persist until
    repaired/replaced when the relevant mode uses persistent damage.
   Character-specific transformations, corruption states, equipment and adult-content-related states
    may alter models, outfits, animations, VFX and battle presentation.
   Combat functionality must not depend on censorship-like visual substitutions in the core design; the
    rendering/content pipeline should be capable of representing the intended adult states directly.

AA.6 Gacha and version integration
   Adult-content capability belongs to Character Identity/Version data and survives normal rarity, rank
    and progression changes.
   Base, Awakened, Corrupted and other Versions of one Character Identity can have different adult-
    content behavior, presentation and scene libraries while remaining grouped under that identity.
   A manifestation acquired through gacha does not lose adult-content availability because the
    corresponding story route was not completed.
   Adult-content content is therefore not gated behind finite authored story progression.

AA.7 Scene/archive architecture
   Previously experienced adult/story scenes can be accessed through an archive/gallery.
   The archive does not impose relationship or story-state restrictions on replay once content is
    available.
   The archive should preserve which Version/Manifestation and visual state were involved where
    relevant.
   Adult-content scenes can also be generated/selected contextually from current character state rather
    than existing only as one-time gallery unlocks.

AA.8 Reproduction and formal partnerships
   There is currently no general pregnancy/reproduction simulator.
   Pregnancy/reproduction may remain available for exceptional species, characters or authored
    mechanics later if it becomes useful, but it is not part of the baseline architecture.
   Marriage/partnership can exist as a lightweight social/political state where useful, but it is not
    required for sexual content and does not become a dating-sim progression ladder.
                                                   Page 70

<PARSED TEXT FOR PAGE: 71 / 143>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AA.9 Technical/content architecture
   Adult scenes, damaged-clothing states, alternate outfits and character-specific erotic states should be
    packaged as modular character content rather than hard-coded into the global combat engine.
   The character data model should support adult eligibility, scene references, contextual tags, visual-
    state requirements, version compatibility and persistent state effects.
   The system should avoid duplicating entire character packages for every clothing-damage or adult
    scene state; use modular meshes/material states, animation sets and scene packages where feasible.
   Premium bespoke adult scenes can coexist with systemic in-world adult states; the former are
    authored content, the latter are runtime character-state presentation.

AA.10 Scope rule
Within the project architecture, adult content is treated as integrated core content rather than optional
or romance-gated. The design remains restricted to canonically adult characters, and sexual
participation is not implemented as forced/non-consensual behavior. Outside that boundary, content
access should not be artificially blocked by affection meters, romance routes or unrelated story
prerequisites.

AA.11 Feasibility
The systemic portion is feasible: adult eligibility, character-state flags, modular clothing damage, scene
selection, version-specific content and persistent consequences are standard data/asset problems. The
major cost is content production: bespoke adult animation, illustration, voice, variant outfits and
transformation states scale with the roster. The architecture therefore needs strong reuse through rig
families, modular outfits, shared animation foundations and package streaming, while reserving fully
bespoke production for important characters/scenes.




                                                  Page 71

<PARSED TEXT FOR PAGE: 72 / 143>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AB. Mature Content Scope - Expanded Definition
The project uses “mature content” as the umbrella term for sexual content involving canonically adult
characters, nudity where appropriate, blood/gore, severe physical violence, injury states, destructible
equipment and clothing damage. This supersedes any earlier wording that could be read as treating
adult content as sex-only.
   Combat presentation is not sanitized by default: blood, visible wounds, broken weapons, damaged
    armor and torn clothing are valid persistent or temporary visual states when the relevant assets and
    performance budget allow them.
   Weapons and armor can physically break, become unusable, require repair or remain historically
    broken; visual presentation should correspond to actual equipment state rather than fake cosmetic
    damage.
   Clothing damage can progressively alter or remove garments in combat. Sexual/nude presentation
    remains restricted to canonically adult characters.
   Mature visual states can be driven by damage type, equipment durability, transformation,
    corruption, environment and character-specific mechanics rather than one generic “damaged
    costume” toggle.
   Violence presentation can vary by species/material: blood, ichor, mechanical fluids, energy
    discharge, fractured armor, severed constructs, etc., according to what the target actually is.
   The project should avoid arbitrary censorship-like substitutions inside its own content pipeline; the
    intended mature presentation should be authored directly where the character/content is eligible.




                                                 Page 72

<PARSED TEXT FOR PAGE: 73 / 143>

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

<PARSED TEXT FOR PAGE: 74 / 143>

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

<PARSED TEXT FOR PAGE: 75 / 143>

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

<PARSED TEXT FOR PAGE: 76 / 143>

                                                  OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AC.11 Feasibility and simulation resolution
The exploration target is feasible only through adaptive simulation fidelity. Nearby important entities
are physically simulated; distant ecology, armies, migrations, gathering and NPC routines are
summarized statistically/event-wise until they become relevant. Environment-specific traversal is
implemented through shared movement/interaction interfaces with medium-specific capability modules
rather than building unrelated engines for ground, sky, underwater and space.




                                                Page 76

<PARSED TEXT FOR PAGE: 77 / 143>

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

<PARSED TEXT FOR PAGE: 78 / 143>

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

<PARSED TEXT FOR PAGE: 79 / 143>

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

<PARSED TEXT FOR PAGE: 80 / 143>

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

<PARSED TEXT FOR PAGE: 81 / 143>

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

<PARSED TEXT FOR PAGE: 82 / 143>

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