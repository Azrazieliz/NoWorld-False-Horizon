# P0 Weapon Motion ? Production Input Blockers (2026-10-18)

Correction: This game supports fists, unusual anatomy, slingshots, bows, thrown items, swords, polearms, spellcasting, firearms and futuristic devices, without a mandatory weapon type or single gun-holding pose.

## Actual Gate12 changes

World HUD attack icon is now equipment-neutral, rather than a fixed sword. Runtime accepts a user/authored motion-profile token in a real equipped item's existing EvolutionStateJson under the optional key combat_motion_profile (read only, no save migration). It does not infer weapon type from names or require a fixed enum. Authorable examples: sling, fist, spellcraft, bow, sword, energy_cannon. Animation keys: CombatAttack.<profile>.<1-3>, CombatSkill.<profile>.<1-3>. Only rig-compatible existing authored clips play; unsupported styles never become fake firearm poses. Existing combat damage, targeting and progression remain unchanged. Native profile tests cover missing, malformed, diverse, conflicting and broken-equipped-item cases.

## Exact dependencies for other Gates

Gate 07 (Items): For each approved usable item, deliver definition ID, actual permitted body slots/anatomies, authored motion-profile identifier, compatible equipment combinations and source metadata. If EvolutionStateJson is not the authoritative home for a purely visual binding, provide the exact approved alternative; Gate12 must adapt once, rather than invent a competing model. No fake inventory.

Gate 09 (Animation and SFX): For each approved profile AND skeleton, provide actual authored attack clip/montage, idle/walk/run/swim/fly carrying posture, draw/release or cast/strike animation, grip and socket placement, interruption/recovery and contact marker. Prioritize visibly different unarmed, sling, spell, firearm and tech motions. The animation packages and actual skeleton compatibility are missing; a routing table is NOT finished animations.

Gate 04 (Combat): Provide per-kit delivery/range/targeting behavior, physical projectile trajectories, magic area/LOS, collision/hit markers, damage and cancel windows inside both frozen executors. Current diagnostic fallback of fixed range and damage must not be used as universal combat.

Gate 10 (UI): Supply distinct authored weapon/skill icons and interaction presentation matching the approved World HUD. Until actual equipment bindings exist the action icon stays neutral.

Gate 11 (Registry): Accept and register actual authored assets and identity/ownership as needed. Do not synthesize records.

Gate 12: Integrate real approved equipment/profile updates and gameplay animation, evaluate visible result per skeleton, build Android and test the connected Galaxy S26 Ultra. No production animation, 3D weapon, projectile, audio or mobile acceptance is claimed by this code-only bridge.

## Further implementation: innate style and Ultimate dispatch (source-only, build withheld)

The existing equipment-profile resolver also accepts the optional 'combat_motion_profile' string in the real canonical actor's EntityStateJson, for specifically authored **natural weapon, bare-handed, supernatural, magic and nonhumanoid techniques**. Character state is used only when no equipped item supplies a profile. Conflicting usable equipped-item profiles still fail closed; never replace with innate style. A single usable equipment profile takes precedence. No synthetic equipment, canonical ID or schema migration is created.

Canonical characters without an explicitly authored character/equipment profile no longer trigger the legacy generic humanoid attack/recoil: correct animation is deferred until a compatible authored clip exists. Explicit noncanonical diagnostic fixture actions may retain old diagnostic clips.

Clip families: CombatAttack.<profile>.<1-3>, CombatSkill.<profile>.<1-3>, and distinct CombatUltimate.<profile>. Matching optional Gate09 authored SOUND events: gameplay.attack.<profile>, gameplay.skill.<profile>, gameplay.ultimate.<profile>; only played if the compatible action clip is actually bound. No generic rifle/pistol sound and no disconnected attack audio. Gate09 supplies skeleton-compatible clips, grip/holster, release/contact timing, recovery, transition, distinct locomotion per style. Gate04 supplies authored action/kit-specific delivery, collision/target/LOS, projectile and magic rules. Gate07 signs off the canonical source for the optional item and innate presentation profiles; JSON remains an interim read-only bridge, not a Registry/save migration.

This refinement is NOT COMPILED, NATIVE TESTED, OR VISUALLY ACCEPTED YET by the current user instruction to defer builds until feasible implementation is complete.
