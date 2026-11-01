# v0.40 historical master — source extraction 4/10
Source: Offline_Adult_Gacha_RPG_Master_Architecture_Baseline_v0.40_FINAL_PRECODING_FREEZE_RELEASE.docx, 175 pages; uploaded 2026-11-01. Parsed original text lines 2701–3600. Historical source only: newer explicit instructions, verified latest code decisions and v0.41 (missing) may supersede conflicting clauses. Original DOCX remains higher-fidelity for embedded images and precise tables.

   Version creation never deletes earlier forms from the database or Codex.




                                                  Page 52

<PARSED TEXT FOR PAGE: 53 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

T.3 Manifestation defeat and specificity inheritance
   A manifestation that is destroyed, absorbed, invalidated or otherwise removed from active
    ownership is never physically deleted from the database; its state becomes
    historical/inactive/absorbed.
   If a rival manifestation of the same Character Identity is permanently overcome through an
    appropriate mechanic, some or all of its developed specificities may become inheritable by the
    player's corresponding manifestation.
   Potential inheritance targets include learned skills, evolution discoveries, character-specific
    resources, awakening progress or unique developmental traits. Exact transfer rules are mechanic-
    defined rather than automatic full cloning.
   This creates a way for competing manifestations to matter without forcing the player to repull a
    previously acquired character.

T.4 Banner types and character pools
   The game supports permanent standard banners, limited character banners, event/themed banners
    and faction/world-specific banners.
   New summonable characters do not need to be introduced by a finite scripted story. Hidden/sealed
    assets may become summonable through World Director releases, events, newly accessible worlds
    or other systemic context.
   Banners expose contract-accessible Character Identities/Versions rather than requiring those
    characters to have been physically living as ordinary NPCs beforehand.
   All Origin Rarities may appear in banners. Pull probability is based primarily on immutable Origin
    Rarity, not Current Rarity.
   The World Director can schedule banners based on world openings, events, factions, prepared sealed
    content, variety or other valid live-world conditions, but cannot target releases specifically to
    rescue/counter the player's current team.

T.5 Acquisition philosophy
   Pulling a character grants the full character/manifestation; base character acquisition is not
    fragment-gated.
   The economy aims to preserve genuine lucky-pull excitement while preventing the extreme multi-
    month drought/frustration associated with very low-rate no-safety acquisition.
   Hard pity baseline for standard limited character banners is 70 pulls.
   Featured acquisition uses a two-step guarantee model: the first qualifying top result may miss the
    featured target; after a miss, the next qualifying top result is guaranteed to be the featured target.
   Pity and featured-guarantee state carry over within compatible banner categories.
   Soft pity is banner-specific rather than universal.
   Multi-feature and preferred-target/designation banners are supported.
   Ten-pull guarantees are supported (for example, at least SR+ Origin Rarity), preserving visible value
    even when the featured result is not obtained.
   Low-Origin-Rarity characters remain potentially exciting because kits, evolution routes, synergy and
    long-term Current Rarity progression can make them strategically valuable.




                                                  Page 53

<PARSED TEXT FOR PAGE: 54 / 175>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

T.6 Duplicate-equivalent pulls
   A character already owned never disappears from the account merely because another acquisition
    result targets the same identity/version.
   SUPERSEDED: repeat acquisitions now create full persistent Manifestations of the same Character
    Identity so divergent development routes can coexist. Copies are not automatically converted into
    fragments/material.
   Current rule: each repeat Manifestation can independently pursue Awakening, Evolution,
    Corruption, Current Rarity, Rank, equipment, skill and future development routes; later Grand
    Convergence may explicitly fuse fully reinforced divergent Manifestations.
   There is no generic duplicate/fragment dependency in the frozen architecture. Character progression
    routes are achieved through their actual mechanics; repeat Manifestations are full characters rather
    than upgrade tokens.
   Repeat-pull value comes from owning another persistent build/history and potential Grand
    Convergence participation, not automatic material conversion.

