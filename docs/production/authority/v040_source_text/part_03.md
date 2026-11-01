# v0.40 historical master — source extraction 3/10
Source: Offline_Adult_Gacha_RPG_Master_Architecture_Baseline_v0.40_FINAL_PRECODING_FREEZE_RELEASE.docx, 175 pages; uploaded 2026-11-01. Parsed original text lines 1801–2700. Historical source only: newer explicit instructions, verified latest code decisions and v0.41 authoritative narrow addendum `docs/MATURE_CONTENT_V041_PRODUCTION_FREEZE.md` and later explicit user decisions may supersede conflicting clauses. Original DOCX remains higher-fidelity for embedded images and precise tables.

    tetration mathematics; a compact significand/exponent or equivalent large-number type is
    appropriate.
   End-of-battle recap should emphasize analytical totals and contribution: total damage, per-character
    damage share, healing, shielding, damage prevented/taken, crit performance, DoT, follow-up,
    ultimate, reserve and combo contributions where meaningful.

H.11 Auto combat and reproducibility
   Auto mode is required for deep 18-character teams. It should support tactical priorities and
    conditional rules rather than merely random skill use.
   Auto behavior may later learn from the player's style, but deterministic rules/presets remain the
    reliable/debuggable foundation.
   Turn-battle speed target currently includes 1x and 2x. Ultimate/cinematic presentation is
    configurable independently (full, shortened, first-time-only, skip in auto, etc.).
   Battle RNG must be deterministically seeded so crits, dodges, procs and other probabilistic
    interactions can be reproduced for debugging.

H.12 Codex and discovery
   Enemy information can be incomplete on first encounter and become more complete through
    observation, repeated encounters, research or other knowledge mechanics.
   The Codex/Album can record characters, environments, mobs, powers, effects and discovered
    combat information.

Combat feasibility guard
   Current verdict: AMBER-GREEN. The central challenge is software/system complexity, not impossible
    mobile rendering.
   The effect/rule engine must be data-driven and strongly typed in C++/reviewable data. Character-
    specific behavior should compose reusable primitives rather than accumulate ad-hoc Blueprint
    exceptions.
   Only six strategic characters are active at once in turn battles, and World Mode uses a three-
    character immediate party centered on the Ruler, keeping active rendering/simulation bounded.
   The current combat design does not require a tactical grid, 18 simultaneously rendered fighters, or a
    full summon army subsystem.




                                                 Page 34

<PARSED TEXT FOR PAGE: 35 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

I-P. Turn-Combat Rules Checkpoint (v0.5)
STATUS: CURRENT AUTHORITATIVE BASELINE — This section records the accepted detailed turn-
combat rules from the 261-420 questionnaire. Exact numeric tuning and formulas remain open where
noted.

I. Skill selection and action economy
   Turn selection is HSR-like in presentation: each acting character exposes a basic attack plus a
    variable set of active skills, with the Ultimate handled separately when its activation rules permit it.
   Almost every character has a free/basic attack. Basic attacks may themselves transform, chain, inflict
    effects or otherwise become mechanically complex.
   Selectable active-skill count is character-dependent; approximately two to three is common, but the
    architecture allows broader variation within the previously agreed overall kit-density target.
   There is no universal team Skill Point resource. Character actions use independent cooldowns,
    charges, HP costs, conditions, personal resources or other explicit mechanics.
   Cooldowns and charged skills are both supported. Once-per-battle restrictions are not a normal
    balancing crutch; they are used only when a specific kit concept warrants them.
   Conditional skills may appear/become usable only when their activation requirements are met.
   Skills can evolve dynamically during battle, but fusing/altering permanent skills belongs primarily to
    character development outside combat. Character development must preserve the earlier rule that
    characters do not simply accumulate duplicate copies of the same skill; evolution paths can be
    numerous/undefined.
   Actions do not universally cost exactly one action. Some moves can consume future actions/turns,
    require charge time, or otherwise alter action economy.
   Free actions are a normal strategic mechanic, but are usually delivered by passive/triggered effects
    or specific skills rather than an unrestricted generic command.
   Counters, interceptions and other off-turn reactions are supported but remain explicit kit mechanics
    rather than universal real-time behavior for every unit.
   On a player turn, the timeline waits for input. Enemy intent is knowledge-dependent: exact
    information is shown only when the player/party has sufficient knowledge, observation, abilities or
    intelligence.

J. Formation and battlefield presentation
   Standard full team: 18 characters organized as three waves of six. Only the current six are
    active/visible combatants. If Wave 1 wins cleanly, Waves 2 and 3 may never participate.
   Implementation choice: use six direct succession lanes (A1→B1→C1, etc.). This offers meaningful
    team-building without a complex free-form reserve scheduler.
   When an active unit is defeated, replacement occurs after the current action resolves. The incoming
    unit normally enters the timeline without an immediate action unless its entry/passive rule says
    otherwise.
   All 18 can be reordered before battle and saved as reusable formation presets. Formation setup
    should show all 18 portraits simultaneously.
   During battle, show the six active characters plus their immediate successors by default; deeper
    reserve information is expandable.
   Reserve kits can be fully inspected while battle input is paused.


                                                  Page 35

<PARSED TEXT FOR PAGE: 36 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Voluntary replacement has no universal cost rule; cost/permission is mode- or mechanic-defined. A
    voluntarily withdrawn character can return to reserve rather than being permanently consumed.
   Free-form reserve-chain shuffling is not a core requirement. Keep the interface simple unless a
    specific skill manipulates formation.
   Skills may manipulate formation, pull reserves forward/back, target reserves, temporarily deploy a
    reserve, or skip normal succession rules.
   Special attacks may target one enemy, adjacent/multiple enemies, all six active enemies, active plus
    reserve members, or exceptionally the full 18, according to ability definition.
   The six active characters are presented statically in an HSR-like battle staging; they animate/move
    for skills and effects rather than occupying a tactical grid.

