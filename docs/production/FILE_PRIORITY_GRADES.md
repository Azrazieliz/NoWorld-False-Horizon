# NoWorld — Source-File Importance Grades (1★–7★)
Updated: 2026-11-01. Classifications describe maintenance priority, not creative authority.

| Grade | Exact responsibility | Typical examples |
|---|---|---|
| **7★ — locked** | The complete canonical game design and later approved supersession rules; `v0.40` baseline plus selective `v0.41` mature-content changes and later explicit accepted decisions. | `docs/production/NOWORLD_7STAR_AUTHORITY.md`, archives referenced there (historical source evidence only) |
| **6★ — locked** | Full integrated gameplay advancement against 7★, exact source commits, genuine player-visible results, remaining gaps, device acceptance. | `docs/production/ADVANCEMENT_PROGRESS.json` |
| **5★ — locked** | Reusable agent-operable production instructions AND runnable automation: source cards, Blender, animation, artwork, Unreal import, native asset registration, playable action linking, GitHub LFS publishing. | `docs/production/CONTENT_BINDING_5STAR.md` and the implementation scripts it indexes |
| **4★** | Essential current runtime gameplay implementation: combat/World/UI controllers, canonical persistence, effects, animation runtime and authoritative data models. | `Source/OfflineGame/**`, `Config/DefaultGame.ini` |
| **3★** | Essential actual content assets and original approved designs, versions and provenance; their artistic approval is decided by 7★/accepted source, not by this grade. | Original character cards, approved artwork, source clips, audio and authored catalog assets |
| **2★** | Supporting evidence, reproducibility, build toolchain, integration tests, CI workflows and diagnostic history. No automatic gameplay-completion credit. | `.github/workflows/**`, `docs/production/gate12/evidence/**` |
| **1★** | Low-priority temporary notes, superseded logs and disposable generated caches. Only genuinely disposable material qualifies; never classify irreplaceable original source as 1★. | Temporary local diagnostics; superseded scratch records |

## Classification rules
- This is a grade **by responsibility**, not a license to erase data or a formula that 7★ documents automatically make their referenced scripts 7★. Referenced production programs remain 5★; historical complete master text is original source evidence indexed through the 7★ authority.
- A file may be associated with several roles but has exactly one primary grade in an automated registry. Grade ownership stays 7/6/5 as the Creative Director defined, regardless of technical urgency.
- Missing latest decisions are marked `UNRECOVERED`, conflicting sources recorded as `SUPERSEDED_IF_CONFLICTING`; never infer unapproved content.
- Do not promote technical scripts, tests, commits or documentation to game progression. Update 6★ only for objective player-facing game-system advancement with honest status.
- Future sessions read 7★ → 6★ → 5★, then the touched 4★ runtime code, 3★ source art/content, 2★ evidence as needed. No repeated full historical audit.
