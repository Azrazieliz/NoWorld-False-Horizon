# v0.40 historical master — source extraction 6/10
Source: Offline_Adult_Gacha_RPG_Master_Architecture_Baseline_v0.40_FINAL_PRECODING_FREEZE_RELEASE.docx, 175 pages; uploaded 2026-11-01. Parsed original text lines 4501–5400. Historical source only: newer explicit instructions, verified latest code decisions and v0.41 (missing) may supersede conflicting clauses. Original DOCX remains higher-fidelity for embedded images and precise tables.


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

<PARSED TEXT FOR PAGE: 94 / 175>

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

<PARSED TEXT FOR PAGE: 95 / 175>

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

<PARSED TEXT FOR PAGE: 96 / 175>

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

<PARSED TEXT FOR PAGE: 97 / 175>

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

<PARSED TEXT FOR PAGE: 98 / 175>

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

<PARSED TEXT FOR PAGE: 99 / 175>

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

<PARSED TEXT FOR PAGE: 100 / 175>

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

<PARSED TEXT FOR PAGE: 101 / 175>

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

<PARSED TEXT FOR PAGE: 102 / 175>

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

<PARSED TEXT FOR PAGE: 103 / 175>

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

<PARSED TEXT FOR PAGE: 104 / 175>

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

<PARSED TEXT FOR PAGE: 105 / 175>

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

<PARSED TEXT FOR PAGE: 106 / 175>

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

<PARSED TEXT FOR PAGE: 107 / 175>

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

<PARSED TEXT FOR PAGE: 108 / 175>

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

<PARSED TEXT FOR PAGE: 109 / 175>

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
   SUPERSEDED BY AV: Crown is the non-unique apex of ordinary mastery. Grand is not merely a
    higher mastery tier; it is the unique metaphysical Class office/seat, with at most one active Grand
    bearer per canonical Class across the connected dimensional defensive order.
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

<PARSED TEXT FOR PAGE: 110 / 175>

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

<PARSED TEXT FOR PAGE: 111 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   The Creative Director may modify any reported decision. New explicit direction supersedes the
    delegated design choice and must be propagated through the cumulative baseline and
    implementation documentation.
   Plot twists, hidden identities and future narrative answers remain spoiler-protected unless the
    Creative Director explicitly asks to inspect them; large questions may still be asked about tone, limits
    or desired experience.

AK.8 Former open choices - resolved
   RESOLVED by checkpoint AK: one irreversible protagonist Transcendence whose permanent grade
    equals the highest qualification satisfied at breakthrough.
   RESOLVED by checkpoint AK: intrinsic personal World Manifestation and territorial Domain
    Manifestation/Authority coexist as independent higher-order systems.
   RESOLVED by checkpoint AL: Ruler progression is organic and state-based, with major milestone
    titles rather than a universal numbered/name ladder.




                                                  Page 111

<PARSED TEXT FOR PAGE: 112 / 175>

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
   Each grade requires sufficient power plus meaningful persistent-world personal conditions/feats
    rather than one universal checklist. There are no alternate ordinary save branches.

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

<PARSED TEXT FOR PAGE: 113 / 175>

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
   Overlord is the highest universal sovereignty title and reflects both sufficient personal/Authority
    capability and genuine Overlord-scale sovereignty. Subordinate Rulers are one possible structural
    expression but are not universally required.
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