K. Combat stats
   Core numerical stats: HP, ATK, DEF, SPD. SPD drives timeline/action frequency.
   Special stats are primarily percentage-based: Crit Rate, Crit Damage, Crit Rate Resistance, Crit
    Damage Resistance, Dodge, Hit, Block, DEF Penetration, Resistance Penetration, Damage Dealt,
    Damage Taken Reduction, effect-related statistics and other build-specific modifiers.
   Normal attacks hit by default, but Dodge can avoid them and Hit counters Dodge.
   Block is separate from Dodge and reduces a successful hit. Parry is primarily an active-combat or
    explicit-skill mechanic rather than a generic turn-combat stat.
   Penetration stats are percentage-based rather than split into flat and percentage versions by default.
   Generic Damage Dealt and Damage Taken modifiers can reach extreme values when kit conditions
    justify them (e.g. enormous bonuses against a specific afflicted state).
   Debuff application uses application chance/accuracy versus resistance. Hard CC is not given a
    separate universal control-resistance stat; it is represented through debuff/affliction rules and levels.
   Healing is not a universal stat family. Sustain is primarily regeneration per turn and
    vampirism/regeneration after attacks. Shield strength is an effect property/buff, not a universal stat.
   Vampirism/lifesteal is a universal possible effect/stat concept but is normally granted through race,
    skill or build rather than present on everyone.
   Counter and follow-up behavior come from explicit skills/passives, not universal Counter Rate or
    Follow-Up Rate stats.
   Status-specific and element-specific modifiers are supported, such as Burn Damage, Freeze
    Resistance, Fire Damage, duration modifiers, etc.
   A visible GP/Power number summarizes overall build strength for comparison only. It never directly
    determines combat outcome, and complex kits can make a lower-GP character outperform a higher-
    GP straightforward one.

L. Damage formula behavior
   Typical attacks start from ATK × skill multiplier, while abilities may instead scale from HP, DEF,
    missing HP, target stats or other defined values.
   DEF uses ratio/progressive reduction behavior rather than simple subtraction, supporting very large
    stat ranges more safely.
   Higher Rank/Existence entities are stronger primarily because they naturally possess superior stats,
    resistances and mechanics, not because of an arbitrary level-difference penalty.
   Rank Suppression exists for extreme metaphysical gaps and may suppress damage/effects beyond
    ordinary stats. Specific abilities can ignore it, though raw stat disparity may still make damage
    negligible.
                                                   Page 36

<PARSED TEXT FOR PAGE: 37 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Absent changes in stats, buffs/debuffs, affinities, crit/dodge/proc RNG and other state, identical
    attacks should resolve on the same scale rather than having arbitrary random damage variance.
   Critical hits are probabilistic, with Crit Rate/target resistance affecting chance and Crit Damage
    defining the multiplier. Conventional base multipliers are acceptable as a starting point and will be
    tuned later.
   True Damage ignores DEF, ordinary resistances and normal damage reduction, but does not
    automatically ignore explicit True Damage Resistance (TDR) or higher-order rule effects.
   Percentage-of-Max-HP offensive damage is not a normal direct-attack damage formula. It is allowed
    through explicit afflictions, debuffs and other specifically authored special mechanics. Defensive HP-
    loss caps such as 'maximum X% Max HP per hit/turn' remain valid mechanics. Execute/missing-HP
    scaling, target-stat scaling and damage-type conversion remain supported.
   Execute/missing-HP scaling, target-stat scaling and damage-type conversion are supported.
   Overkill can increase reward/charge and may feed specific mechanics.
   Multi-hit crit/dodge/status rolls are ability-defined: some attacks roll each hit separately, others
    resolve as one attack sequence.
   Battle recap should emphasize damage by Basic, Active Skills, Ultimate and DoT rather than an
    excessively fragmented taxonomy.

M. Effect architecture
   Effects use meaningful lineages: e.g. Burn → Ignite → Calcination, with derivative branches such as
    Dark Ignition. Evolution may add new mechanics, not merely higher potency.
   Effect upgrades can occur through reapplication, specific skills, thresholds or other effect-defined
    triggers.
   Evolved/derived effects can retain family membership so generic Burn interactions still recognize
    Ignite/Dark Ignition while specialized rules can target the derivative specifically.
   Primary effect dimensions are stacks and remaining turns. Potency is encoded by explicit
    named/leveled variants such as Burn I/II/III, Ignite I/II/III rather than a separate generic potency
    scalar.
   Not every effect needs every dimension. Duration decay is effect-defined rather than universally tied
    to source turn, target turn or global round.
   Permanent-until-resolved and normally unremovable effects are supported. Sufficiently high-
    authority cleansing can explicitly override normal unremovability.
   Buffs may also evolve through lineages; exact naming will be designed later.
   Effects can cancel, transform or combine with opposing effects/environmental states. Battlefield
    conditions such as oil, water, fire and special fields can participate.
   DoTs/statuses remember their source for damage credit, rewards and triggered interactions.
    Transfer, theft, reflection, copying, spread, detonation and duration isolation/freeze are supported.
   Debuffs/afflictions can have levels so application/resistance is more granular than simple
    immune/not-immune.
   Effect Authority is a distinct layer above ordinary skill potency. Normal R/SR/SSR skills generally
    operate without Authority; higher progression classes such as UR/LR may introduce Authority. At MR
    Current Rarity, every character can unlock World Fantasm regardless of Origin Rarity. World
    Fantasm is a personal reality-field ability whose grade is determined by immutable Origin Rarity; its
    current grading is specified in the v0.21 World Fantasm checkpoint below. Authority can itself be
    modified, but ordinary non-authority buffs cannot casually affect it unless explicitly permitted.


                                                  Page 37

<PARSED TEXT FOR PAGE: 38 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Authority can itself be modified, but ordinary non-authority buffs cannot casually affect it unless
    explicitly permitted.
   UI groups effects broadly (Buff, Debuff, Control, DoT, Sustain, Special, Rule) while preserving their
    specific identities.
   Element charts may vary by world/system. The common effect engine links equivalent derived states
    without forcing all worlds into one elemental taxonomy.

