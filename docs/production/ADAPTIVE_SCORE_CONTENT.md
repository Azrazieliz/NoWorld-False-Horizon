# Authored adaptive score

`UOGAdaptiveScoreComponent` is installed on the existing World character. Assign a
`UOGAdaptiveScoreDefinition` to its Score property in the content pawn Blueprint.
No soundtrack, lore, faction, encounter or audio asset is invented by the runtime.

Each rule has a unique RuleId, authored priority, required/excluded tags, a real
Music-category semantic audio definition, fade-in/out and minimum hold duration.
The sound asset/MetaSound owns looping, synchronized stems and musical phrasing.
The runtime does not independently start unsynchronized stems or invent a tempo.
Use soft sound references to retain normal asynchronous audio loading and cooking.
Set bWorldOnly=false for a rule meant to play in Ruler Mode.

The component reads `mode.world`/`mode.ruler` from the presentation subsystem and
`battle.running`/`battle.idle` from the canonical GameCore battle. Actual biome,
weather, faction and cinematic presentation providers publish their own named
tags with SetContextTags(Provider, Tags), then ClearContextTags on leaving context.
Providers cannot override the reserved mode/battle namespaces. All requirements
are AND conditions; any excluded tag vetoes a rule. Highest priority wins, with
RuleId lexical order resolving ties independently of asset array order.

An invalidated rule bypasses its musical minimum hold. A still-eligible current
rule may finish its authored minimum hold before a higher-priority rule replaces
it. The old score stays audible while a replacement soft asset loads, and only
crossfades after sound playback actually starts. Reversing a pending transition
cancels the obsolete load. Reversing a fade reuses the existing sound instance.
No matching rule fades the owned score to silence. Music events retain the
existing volume/profile/routing settings; changing settings during a fade does
not erase its envelope. Local control loss, Core unavailability and EndPlay retire
only this component's events. Existing SFX, voice, captions and haptics remain
owned by their existing runtime and settings.

This checkpoint is source implementation. Native compilation, actual authored
score admission, audible transition verification and Android acceptance remain
separate evidence; an unassigned Score intentionally produces no replacement music.