T.7 Pity state distinction
   Pity Counter and Featured Guarantee are separate states.
   Pity Counter tracks pulls since the last qualifying high-rarity result and resets when that result
    occurs.
   Featured Guarantee records whether the next qualifying high-rarity result must be the featured
    target because the previous qualifying result missed.
   Both states can carry over between compatible banners. A banner may therefore begin with low pity
    count but an active featured guarantee, or high pity without a guarantee.


U. Gacha Currency and Reward Economy
   Pulling primarily uses one earnable currency; special tickets are additional access items rather than
    the core economy.
   Tickets may include special/event tickets and guaranteed-rarity tickets/pools.
   Pull currency is earned from all major game pillars: exploration, story, bosses, achievements,
    territory, events, rankings, simulated PvP, dispatches, routine activity and World Director rewards.
   Territory can generate pull currency over time. High-level polities such as Overlord-scale domains
    can generate substantial amounts rather than a token trickle.
   Offline accumulation and return/catch-up rewards exist, but active play remains more rewarding
    overall.
   The same pull currency can have finite one-time sources and infinitely repeatable sources. There is
    no permanently finite/limited total supply of pull currency in the game.
   Daily/weekly tasks may exist but should support one-button/automatic completion or claiming
    where appropriate so they do not become tedious chores.
   Acquisition should be easier than fully maximizing a character. Scarcity can therefore concentrate
    more heavily in Awakening, rarity advancement, Transcendence and special development materials
    while preserving non-duplicate routes.




                                                 Page 54

<PARSED TEXT FOR PAGE: 55 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

V. NPC Rulers and Shared Gacha Simulation
   NPC rulers use their own gacha currency/resources, pity state, banner access and strategic
    preferences rather than receiving arbitrary free characters.
   Because summonable identities can have ruler-specific Manifestations, NPC rulers do not
    permanently consume the player's ability to acquire a featured character.
   No invisible NPC pull may permanently steal the player's pity target. Competition for a truly unique
    world-born character or unique contract should occur only through explicit world/event mechanics.
   Important NPC rulers can field their own developed manifestation of a Character Identity. If that
    identity conflicts with the player's version in the same encounter, local identity exclusivity prevents
    simultaneous duplicate deployment.
   NPC manifestations keep their own development history and can become meaningfully different
    from the player's manifestation of the same base identity.


W. Gacha / Gameplay boundary
   Gacha primarily expands the roster and grants access to Character Identities/Versions.
   Gameplay primarily develops the roster through Level, Rank, Current Rarity, skills, equipment,
    Evolution, Awakening, Corruption, Transcendence and world experience.
   Equipment remains outside the normal gacha.
   Forms/evolutions should normally come from gameplay development; gacha can provide separately
    collectible Versions where the lore/design treats them as distinct persistent manifestations.
   Limited characters are eligible for future reruns unless temporarily blocked by a specific world-state
    reason.

W.1 Provisional acquisition-rate recommendation
The emotional target is between guaranteed/fragment-heavy acquisition that produces little excitement
and extremely low-rate acquisition that produces long droughts. Recommended starting point for
simulation testing: full-character acquisition; approximately 1.5% top-Origin-Rarity rate on a standard
limited banner; banner-specific soft pity beginning around 50-55 pulls; hard pity at 70; first qualifying
top result may be off-feature, next is guaranteed; pity and guarantee carry within banner category.
These are tuning defaults, not final fixed numbers.

W.2 Feasibility / risk note
The Manifestation model is technically feasible and is cleaner than a globally unique gacha-character
pool. The main design risk is conceptual clarity: the game must clearly distinguish Character Identity,
Version and ruler-specific Manifestation so players understand why multiple rulers can own 'the same'
character without literal cloning contradictions. The Codex/UI should expose this hierarchy explicitly.

W.3 Unresolved minor points
Login, daily, weekly or return rewards that exist are automatically claimed/credited; there is no manual
daily-login claim chore. Exact base rarity rates, featured split, banner-category taxonomy, soft-pity
curves, ticket types and Transcendence costs remain numerical tuning work. Repeat pulls are full
Manifestations and are not converted through a generic duplicate-resource economy.



                                                  Page 55

<PARSED TEXT FOR PAGE: 56 / 175>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

X. Ruler, Territory, Domain Core and Overlord Authority
This checkpoint defines the current Ruler/Domain framework. It keeps the strategic layer focused on
RPG progression, territorial power, combat, contracts and geopolitics rather than government-simulator
detail.