N. Ultimate charging
   Ultimate mechanics are character-specific rather than forcing every character into one identical
    Ultimate/gauge model.
   Default structure is closer to Fate-style NP charging: attacks/skills/passives generate per-character
    charge; charge capacity/threshold can vary by character and kit.
   Starting charge may be modified by builds, equipment, passives and formation effects.
   There is no universal team energy resource. Each skill/action explicitly defines charge gain, and kits
    can modify or replace the usual pattern.
   Overgauge is supported. Excess energy above the activation threshold remains stored as reserve
    rather than necessarily being wasted.
   Some characters can materially evolve their Ultimate at higher charge thresholds, producing
    genuinely different Ultimate levels/forms rather than merely a numerical multiplier.
   Most Ultimates may be activated outside normal turn order. Enemies can use equivalent mechanics.
   Multiple allied Ultimates can queue in selected order and may produce chain/combo effects. Specific
    characters can unlock fused Ultimates.
   Ultimates can be sealed, delayed, drained or interrupted where mechanics allow; some can become
    uninterruptible once activated.
   On defeat/revival, retained Ultimate charge is effect-defined.

O. Progression-unlocked skills
   Characters may be acquired with only part of their eventual kit.
   Major kit growth sources currently include Rank, Rarity, Evolution, Awakening and Corruption.
    Rank and Rarity are distinct. Evolution does not inherently change Rank or Rarity. Awakening is a
    major character rework/refonte rather than a simple upgrade node.
   Normal leveling mostly increases numbers. Major progression milestones unlock or alter mechanics.
   Ordinary skills do not use conventional skill levels. Higher-order systems such as Authorities may
    have their own progression levels where appropriate.
   Passive mechanics can unlock at progression thresholds/ranks.
   Awakening can replace/rebuild old skills; replaced versions are not normally selectable afterward.
   Character development can branch. Different instances/states of the same base concept may evolve
    differently, but paths are not hard exclusive locks: skills can later be fused, altered or further
    evolved.
   There is no normal respec system for these organic developmental paths.
   Corruption can be an in-battle transformation, permanent/alternate developmental state or separate
    roster version depending on lore, with permanent/alternate states being common.
   Base, Evolution, Corruption, Corrupted Evolution and other combinations are supported when lore
    permits.
   Higher rarity frequently unlocks whole subsystems unavailable to lower rarity at first, but rarity
    itself can be increased and low-rarity characters can eventually regain strategic relevance.
                                                  Page 38

<PARSED TEXT FOR PAGE: 39 / 175>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Old characters are preferably strengthened through new progression/evolution/awakening/reworks
    rather than only flat number buffs.
   Kit complexity may grow with development: early versions can be easier to understand while
    evolved forms become substantially denser.

P. Eighteen-character team construction
   18 is the hard maximum for the standard high-end turn team. Earlier/lighter content can use fewer
    slots.
   All 18 contribute to displayed team GP even though only six begin active.
   There is no deployment-cost restriction: if the player legitimately owns 18 extremely powerful
    characters, they may field all 18.
   No duplicate of the exact same canonical version/entity can be fielded. Alternate versions may
    coexist only when lore says they can simultaneously exist; otherwise they are mutually exclusive
    representations of one canonical entity.
   Both generic faction-threshold bonuses and character-specific/faction-specific synergy conditions are
    valid. Mixed-faction teams remain fully viable.
   One team-wide Leader/Captain slot exists; there is not a separate captain for each wave.
   The protagonist/Ruler normally commands outside the 18-character team rather than consuming a
    slot.
   The Ruler can issue explicit Ruler Commands during turn battles, including tactical field attacks and
    other authority-based actions. Ruler Commands do not use one mandatory universal command-point
    economy. They become available through explicit conditions, encounter rules, battle types,
    preparation or other ability-specific requirements; some fights may not permit them at all.
   High-end preparation can include: choose 18, order three waves/direct successor lanes,
    equipment/builds, auto-tactical rules, leader selection and synergy/faction planning.
   Cleared-content persistence should work Dokkan-like in spirit: the previous successful clear/team
    configuration is stored as the default baseline so routine replay does not require rebuilding the
    entire setup.
   Different waves can intentionally serve different purposes (burst/setup, sustain/counter,
    emergency/endurance).
   Entry/death effects are valid kit mechanics but are not mandatory universal team-building systems.
    Specific predecessor-successor identity combos are not a baseline mechanic.
   Skills can skip normal waves, temporarily enter from reserve and return, or otherwise manipulate
    deployment when explicitly defined.
   Reserve units are normally protected from ordinary attacks, but special mechanics can target or
    disable reserves. A boss may exceptionally wipe/disable an entire reserve wave.
   Victory occurs when the opposing active side and all available reinforcements/roster members
    required by the encounter are defeated.
   Ruler-relevant enemy forces may use the same 18-character structure. Random mobs and ordinary
    unaffiliated encounters do not need it.
   Standard encounters do not exceed 18, but exceptional set-piece battles can temporarily expose a full
    larger roster/reinforcement structure, analogous in spirit to rare large-roster survival/set-piece
    encounters rather than the everyday UI.
   Large multi-front encounters should use several independent 18-character teams rather than one
    36/54-character battlefield.


                                                 Page 39

<PARSED TEXT FOR PAGE: 40 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Across consecutive dungeon/war battles, defeated/injured characters remain unavailable until
    healed/recovered. Large roster depth therefore has direct strategic value.

P.1 Resolved interpretation notes
   Question 315 (Follow-Up Rate): resolved the same way as Counter Rate — follow-ups come from
    explicit kit mechanics/passives, not a generic universal proc stat.
   Question 362 (standard ultimate generation): resolved as a Fate-like per-character charge model with
    explicit charge values on attacks/skills and character-specific overrides, not a universal team energy
    economy.
   Question 395 (alternate versions): coexistence is lore-gated; exact duplicate entities are never
    allowed.
   The answer numbered 512 in sequence is interpreted as the answer to Question 412: yes, special
    bosses may wipe/disable an entire reserve wave.

