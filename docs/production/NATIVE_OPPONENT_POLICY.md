# Authored native opponent policies

The existing World non-player turn runner remains in place. A real opponent body
may add `UOGNativeTurnAgentComponent` with a `UOGNativeTurnPolicy` to replace its
generic ability rotation with authored decisions. The legacy runner yields to
that component and requires a real bound body even for its existing fallback.
This adds no encounter, NPC, mesh, stats, ordinary lineage or damage algorithm.

The actual encounter factory must first create/load the original canonical NPC,
World presence, approved native body and kit through the existing game systems,
then bind `UOGCanonicalCombatActorBindingComponent` to its real battle/unit IDs.
The policy component discovers that exact binding and configures once per pair.
Explicit ConfigureBattleUnit is also available to the same trusted factory.
No policy can turn an unbound mesh or an invented ID into a battle participant.

Each policy declares exact Identity/Version IDs, names of canonical string fields
for identity/Version/life state, and the required life value. These fields and
values come from authored NPC state; the runtime supplies no universal life-state
vocabulary. An optional phase field supports actual canonical boss/encounter state.
The local Ruler must still own the GameCore encounter. Human-controlled pawns,
the Ruler, the selected character and all locally owned Manifestations are excluded
from opponent automation. Invalid/dead/changed canonical participants cannot act.

Rules select an actual active or Ultimate skill from that exact unit's existing
kit. Rule priority and lexical key order are deterministic. Optional canonical
phase and own-health-ratio conditions gate rules. Targets use actual teams,
presence and HP: self, primary/lowest-ratio/highest-attack enemy, lowest-ratio ally,
all active allies/enemies, or a genuinely defeated ally. The latter only proposes
a target; it cannot grant revival. There is no fixed small AoE target ceiling.
Health ratios use the existing large-number representation; AI performs no damage,
healing, cooldown, energy, QTE, Rank or status arithmetic.

Decision timing and command context (including an exact Ultimate branch) are
authored data. Normal action delay comes from the source unit's existing resolved
timing. The existing FOGTurnBattle expands approved field targets and validates
the ability, resources, cooldowns and effects. An unavailable rule can yield only
to another authored rule. If all applicable rules fail, the component records
the actual dependency and does not retry the unchanged battle-log sequence every
frame. A changed native battle state or explicit RetryDecision allows another
decision. ReleaseBattleUnit disables automatic rebinding until an explicit retry
or configuration. Ruler Mode does not silently execute the player's turns.

After a committed action, actual HP changes drive existing native impact cues and
exact-rig hit/heal/defeat semantics; the source faces its actual target and requests
its exact skill/Ultimate/branch motion. Missing clips never create substitute poses.
Core release retires the transient agent binding without changing canonical state.

The initial package contains no fabricated NPC policies. Original encounter/NPC
definitions, registered effect providers, native bodies and shared motions remain
content/integration requirements. Two native automation tests cover source-kit
selection, phase/health conditions, authored fallbacks, full-field targeting and
stable tie-breaks. They have not been executed here. No native build, rendered
opponent battle, Android package or owner acceptance is implied by this source.