X.1 Territory before Domain Core
   A newly controlled territory does not automatically possess a Domain Core.
   Territory begins as ordinary controlled land/space/realm under a Ruler or proto-Ruler structure.
   Once the territory reaches the required Rank/power/development conditions, it can crystallize or
    awaken a Domain Core.
   The Domain Core is therefore a progression milestone rather than the thing that creates the territory
    in the first place.
   Unlocking a Domain Core grants new territory-scale advantages and higher-order Ruler mechanics.

X.2 Main territory and affiliates
   A Ruler may control several territories simultaneously.
   One territory is designated the Main Territory; additional holdings behave as affiliated/subordinate
    territories rather than perfectly equal capitals.
   At higher scales, especially Overlord scale, holdings can span worlds and dimensions.
   Territories can remain linked through political authority, contract networks, dimensional
    infrastructure or subordinate-Ruler appointment.

X.3 Domain individuality
   Each mature Domain can develop its own Domain abilities and rule set rather than every Ruler
    receiving the same powers.
   Domain abilities may derive from world type, territory history, population/species, Ruler
    development, conquered cores, structures, resources and special events.
   Domain powers can affect combat, travel, summoning/contracts, infrastructure, resurrection,
    defense, teleportation and higher-order territory mechanics.

X.4 Conquest and Domain Core outcomes
   When defeating another Domain, several outcomes can exist depending on mechanics and intent:
    conquer and preserve, destroy, wipe/neutralize, subordinate, or absorb/fuse the defeated Domain
    Core.
   Core fusion can transfer or transform Domain-specific mechanics rather than simply add flat stats.
   A destroyed or absorbed Domain Core remains part of history/state and is not erased from the
    database.
   Core fusion should carry compatibility, stability and capacity requirements so repeated conquest is
    not a free additive snowball.

X.5 Overlord structure and appointed Rulers
   Becoming an Overlord allows the player to appoint selected important roster characters as
    subordinate Rulers by granting them Ruler Authority.

                                                 Page 56

<PARSED TEXT FOR PAGE: 57 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Subordinate Rulers govern affiliated territories and remain distinct Ruler entities rather than
    ordinary governors.
   Subordinate Rulers may wage wars independently where their granted authority permits it, but the
    Main Territory can impose permission rules or strategic restrictions.
   The Overlord can directly command characters contracted by subordinate Rulers, reflecting the
    higher contract/authority hierarchy.
   Subordinate Rulers can possess their own contract/gacha access. They may pull using their own
    resources and resources allocated/given by the superior Ruler. Under the player's Overlord
    hierarchy, acquisitions made by subordinate Rulers ultimately belong to the wider player-controlled
    roster ecosystem, while exact accounting/assignment rules remain an implementation/economy
    detail.

X.6 Ruler Rank
   Ruler progression is not a universal fixed ladder. The system evaluates the Ruler's actual sovereignty
    state across personal power, Authority, territory control/quality/scale, Domain/Core state,
    contracts/roster, subordinate Rulers, geopolitical reach and world/dimensional scope.
   There are only two universal sovereignty titles: Ruler and Overlord. Overlord is the highest universal
    title and requires both sufficient personal/Authority capability and genuinely Overlord-scale
    sovereignty. Subordinate Rulers are one valid route to that scale, not a mandatory requirement; one
    sufficiently vast world/dimension-spanning sovereignty structure can also qualify. Other titles are
    cultural, political, religious, military, story or historical rather than higher universal sovereignty
    tiers.
   Milestone recognition can be gained, lost or changed when the underlying state changes. Titles
    describe real achieved structure rather than granting hidden catch-up power or forcing every Ruler
    through identical steps.
   The player-facing Ruler page should show the concrete sovereignty facts and meaningful milestone
    titles, not a misleading single linear rank number. Internal summary metrics may exist for
    AI/comparison/debugging but never replace the underlying state.