Feasibility checkpoint
   Current detailed turn-combat design remains AMBER-GREEN. Complexity is concentrated in the
    data-driven effect/rule engine, AI/auto tactics, battle UI and content authoring rather than rendering
    18 simultaneous actors.
   Only six strategic characters are active at once. Direct succession lanes and static HSR-like staging
    keep runtime/UI complexity bounded.
   No universal summon-heavy turn layer, tactical-grid simulation or per-character bespoke hard-
    coded logic is required. Exceptional mechanics must compose reusable rules/primitives.




                                                  Page 40

<PARSED TEXT FOR PAGE: 41 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

Q. Turn-Combat Refinement Checkpoint II
This checkpoint incorporates the decisions from questions 421-465 and supersedes any earlier
conflicting combat notes. The design remains a six-active / eighteen-character succession-based gacha
RPG battle system.

Q.1 Skill selection and action readiness
   Turn UI is broadly Basic + selectable Skills + Ultimate, but character presentation is more authored
    than a plain generic button row. Unavailable abilities remain visible/greyed and may use character-
    specific feedback (for example, a voice/text line explaining that sufficient power has not yet been
    accumulated).
   Basic attacks are not filler actions. They may be central to optimal rotations and can become the
    strongest action in a particular setup when combined with buffs, prior skills, states or synergies.
   Characters have a variable number of selectable active skills. Basic attacks themselves may be
    complex and may transform or gain secondary effects.
   There is no universal shared team Skill Point resource.
   Ordinary turn-count cooldowns are NOT frozen as the default gating model. A skill instead exposes
    an explicit Readiness Requirement. Readiness may be controlled by charge, states, resources, prior
    actions, conditions, limited charges, preparation time, or (where appropriate) an actual cooldown.
   Skill charges and recharge rules are skill-defined rather than universal.
   Once-per-battle restrictions are not a normal balancing tool; encounters should be capable of
    surviving powerful kits without broadly nerfing skill reuse.
   HP-cost skills and conditionally available skills are supported.
   Skills can evolve dynamically during combat when their state/conditions change. Permanent
    fusion/alteration of skills belongs primarily to character development rather than ad-hoc in-battle
    crafting.
   Actions do not universally cost exactly one action. Skills may consume multiple actions, future
    actions, charge time, or other resources.
   Free actions are a normal kit possibility, but are generally implemented through passive/triggered
    mechanics or explicitly free actives rather than as a universal action economy.
   Reactive/counter/interruption skills are supported but remain exceptional rather than turning all
    turn combat into continuous real-time input.
   Player decision time pauses the active decision flow.
   Enemy intent visibility is knowledge-dependent: exact information may require observation, Codex
    knowledge, abilities or other intelligence.

Q.2 Passive terminology
   A passive is never manually activated. It is either continuously active or automatically triggered by a
    defined event/condition.
   Static, triggered, reactive, entry, defeat, reserve and aura passives remain valid categories.
   Counters and follow-up attacks are generated by explicit skill/passive mechanics, not by a universal
    Counter Rate or Follow-Up Rate stat.




                                                  Page 41

<PARSED TEXT FOR PAGE: 42 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

Q.3 Formation and reserve behavior
   Standard high-end formation remains exactly eighteen characters: six active, six second-line
    reserves, six third-line reserves.
   The baseline succession model uses direct lanes: A1→B1→C1 through A6→B6→C6.
   When an active character is defeated, the current action resolves before the replacement enters.
   Replacement units enter the timeline normally unless an entry skill/passive changes that behavior.
   All eighteen can be reordered before battle and full formation presets can be saved.
   Combat HUD shows the active six plus their immediate successors by default; the full reserve roster
    remains inspectable.
   Reserve buffs/debuffs and ordinary durations freeze while the character is in reserve unless an
    effect explicitly says otherwise.
   Reserve-targeting effects exist but are mildly rare; ordinary attacks do not casually invalidate the
    reserve structure.
   There is no free universal voluntary reserve switch. Normal succession occurs through defeat/death-
    state replacement. Mid-battle reserve changes occur only when an explicit skill, Ruler Command,
    encounter mechanic or other authored rule permits them.
   If a retreat-to-reserve mechanic is used, current HP, Ultimate charge, buffs and debuffs are
    preserved unless the mechanic explicitly modifies them.
   Formation-manipulation skills may swap units, pull reserves forward, send units backward, attack
    reserves or skip succession stages.
   AoE targeting may range from one target to all six active units and, for special mechanics, extend
    into reserves or the full eighteen.
   Active battlefield presentation is HSR-like/static between actions: characters hold cinematic positions
    and move primarily when skills/animations execute, not on a tactical grid.

Q.4 Core statistics
   Foundational stats include HP, ATK, DEF and SPD.
   Special stats are predominantly percentage/probability based: Crit Rate, Crit Damage, Crit Rate
    Resistance, Crit Damage Resistance, Dodge, Hit, Block Rate, Block Reduction, penetration, Damage
    Dealt modifiers, Damage Taken modifiers, effect application/resistance, regeneration, vampirism and
    effect-specific modifiers.
   Attacks normally hit; Dodge creates a probability to evade and Hit opposes Dodge.
   Turn-based parry is not a generic stat. Parry remains an active-combat or explicit character
    mechanic.
   DEF penetration and resistance penetration are percentage-based; extreme penetration can exceed
    100% and become effective defense negation/negative-defense pressure where rules permit.
   Generic Damage Dealt and Damage Taken modifiers are supported, including extremely large values.
   No generic Healing Done/Received stat is used. Direct restoration can exist as a skill effect; persistent
    sustain statistics are Regeneration and Vampirism.
   Shield strength is an effect/property rather than a universal character stat.
   Status-specific and system-specific modifiers are supported (for example Burn damage, Freeze
    resistance, Fire damage), without requiring a universal element system across all worlds.
   GP/Power is a visible summary metric only. It never determines combat outcomes directly and can
    misrepresent unusual or highly synergistic kits.



                                                   Page 42

<PARSED TEXT FOR PAGE: 43 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

