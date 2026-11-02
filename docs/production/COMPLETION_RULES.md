# NoWorld — mandatory completion evidence

Effective date: 2026-10-16. Applies to every Gate, issue, production report, handoff, commit, acceptance notice and release summary.

## One meaning for DONE

**DONE = all exact frozen upstream requirements implemented, integrated, tested, visually accepted where specified, and committed to the designated Git branch with independent reproducible evidence.** Nothing else is DONE.

- **SOURCE_IMPLEMENTED** means source exists, not compiled, usable or accepted.
- **RUNTIME_VERIFIED** means the specified test actually ran on the cited build with an observable result. It does not assert that the original frozen visual design is reproduced.
- **VISUAL_VERIFIED** means real renderer evidence exists; it does not assert the screenshot *matches* a frozen approved image until the gate acceptance explicitly says so.
- **ACCEPTED** is an explicit independent Gate authority decision against the actual source/version. It does not imply downstream packaging, Android tests or full game completion.
- **DONE** requires every frozen requirement for that exact task, and no remaining requirements.
- **BLOCKED** preserves exact unmet requirements, their owner and input. Do not generate simulated ownership, currency, character art or animation as a workaround.

A gate may report a smaller, precisely-scoped subtask DONE once all requirements of *that subtask* pass, but cannot rewrite task requirements after the fact to erase uncompleted items.

## Machine enforcement

Canonical `docs/production/ACCEPTANCE_LEDGER.json`. The checks fail closed:

```text
python Scripts/production/verify_completion_contract.py
python -m unittest discover -s Scripts/production -p test_completion_contract.py -v
```

New CI `.github/workflows/completion-contract.yml` runs on production branches, pull requests and main. A claimed DONE requires the full matching requirement list, zero unresolved blockers, SHA256-addressed committed source + actual native-test + separate owning-Gate approval evidence, fixed tested acceptance commit; for visuals additionally raw source-art, real visual comparison, user design approval; for Android final additionally real device testing and user approval.

**Do not fabricate approval files.** Source hashes and a GitHub issue/acceptance response must actually derive from the relevant Gate/user decision. The checker prevents incomplete formal ledger claims but cannot independently authenticate a person or replace real testing and human comparison. Branch protection must require the check to prevent override; a green workflow alone is not release approval.

All user-visible reports should cite the ledger task id, source commit and test evidence with their limited status. A build/green unit test or image lookup is NEVER authority to claim an approved opening/World HUD/character animation.

## Frozen Gate 10 exact reference hashes (SHA256)

Supplied original `Gate10_FINAL_Gate00_Handoff_2026-10-13.zip` SHA256: `b92074ca762d4db3228ef237747e664ea3abc9c2da48941a20716ae84c2e7f49`. The references below are **visual targets only**, never screenshots pasted as interactive game UI.

- Opening direction 04 `variants/04_vacant_throne.png`: `63b824ccb8740983961d79f59bb7809fa1565ad8e22b0f7d9f4ffaa273594d71`.
- World HUD `01_FROZEN/WorldHUD/world_hud_freeze.png`: `6d7691507ce3792e8e75289496d20c946c148ee83a188d720960a876bab2616e`.
- Ruler Home `01_FROZEN/RulerHome/approved_visual.png`: `ad44793fbb1e9edcc09a03249cc213b3d9c6df000957dbdb2388754c3ecc4d1e`.
- Records `01_FROZEN/Records/approved_visual_with_placeholder_text.png`: `4fb28851a622ad8cad93f63232cfa9c43b41dbcfb49bae2f80edf95ad783115b` (displayed content is NOT canonical).
- Gacha `01_FROZEN/Gacha/user_approved_reference.png`: `a8978a82348d9c719d2bde754564bded7959b38fc6ba6226c3b8507881baeeb6`.
- Territory `noctis_basin_astral_territory_map.png`: `68b49c6ff05b489c668420dda2121ff0c46531ecb4883e7102010b5c12b4ada2`.
- Territory `gothic_territory_map_of_astral_prime.png`: `ab30192affaac97b3684303b14fedb3352dfcc0be032acf758c7c7080ae4e0bd`.
- Territory `gothic_celestial_territory_map.png`: `8b6a669b64f89f8f94b103571d1ac01a712161bfd241a02c0572abfe0d8eaf8a`.

User supersedes older documentation that proposed **character chips**: no chips; a repeat acquisition represents an independently owned Manifestation. Keep frozen Identity/Version/Manifestation, Privacy rendering vs actual state, six thin-line rarity frames and character-specific Absolute. **Sword/melee** is the required World combat visual, never firearm character default. Gate09 actual sword animations/rig, Gate01 authentic character visuals, Gate02 world, Gate07 item equip, Gate11 provenance, Gate10 visual targets, Gate12 runtime integration remain distinct. Foundation frozen, PR#16 unmerged.

## Current truth

Full game release: BLOCKED. Gate10 art-direction match: BLOCKED. Gate12 native Ruler/World round-trip: RUNTIME_VERIFIED, not final visual. Gate12 isolated genuine 45-piece Luminous viewer: RUNTIME_VERIFIED, not accepted P0 fidelity. Android: not newly cooked or installed (explicit stop). The ledger is the machine authority for statuses; this text is not an acceptance certificate.