X.7 Domain Authority and territorial advantage
   Domain Authority is strongest within owned territory and weakens outside it.
   Two Domains may overlap and contest control.
   Domain powers can directly affect battle inside owned territory.
   Rulers can construct permanent Domain-scale structures and systems such as barriers, teleport
    infrastructure, resurrection facilities, contract/summoning facilities and other territory-dependent
    assets.
   At sufficiently high levels a Domain may establish broad territorial rules of reality, provided the
    Ruler actually possesses the required Authority and infrastructure.
   A Domain may suppress, modify or constrain hostile abilities inside its area when its rules/Authority
    justify doing so.
   A stronger opposing Ruler/Domain may contest or override those rules through explicit Authority
    resolution rather than automatic immunity.




                                                 Page 57

<PARSED TEXT FOR PAGE: 58 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

X.8 Ascension and old territories
   Ascending to a higher dimension does not sever previous territory from the player's
    historical/political structure.
   The player may appoint a subordinate Ruler or successor to manage the old territory while retaining
    links through hierarchy, contracts, alliances or Domain networks.
   The old territory continues evolving independently under its appointed ruler rather than freezing
    when the player leaves.
   Residual strategic benefits depend on the maintained relationship/link; they are not automatically
    lost merely because the protagonist moved to a higher dimension.

X.9 Difficulty hardening principles
   No hidden anti-player stat scaling or rubber-banding is used. Difficulty comes from explicit world
    rules, stronger entities, finite strategic attention, attrition and the consequences of overextension.
   Domain Core vulnerability: a mature Domain gains strong advantages but also gains a strategic
    heart/anchor that can be attacked, sealed, corrupted, isolated or destabilized.
   Administrative bandwidth: an Overlord may own many territories, but direct high-fidelity
    intervention everywhere at once is impossible. Remote holdings can rely on subordinate Rulers,
    autonomous systems, prepared defenses, infrastructure, standing orders or other world-valid
    arrangements rather than requiring delegation in every case.
   Authority bandwidth: directly commanding subordinate-Ruler contracts and projecting Domain
    powers across many territories consumes limited Authority/energy/capacity rather than being
    globally free.
   Cross-dimensional logistics remain expensive. Reinforcement, teleport infrastructure, high-rank
    materials and Domain projection across dimensions require real infrastructure and resources.
   Core fusion has risk: incompatible cores can destabilize, mutate, overload, corrupt or reduce control
    instead of being unconditional permanent buffs.
   Power attracts geopolitical response through observable behavior, not secret scaling. Neighbors can
    form alliances, prepare counters, fortify or strike exposed holdings when a growing empire becomes
    a credible threat.
   Higher Rank unlocks access to stronger worlds/junctions and therefore stronger opponents.
    Progression expands the danger ceiling instead of only making old content easier.
   Territorial defense is asymmetric: the Ruler cannot personally protect every affiliate. Enemies may
    attack multiple fronts, exploit travel time, sabotage infrastructure or force difficult prioritization.
   Failure is persistent: lost territory, destroyed infrastructure, incapacitated characters, broken
    equipment and dead/reviving characters create recovery problems rather than instant reset.
   Ruler Commands and Domain powers are powerful but conditional on location, Authority,
    infrastructure, charge/cost and counter-Authority; they are not unrestricted 'win buttons'.
   Subordinate Rulers are assets and liabilities: strong autonomy enables expansion but can create
    wars, consume resources or make strategically poor decisions within the authority they were
    granted.
   High-end enemies can target systems rather than HP alone: contracts, Domain Core, resurrection
    anchors, teleport links, reserve formations, equipment, Authority layers and supply lines.
   Some threats can be functionally unwinnable at the current stage. Survival, delay, evacuation,
    sacrifice, diplomacy or abandoning territory can be valid outcomes.
   The player is never guaranteed that every simultaneously threatened territory can be saved.

                                                  Page 58

<PARSED TEXT FOR PAGE: 59 / 175>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

X.10 Feasibility
The Ruler/Domain framework is feasible because most Domain power and geopolitical effects are
data/state systems rather than continuous physical simulation. The main implementation risks are rule
interaction and multi-front strategic state, both manageable through hierarchical simulation and explicit
Authority resolution. Difficulty hardening is achieved by costs, exposure, multi-front commitments and
stronger reachable content rather than expensive AI or hidden scaling.




                                                 Page 59