Q.5 Crit, Hit and overflow
   Crit Rate is probabilistic and resolved through seeded RNG after opposing Crit Resistance is applied.
   After Crit Resistance is deducted, Crit chance may exceed 100%. Every attack is then guaranteed to
    crit; remaining overflow is converted into Crit Damage.
   Current baseline conversion: after Crit Resistance is deducted, every 1 percentage point of Crit Rate
    above 100% converts to +2 percentage points of Crit Damage. Example: 140% effective Crit Rate =
    guaranteed critical hit plus +80% Crit Damage from overflow.
   Hit versus Dodge uses the same overflow philosophy, but excess Hit converts into additional eligible
    hit instances rather than Crit Damage. Full 100% overflow bands can create guaranteed extra hit
    replication, with any remainder providing probability for an additional replication. A skill may
    explicitly prohibit replication when its fiction/mechanics require a single indivisible strike.
   Block uses two values: Block Rate (probability) and Block Reduction (damage reduction when a block
    succeeds).

Q.6 Damage formula behavior
   Typical attacks begin from ATK × skill multiplier, while individual skills may scale from HP, DEF,
    target stats, missing HP or other quantities.
   DEF uses ratio/progressive mitigation rather than simple subtraction.
   Power differences arise primarily from actual stats, resistances and mechanics rather than an
    arbitrary level penalty. A separate Rank Suppression layer may represent metaphysical/existence
    gaps and can itself be bypassed by appropriate abilities.
   Identical attacks should remain within the same damage scale, but exact outcomes naturally vary
    through stats, buffs, debuffs, affinities, RNG, criticals and other mechanics.
   True Damage ignores DEF, ordinary resistances, ordinary Damage Reduction and Block unless an
    effect explicitly states that it protects against True Damage. True Damage Resistance remains capable
    of reducing it.
   Direct offensive fixed-% Max-HP damage is NOT a normal attack mechanic. Percentage-HP damage
    belongs mainly to explicit afflictions/debuffs/special rules. Defensive caps such as 'cannot lose more
    than 4% Max HP per hit/turn' are fully supported.
   Execute/missing-HP scaling, target-stat scaling and damage-type conversion are supported.
   Overkill can contribute to additional charge/reward mechanics when a skill/system defines it.
   Multi-hit crit/dodge/status resolution is ability-defined.
   Battle recap groups damage primarily by Basic, Active Skills, Ultimate and DoT rather than many
    micro-categories.

Q.7 Percentage modifier composition
   Separate modifier categories multiply each other.
   Within a category, the preferred design is also multiplicative when effects are authored as
    multipliers. Example: 200% × 300% = 600% of the reference value.
   To avoid ambiguous '+200%' notation, implementation/UI should distinguish explicit multipliers
    (×2.00, ×3.00) from additive percentage-point bonuses. This preserves the intended multiplicative
    stacking while keeping formulas debuggable.
   Character kits may still define additive bonuses explicitly where desired.




                                                 Page 43

<PARSED TEXT FOR PAGE: 44 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

Q.8 Regeneration, restoration and excess HP
   Direct healing/restoration skills are valid (for example 'restore 30% HP'). What is excluded is a
    generic universal Healing stat.
   Baseline Regeneration should scale from Max HP, not remaining HP or missing HP. Example: 5%
    Regeneration restores 5% of Max HP at its trigger. This is the recommended universal meaning
    because it is stable, readable and does not inherently snowball at high HP or become
    disproportionately strong only at low HP.
   Specific skills may instead explicitly scale from missing HP, remaining/current HP, marked HP,
    damage dealt or another quantity.
   Default Regeneration triggers on the affected character's turn unless the effect defines another
    timing.
   Ordinary restoration caps at normal Max HP. Overhealing does not universally create extra
    durability.
   Specific characters/effects may convert excess restoration into Barrier/Shield or into Overflow HP /
    additional HP bars. A character with extreme sustain can therefore temporarily reach values such as
    300% of nominal HP when their kit explicitly permits it.

Q.9 Effect architecture and authored variants
   Effect lineages such as Burn → Ignite → Calcination are valid, but progression may add mechanics
    rather than only increase magnitude.
   An authored variant is not automatically decomposed into generic component debuffs. Frozen Flame
    or Dark Flame can remain atomic named effects with their own behavior.
   A variant may optionally expose compatibility/inheritance relationships to broader families (for
    example 'counts as Burn-family for this synergy') without literally becoming a stack of generic Burn
    + Freeze/Darkness entries.
   If two sources apply the exact same effect instance/type and level (for example Burn II), their stacks
    can combine according to that effect's stacking rule while preserving source ownership internally.
   Different authored flame variants do not automatically stack merely because both are visually fire-
    like; stacking/compatibility is explicitly defined.
   A target may simultaneously carry base, evolved and derivative effects where the relevant effect
    definitions permit it.
   Effects primarily use Stack Count + Turns. Potency is represented by authored grades/names such as
    Burn I, Burn II, Ignite I rather than a universal separate potency number.
   Stack limits, duration decrement rules, permanence, cleansing, transfer, reflection, copying, spread,
    detonation and duration freezing are all effect-defined.
   Cleanse skills state exactly what they can remove, including family, category, count, level and/or
    Authority constraints.
   Authority is separate from ordinary effect grade/level and is used for higher-order rule conflicts.
    Authority can be modified only by effects capable of operating at the relevant Authority layer;
    ordinary low-tier buffs cannot casually alter it.
   UI may group effects broadly as Buff, Debuff, Control, DoT, Sustain, Special and Rule while
    preserving the precise authored effect underneath.
   Elemental charts can vary by world/system.




                                                 Page 44

<PARSED TEXT FOR PAGE: 45 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

Q.10 Rarity, Authority and post-MR growth
   R/SR/SSR/UR/LR/MR refer to Rarity, not Rank. Rank and Rarity remain separate systems.
   Authority access is rarity-locked according to the progression design: ordinary lower rarities do not
    naturally gain the higher-order system before reaching the required rarity.
   Because every character can eventually raise Rarity to MR, rarity-locked higher-order mechanics
    remain eventually accessible to the whole roster.
   To avoid base-MR characters hitting a dead ceiling, MR should remain the highest conventional
    Rarity rather than endlessly adding new banner rarities. Once a character is MR, further ceiling
    growth moves into a separate universal post-MR progression layer (working name: Transcendence).
   Base-MR characters can enter Transcendence immediately once its requirements are met; lower-
    rarity characters first climb to MR and then access the same post-MR system. This keeps base-MR
    characters from being structurally disadvantaged while preserving MR as a meaningful top rarity.
   MR Current Rarity unlocks World Fantasm for every character regardless of Origin Rarity. A
    character does not climb sequentially through World Projection -> World Alteration -> World
    Materialization -> World Manifestation -> World Negation. Instead, immutable Origin Rarity
    determines the World Fantasm grade obtained at MR. Moving from MR into Transcendence can
    strengthen or expand that Fantasm without automatically changing its grade.

