# Classless branch

This branch adapts mod-item-affixes to
[mod-classless-wildcard](https://github.com/DustinHendrickson/mod-classless-wildcard).
Built from upstream `Nevaden/mod-item-affixes` @ `d313190`.

mod-classless-wildcard turns players into "Heroes" on a Paladin chassis. A Hero
owns abilities and talents from every class, so `Player::getClass()` says
nothing about what it can do. Upstream filters almost every roll by
`getClass()`, which left a Hero with Paladin affixes only.

## What changed

All class questions go through `src/ClasslessBridge.{h,cpp}`. The bridge reads
the Wildcard character state (`ClasslessMgr`) when `ClasslessMgr.h` is on the
include path (`__has_include`, true when both modules are built statically into
the same worldserver). Without mod-classless-wildcard, or for players it exempts
(bot account prefix), the bridge answers from the real class and the module
behaves as upstream.

| Area | Upstream | Classless branch |
|---|---|---|
| Class skill affixes | real class + spell known | every class the Hero owns something from + spell known |
| Imprints | real class + required spell known | owned classes + required spell known |
| Talent affixes | real class + dominant tree | the talent must be owned (`Classless.RequireOwnedTalent`); owned classes |
| Spec (dominant tree) | most ranks in own class's trees | the talent tab with the most owned ranks, any class |
| Stat affixes | role/main stat auto-picked from class+spec | weighted by the Hero's stat allocation (`Classless.StatAllocWeighting`) |
| Role/main stat selectors | shown per class/spec | hidden for Heroes |
| Progression | on by default | off by default; the class-affix gate only applies while Progression is on |

### Talent ownership

`talent_affix_def` names its talent but has no Talent.dbc id. At load the
module resolves the id from the talent's rank-1 spell name within the def's
class (a trailing `" (Spell)"` qualifier is ignored). 171 of 182 defs resolve
against 3.3.5a client data. The other 11 name talents that do not exist in
3.3.5a under that name and never roll while `RequireOwnedTalent = 1`:
Improved Frost Nova, Sanctified Seals, Empowered Moonfire, Improved Regrowth,
Improved Serpent Sting, Improved Sunder Armor, Improved Battle Shout,
Improved Death Coil, Improved Blood Strike, Improved Fade, Improved Enslave Demon.

### Stat allocation weighting

Factor per stat affix, relative to an even allocation (an even allocation rolls
exactly like upstream). Multiplies the template weight.

- Main stat (STA/STR/AGI/INT/SPI): `share / 0.2`
- Role-masked stat, best role in the mask: Physical `(STR+AGI) / 0.4`,
  Ranged `AGI / 0.2`, Tank `STA / 0.2`, Caster `INT / 0.2`, Healer `SPI / 0.2`
- Other stats (crit, haste, hit, ...): 1
- Floor: `Classless.StatAllocMinWeightPct` (default 10%)

A Hero with no points allocated rolls uniformly.

## Core patches on Linux

`scripts/apply_core_patches.py` is a Python port of `apply_core_patches.ps1`
(same patches and detect strings, idempotent, `--check` for a dry run):

```
python3 scripts/apply_core_patches.py /path/to/azerothcore --check
python3 scripts/apply_core_patches.py /path/to/azerothcore
```

Verified against `mod-playerbots/azerothcore-wotlk` `Playerbot` @ `be923cb`.

## Known gaps

- An affix stays on an item after a Wildcard reroll or unlearn removes the
  spell or talent it was rolled for. The modifier keeps applying (a spell-specific
  one then affects nothing) and the tooltip does not show it as inactive.
- Talent affix values do not scale with the owned talent rank.
- Manual mode's "Talent Tree" selector still lists the chassis class's trees.
  D3 mode (the default) has no selector.
- Both modules ship custom `Spell.dbc` entries in client MPQ patches. These have to be
  merged into one patch. A first check found no overlapping custom spell ids.
- The SQL generator and client patch scripts are PowerShell (`pwsh`).