<PARSED TEXT FOR PAGE: 60 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

Y. Contracts, Domain Power, Core Evolution and Concept
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

<PARSED TEXT FOR PAGE: 61 / 175>

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

<PARSED TEXT FOR PAGE: 62 / 175>

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

<PARSED TEXT FOR PAGE: 63 / 175>

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

<PARSED TEXT FOR PAGE: 64 / 175>

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

<PARSED TEXT FOR PAGE: 65 / 175>

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

<PARSED TEXT FOR PAGE: 66 / 175>

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

<PARSED TEXT FOR PAGE: 67 / 175>

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

<PARSED TEXT FOR PAGE: 68 / 175>

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
    early-game grind; however, acquisition itself remains meaningful enough to preserve gacha
    excitement and does not become trivial.
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

<PARSED TEXT FOR PAGE: 69 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AA. Adult Content Architecture
Adult content is a core pillar of the game, alongside gacha, combat, exploration and the project's broad-
content philosophy. It is not designed as an optional romance minigame or a detachable side gallery. The
mechanical architecture below keeps it integrated with the persistent world, character states and
presentation while restricting explicit sexual content to canonically adult characters.

AA.1 Pillar status and eligibility
   Adult content is a first-class gameplay/content pillar rather than optional side content.
   Explicit sexual content is restricted to characters validated by canonical game data as adult/mature
    characters. Appearance alone does not determine eligibility, while characters canonically treated as
    minors/children remain excluded from sexual-content packages.
   Adult eligibility is an explicit character-data property and remains separate from rarity, power,
    gender presentation, attractiveness and ordinary interactability.
   The protagonist/Ruler is canonically adult.
   There is no global affection, romance-level or story-completion requirement for adult-content access.
   The Ruler may initiate adult-content opportunities with any character whose canonical lore defines
    them as adult/mature without needing relationship-meter, gift, romance-route or appearance-based
    permission checks.
   SUPERSEDED TERMINOLOGY: adult scenes use character-specific sexual-participation eligibility and
    scene-validity state rather than a universal consent meter or mandatory explicit verbal-consent
    prompt. Tsundere/role-play presentation does not by itself invalidate a scene when the underlying
    adult character state represents voluntary participation. Ruler Authority/contracts are not automatic
    sexual-access keys.

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




                                                   Page 69

<PARSED TEXT FOR PAGE: 70 / 175>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AA.4 Character-specific consequences
   Adult content does not grant a universal stat bonus.
   Character-specific mechanics may legitimately connect sexual interaction to that character's nature
    or development.
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
   Canonical adulthood belongs to Character Identity lore and survives normal rarity, Rank and
    progression changes. Version/form data may alter adult-content presentation, profile variants,
    anatomy and scene assets, but cannot add a second generic permission gate for a lore-adult Identity.
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



                                                 Page 70

<PARSED TEXT FOR PAGE: 71 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

AA.8 Reproduction and formal partnerships
   There is currently no general pregnancy/reproduction simulator.
   Pregnancy/reproduction may remain available for exceptional species, characters or authored
    mechanics later if it becomes useful, but it is not part of the baseline architecture.
   Marriage/partnership can exist as a lightweight social/political state where useful, but it is not
    required for sexual content and does not become a dating-sim progression ladder.

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
Within the project architecture, adult content is integrated core content rather than optional or
romance-gated. Canonical lore is the sole maturity authority: if a Character Identity is lore-adult/mature,
the adult-content pillar is available by default. Appearance never determines adulthood. Personality,
preferences, libido, history, injury, anatomy and current state shape what is expressed and physically
possible rather than acting as a generic access lock.

AA.11 Feasibility
The systemic portion is feasible: adult eligibility, character-state flags, modular clothing damage, scene
selection, version-specific content and persistent consequences are standard data/asset problems. The
major cost is content production: bespoke adult animation, illustration, voice, variant outfits and
transformation states scale with the roster. The architecture therefore needs strong reuse through rig
families, modular outfits, shared animation foundations and package streaming, while reserving fully
bespoke production for important characters/scenes.




                                                   Page 71

<PARSED TEXT FOR PAGE: 72 / 175>

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