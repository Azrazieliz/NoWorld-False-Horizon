# Native owned inventory and equipment

`Content/NoWorld/Equipment/EquipmentCatalog.json` is staged as UFS and read offline.
It is intentionally empty until approved equipment content is authored. Stored
items remain browsable; the UI does not grant items or assume humanoid slots.

The required root fields are `schema` (`noworld.equipment.v1`), `slots`, `items`
and `rules`. Unknown fields, duplicate JSON keys and ambiguous rules fail closed.

- Each slot requires exact `identity_id`, `version_id`, `slot_id`, `display_name`.
- Each item display entry requires `definition_id`, `display_name`. This names
  existing instances; it does not create or grant an instance.
- Each compatibility rule requires exact `identity_id`, `version_id`, `slot_id`,
  `definition_id`, `rank_id`, `quality_id` and an explicit `stat_bonuses` object.
  An empty rank/quality string matches only an empty recorded rank/quality.
  There are no wildcard slots, versions, item categories or rarity rules.
- Bonus values contain a nonnegative decimal-string `significand` and an int32
  `exponent10`. The supported additive fields are `max_hp`, `attack`, `defense`
  and integral int32 `speed`. Unsupported mechanics and modified instances need
  their own approved resolver. An empty bonuses object declares no additive
  contribution; it does not imply unstated skills or effects.

Equip rejects broken/invalid stored condition, unapproved combinations, foreign
ownership, and items already bound elsewhere. It never silently moves a physical
item from another copy. Replacement must be reviewed against the actual current
slot. Unequip removes an existing owned binding and retains the item and history;
it does not require inventing a new compatibility rule for a legacy binding.

The canonical command recomputes its quote under the existing WorldStore
transaction and appends an `equipment_changed` receipt with the request ID.
Ownership, condition, slot contents and catalog changes invalidate a pending
review. Repeating the identical committed request acknowledges its receipt.

Runtime order is fresh content/form stats, native catalog equipment, then earned
development. `FCharacterCombatResolver` supplies the fresh base; world-party
refresh and both Core battle initializers project the latter two layers. Cached
equipment/development inputs are read-only projections, not saved equipment truth.
An unchanged refresh cannot stack bonuses, heal or clip current/overflow HP.
Definitions outside this catalog remain with their existing equipment resolver;
their absence never authorizes a new native equip action.

The session rejects commands during a Core-owned running battle, a diagnostic
turn battle, or an active world action. A committed command remains successful
if subsequent party/UI refresh fails; the UI reports that warning separately.

The new Unreal automation tests are under `OfflineGame.Inventory`. They require
the project's native test runner and were added without invoking a native build.