Q.11 Ultimate / NP-like charge
   Ultimates use character-specific mechanics and charge behavior rather than one universal team
    energy economy.
   UI normalizes each character's own base Ultimate threshold as 100%, regardless of the underlying
    internal value.
   Fate-like NP charge is the conceptual reference: attacks/skills/passives can grant explicit charge
    amounts and builds can affect generation.
   Every character's base Ultimate becomes available at 100% of that character's threshold.
   Maximum overgauge is character-dependent and may be influenced by a Mana/Energy-Capacity-like
    stat.
   Overgauge remains stored energy. It does not automatically create a second independent Ultimate
    resource.
   Evolved Ultimate tiers can be materially different at higher charge thresholds (for example 100% /
    200% / 300%).
   The player may deliberately choose a lower Ultimate tier even when a higher one is available; the
    selected tier consumes its own cost and preserves the remainder.
   Most Ultimates may be used outside the character's ordinary turn; enemies can do the same where
    their kits permit it.
   Multiple queued Ultimates can execute in selected order and may create chain/combo effects.
   Fused Ultimates, sealing, drain, delay, interruption and uninterruptible activation states are
    supported.
   Ultimate charge after defeat/revival is effect-defined.

Q.12 Progression-unlocked skills
   Characters can begin with only part of their eventual kit.
   Primary mechanical unlock paths are Rarity, Rank, Evolution, Awakening and Corruption. Rarity
    and Rank are distinct.

                                                 Page 45

<PARSED TEXT FOR PAGE: 46 / 175>

                                                   OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Normal leveling primarily raises numbers. Major progression milestones add/alter mechanics.
   Ordinary skills do not use conventional numeric skill levels. Higher-order mechanics such as
    Authorities may use their own level/grade system.
   Passive abilities can unlock at progression milestones.
   Awakening is the major character rework/refonte mechanism and may replace old skills entirely;
    replaced versions are not normally selectable afterward.
   Character development can branch into an undefined number of evolution paths over time. Paths do
    not have to permanently lock each other; skills can later fuse, alter or create new developments.
   There is no general respec system for character-development history.
   Corruption may be an in-battle transformation, developmental state or separate roster identity
    depending on lore, most commonly the latter two.
   Base, Evolution, Corruption and combinations such as Corrupted Evolution may coexist when lore
    permits.
   Top rarity characters may contain entire mechanics absent from lower rarities, but lower-rarity
    characters can eventually increase rarity and remain strategically relevant.
   Old-character modernization prefers Awakening/new progression/evolution content over simple
    numeric buffs.
   Kit complexity can grow substantially as the character develops.

Q.13 Team leadership, Ruler Commands and replay
   Leader/Captain effects apply across the full eighteen-character formation, including reserves, unless
    the effect explicitly says otherwise.
   Leader effects may impose team-building requirements.
   The protagonist/Ruler normally commands outside the eighteen-character strategic roster.
   Exactly five Ruler Command slots are equipped for a standard battle.
   Ruler Commands have ability-specific recharge/cost rules and may be extremely powerful if the
    protagonist legitimately possesses that capability (field attack, resurrection, forced deployment,
    timeline manipulation, Domain effects, etc.).
   Previous-clear persistence stores the entire successful setup: formation, successor lanes,
    equipment/build preset references, Leader, tactics and Ruler Commands.
   Replay uses the current state of referenced builds rather than an immutable historical stat snapshot,
    but a configuration/build must earn its own manual-clear validation before being treated as a
    trusted auto-clear baseline.
   Auto tactics support both team-wide logic and per-character rules (for example, hold Ultimate until
    200%, prioritize Skill 2 against Burn-family targets).

Q.14 Feasibility note
This refinement remains feasible. The complexity is concentrated in deterministic battle-state
evaluation, the authored effect/rule graph, large-number-safe arithmetic, UI/inspection tooling and
content authoring. No new requirement implies rendering eighteen simultaneous high-detail actors. The
largest engineering risks are combinatorial effect interactions and automated regression coverage, both
of which are manageable only if the combat engine is declarative/data-driven and every battle remains
seed-reproducible.




                                                 Page 46

<PARSED TEXT FOR PAGE: 47 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

R. Character Ontology, Generation and Long-Term
Development
This checkpoint freezes the character rules answered after the combat specification. It preserves the
distinction between authored/gacha-quality characters, generated world NPCs, promotion of emergent
NPCs, progression through Rank/Rarity/Evolution/Awakening/Corruption, and persistent character
history.

R.1 Canonical character identity
   Important recruitable/playable characters are unique canonical individuals rather than disposable
    procedural copies.
   Any sufficiently relevant ordinary generated NPC may eventually be promoted into a full
    persistent/playable character. Promotion is not limited to a pre-authored elite caste.
   Every important character has a persistent biography/history record. Unimportant background NPCs
    may remain lightweight until promoted.
   Previous forms, evolutions, awakenings, corruptions and alternate states are preserved under the
    same canonical character record rather than overwriting history.
   Exact duplicate instances of one canonical version do not coexist. Alternate versions coexist only
    when the lore says they can.

