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
