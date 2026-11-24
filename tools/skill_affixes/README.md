# Skill-affix tools

Read-only tools for CoA class skill affixes (`COA_SKILL_AFFIX_PLAN.md` §6). Python 3.11+, no extra packages.

## gen_skill_candidates.py
```sh
python3 -B gen_skill_candidates.py [--dbc /opt/coa/server-data/dbc] [--defaults /opt/coa/secrets/my.cnf] [--out out/candidates]
```
Input: server DBC (`Spell`, `SpellCastTimes`, `SpellDuration`, `SpellRadius`, `SpellRange`, `ChrClasses`, `ChrSpecs`,
`CharacterAdvancement*`) via `/opt/coa/azerothcore/apps/coa-dbc`, plus SELECTs on `acore_world`
(`spell_dbc` replaces DBC rows, `coa_spell_fix` patches basepoints, `spell_bonus_data`, `coa_client_spell_description`).
Effect/aura names come from core `SharedDefines.h`/`SpellAuraDefines.h`.

Output (`out/candidates/`):
- `<class_id>_<TOKEN>.json` for classes 12–32: `candidates` (non-passive rank-1 CharacterAdvancement Ability/Talent
  entries, all specs + class tab = spec 0) and `passive_context` (passive entries, description cut at 200 chars).
  Field legend is in each file's `_legend`. Omitted numbers are 0, omitted bools false.
- `candidates.csv`: one row per candidate.

Notes: `mask_unique_bit` = one family-flag bit present on every rank and on no other-named spell in the family (DBC + `spell_dbc`),
else null. `children` = `EffectTriggerSpell` chain (depth ≤ 3); scripted triggers are not visible.
`is_summon` = `self`/`child` (a `SUMMON*` effect on the spell or a trigger child). `known_by_default` = free entry
(AE/TE cost 0) outside a choice group (heuristic from `AscensionCoATalentData.cpp`). `$s1`/`$d`/`$o1`/`$/10;s1`-tokens in
descriptions are substituted; formulas (`${...}`, `$?s...`) are left as-is. C++ `SpellInfo` corrections are not applied.

## validate_reviews.py
```sh
python3 -B validate_reviews.py [out/reviews/23_NECROMANCER.json ...]   # default: out/reviews/*.json
```
Validates reviewer files (schema plan §6.2) against the candidates: known spell ids, no summons/`ignore_caster_mods`,
targeting/op/type rules, magnitude caps, `min_quality >= 3`, unique keys (across files), ≤ 4 affixes per target and
≤ 3 effects per affix. Prints a report; exit 1 on any error.