R.2 Rarity, Rank and Level
   Every character stores immutable Origin Rarity and mutable Current Rarity.
   Current Rarity can eventually rise to MR for every character, subject to progression requirements.
   Rank is separate from Rarity and represents actual advancement/power stage.
   Rank can decrease through seals, injuries, regression, corruption or other valid mechanics; it is not
    strictly monotonic.
   Each Rank contains a repeating conventional level band of 1-100. At level 100, the character must
    satisfy the next Rank promotion requirements before continuing.
   Transcendence is the single universal rarity/state immediately after MR for every character. It is not
    a ladder that one character climbs through several Transcendence states. Its grade/title is
    determined by immutable Origin Rarity using the five-grade Ascendant / Unbound / Exalted /
    Transcendent / Absolute scale defined in the v0.21 checkpoint below.

R.3 Experience and autonomous growth
   Characters can gain progression from combat, dispatches, training and world activity, with different
    activity types awarding different values.
   Characters left in territory continue to train, work, socialize, defend, participate in events and
    progress without requiring manual field use.
   Characters owned by NPC Rulers can likewise grow/evolve before the player ever recruits them.
   Recruiting a previously simulated character preserves the character as actually developed rather
    than replacing them with a standardized banner-reset template.
   Rare/special reset items may reset or reopen a character's skill-development tree after recruitment
    when the player wants to rebuild a naturally developed character.



                                                  Page 47

<PARSED TEXT FOR PAGE: 48 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

R.4 Equipment
   The game supports a normal common equipment structure plus character/species-specific special
    slot layouts for entities that cannot use standard equipment.
   Equipment must have visible aesthetic consequences whenever physically meaningful; it is not
    merely a hidden stat sheet.
   Equipment itself can possess Rarity, Rank and Evolution.
   Equipment combines deterministic identity with limited variation: a given weapon model has fixed
    normal/base stats, while equipment states may carry approximately one to three additional
    attributes/modifiers.
   Equipment may become a named historical object through use, ownership and character evolution.
    A long-used weapon can become an important unique item and remain associated with a character
    across major development.

R.5 Skill learning and integration
   Characters may permanently learn skills beyond their initial authored kit.
   There is no small universal cap on how many skills a character may have learned over its lifetime. A
    character might accumulate hundreds or thousands of learned techniques.
   Learned abilities are not exposed as thousands of active buttons. The character retains a compact
    active combat kit while learned skills are progressively integrated, fused, altered or expressed
    through later kit updates/evolutions.
   Skill compatibility is character-dependent rather than a fixed global skill-slot maximum.
   Only characters with appropriate traits/abilities (for example copy, genius, mimicry or equivalent)
    can directly learn from other characters in the manner their mechanic permits.
   Characters can develop completely original abilities from their own experiences rather than only
    selecting from pre-authored acquisition lists.
   Skill-development provenance should be retained so the game can explain which learned abilities,
    experiences and evolutions contributed to a current fused skill.

R.6 Availability, betrayal and death
   Important owned characters are not intended to be permanently deleted from the player's universe.
   Betrayal and disobedience depend on contract strength/Ruler Rank and can be prevented by
    sufficiently strong binding.
   Capture, betrayal, incapacitation, dimensional separation and similar states may make a character
    temporarily unavailable.
   Canonical death remains reversible under valid world mechanics and conditions rather than
    deleting the character record.
   Death/resurrection can itself become a prerequisite or catalyst for corruption/evolution when
    appropriate.

R.7 Procedural/promoted character production
   Unreal Runtime is responsible for deterministic assembly and representation, not for inventing a
    production-quality character asset from nothing.
   Generated NPCs are created from seeded modular libraries: species/body rig family, body/face
    parameters, hair, materials, outfit components, accessories, voice/personality pools, faction/culture,
    traits, history and rule-generated initial abilities.
                                                  Page 48

<PARSED TEXT FOR PAGE: 49 / 175>

                                                     OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   The seed and chosen components become persistent once the NPC matters, ensuring the same
    individual never visually rerolls on reload.
   Promotion freezes the NPC as a named individual and expands its persistent data, biography, combat
    definition, development graph, portrait/codex representation and higher-fidelity presentation using
    the same underlying identity.
   All promotable NPCs should be assembled from sufficiently capable rig/body families so becoming
    playable does not require replacing them with an unrelated model.
   Truly bespoke assets (unique meshes, hairstyles, signature outfits, animations, premium ultimates,
    etc.) come from the external content-production/Asterion pipeline or prebuilt modular libraries; they
    are not expected to be synthesized from scratch on the phone at runtime.
   Promotion must therefore have an immediate modular playable tier and may later receive richer
    bespoke packages without changing canonical identity.

R.8 Relationships - provisional simplification
   A conventional gift-grind 'Affection meter' is not currently justified as a major standalone system.
   A lightweight Relationship/Bond state remains useful for important characters because it supports
    story reactions, contextual adult-content expression, character-specific events, NPC-to-NPC bonds,
    and explicit combat synergies/fused attacks. It is not a universal adult-content access gate.
   Relationship state should be event-driven and limited to important characters rather than
    continuously simulating the emotional life of every NPC.
   Relationship progression should not be a mandatory source of generic combat power. Specific
    character relationships may unlock bespoke synergies or evolution/story events when authored to
    do so.
   Generic relationship dimensions/meters are superseded by the hard-simplification pass. Important
    relationship facts are represented only through authored/event-driven tags, history,
    consent/eligibility facts, specific bonds and explicit synergies where content requires them.

R.9 Feasibility
The character target is feasible if procedural generation means deterministic assembly from curated
modular asset families plus rule-driven identity/kit generation. It is not feasible to expect Unreal on the
S26 Ultra to autonomously model, texture, rig, animate, voice and art-direct an unlimited number of
bespoke premium characters at runtime. The scalable architecture is therefore: modular runtime NPC
generation -> persistent promotion -> data-driven skill integration -> optional external bespoke-content
upgrade.




                                                   Page 49

<PARSED TEXT FOR PAGE: 50 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

S. Equipment, Forging and Historical Items
This checkpoint freezes the equipment philosophy. Equipment is a physical/aesthetic part of the
character, not merely a stat package, and can accumulate history, affinity, skills and identity over time.

S.1 Slot structure and compatibility
   Equipment layout is character-dependent. There is a light common structure for ordinary
    humanoids, while unusual species/characters may use different or special slots.
   Weapon/equipment compatibility is individual rather than one global class restriction. A character
    may use any item their physiology, training, traits and mechanics actually allow.
   Some characters are incompatible with conventional equipment and use bespoke slot schemas
    instead.

S.2 Proficiency and GP/Rank interaction
   Repeated use of weapons/equipment can improve character proficiency.
   Weapon mastery and equipment quality can contribute to GP/Power and, where appropriate, to
    Rank advancement requirements or effective advancement.
   Equipment is therefore part of a character's lived development rather than an interchangeable stat
    stick.

S.3 Equipment-granted skills and bidirectional learning
   Equipment may grant complete abilities/skills, not only stats or passive bonuses.
   Long-term use can allow a character to internalize, derive or fuse an equipment-origin skill into
    their own developing kit where compatibility permits.
   The reverse direction is also supported: a character's long-term powers, experiences and evolution
    can imprint onto, alter or teach an equipped item new abilities.
   This bidirectional character-item development preserves provenance so the game can record how a
    named technique or weapon property emerged.

S.4 Equipment growth and forging
   Equipment can possess Rarity, Rank and Evolution.
   Evolution triggers are item-dependent and may include materials, forging, owner Rank, owner
    evolution, combat history, kills, important events, absorption/fusion, awakening or other authored
    conditions.
   A forge/crafting system exists for creation, repair, reforging, upgrading and other item-development
    operations.
   Equipment can break or become unusable. Breakage changes state; it does not delete the item's
    canonical record.

S.5 Historical persistence and named objects
   Any sufficiently important ordinary item can systemically become a named historical object through
    long-term ownership, events, evolution or cultural recognition.
   A weapon kept by the same character across major evolution may itself evolve into a named
    signature object.

                                                  Page 50

<PARSED TEXT FOR PAGE: 51 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

   Important designed entities are never silently deleted from the database. Destroyed, broken, lost,
    sealed, stolen or inaccessible items remain part of world history and can potentially be recovered,
    repaired or reforged if mechanics permit.
   Unique historical items retain ownership history and provenance across transfers.

S.6 Affinity and transfer
   Equipment can develop affinity with a long-term owner and perform better, unlock functions or
    evolve specifically through that relationship.
   When transferred, the item's history and developed traits remain, but owner-specific affinity is
    reduced or must be rebuilt unless the item has a mechanic that says otherwise.
   This allows legendary equipment to remain historically significant even after changing hands.

S.7 Fixed identity plus limited variation
   The same ordinary weapon model has deterministic fixed/base stats.
   Depending on its source, an item state may carry approximately one to three additional variable
    attributes/modifiers.
   Variable attributes can originate at drop, forging, evolution or another item-specific creation step.
   Attributes may be rerolled with controlled materials/mechanics rather than requiring endless
    uncontrolled artifact farming.
   Equipment should generally emphasize understandable identity and deliberate buildcraft over large
    numbers of randomized substats.

S.8 Synergy and sets
   Equipment synergy should primarily emerge from the actual identities/mechanics of items.
   Conventional multi-piece set bonuses are allowed only where they make sense in lore/design rather
    than being the default itemization structure.

S.9 Equipment acquisition and gacha boundary
   Equipment is not a normal gacha category.
   There are no standard weapon/equipment banners.
   Equipment is acquired through gameplay: world exploration, enemies, bosses, forging, events,
    factions, characters, territory systems and other in-world sources.
   Character gacha remains the primary gacha system. Signature equipment can still exist, but
    obtaining it is a gameplay/content problem rather than a separate monetization-style pull economy.

S.10 Feasibility
The equipment target is feasible. The main scalable requirement is modular visible equipment
attachment plus data-driven item states. Historical persistence, affinity and item-skill provenance are
database/state complexity rather than rendering complexity. The design deliberately avoids a high-
volume randomized artifact system, reducing both balancing and UI burden.




                                                  Page 51

<PARSED TEXT FOR PAGE: 52 / 175>

                                                    OFFLINE ADULT GACHA RPG - MASTER ARCHITECTURE BASELINE

0. Baseline Governance - Current Authoritative, Not Immutable
All decisions in this document are current authoritative defaults rather than permanently immutable
rules. The newest explicit user decision supersedes older conflicting decisions. When a rule is revised,
dependent systems must be reconciled rather than preserving contradictions. Working names,
numerical values and implementation details remain replaceable until the whole design is frozen for
production.


T. Gacha Character Ontology and Acquisition
T.1 Character Identity versus Manifestation
   A gacha character is defined by one canonical Character Identity record containing origin, design,
    lore, Origin Rarity, version relationships and shared identity metadata.
   A character that has not yet been manifested does not need to exist as a fully simulated world NPC. It
    may remain sealed/hidden content until a banner, event, NPC ruler or other valid acquisition
    mechanism manifests it.
   Each ruler may obtain its own contracted Manifestation of the same Character Identity.
    Manifestations can develop independently through Rank, Level, skills, equipment, evolution history
    and world experiences.
   This replaces the earlier assumption that one gacha identity can belong physically to only one ruler
    in the entire simulation.
   Local identity exclusivity remains: two manifestations/versions of the same Character Identity do not
    fight simultaneously in the same encounter. If an enemy deploys its manifestation, the player's
    manifestation remains unavailable/off-field for that encounter.
   NPC rulers therefore use the same broad gacha system without permanently stealing characters
    from the player or exhausting a global unique-character pool.
   World-born/promoted NPCs may still be truly unique individuals. The manifestation model
    primarily solves summonable/gacha identities.

T.2 Versions, Awakening, Corruption and Branch Forking
   Base, Awakened, Corrupted, Evolved and other versions belong under one Character Identity.
   When a new distinct version is created, the previous/base version becomes available again as its own
    summonable/roster version rather than being permanently consumed.
   The player can eventually own several versions of the same Character Identity simultaneously in the
    roster.
   Only one version/manifestation of a Character Identity may participate in a given local fight unless a
    deliberately exceptional lore mechanic overrides the restriction.
   Natural progression should still remain inside the character when it is truly the same continuing
    state; a separate roster Version is created when the design/lore treats the state as independently
    collectible or persistently distinct.