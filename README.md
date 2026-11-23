# mod-item-affixes (Conquest of Azeroth)

Random stat affixes on gear for the Conquest of Azeroth AzerothCore fork. Branch `coa` of
[Kainodyn/mod-item-affixes](https://github.com/Kainodyn/mod-item-affixes), adapted from Nevaden's
module: only stat affixes, rolled automatically for the receiver's CoA specialization. Spellmod
("class skill"), spell-swap, talent, gem and Imprint affixes, Player Progression, the manual roll
UI, client DBC/MPQ patching and every core patch are removed. The plan and the decisions behind it
are in [COA_PLAN.md](COA_PLAN.md).

## How it works

- **Rolling.** A green, blue, epic or legendary weapon or armor piece rolls its affixes the moment a
  player receives it (loot, quest reward, crafting, vendor). An eligible item that never rolled
  (mail, auction, trade, gear from before the module) rolls for the wearer when first equipped or at
  login while equipped (`ItemAffixes.RollUnrolledOnEquip`). Rolled items keep their affixes through
  bank, mail and trade. Playerbots never roll or apply affixes.
- **Slots.** Green 1, blue 2, epic 3, legendary 4; two-handers +1 slot and 1.5x values. Shirts,
  tabards, bags, ammo, heirlooms and level-scaled items never roll.
- **Values.** `budget(iLvl) * slot modifier * quality fraction * StatMultiplier`, with the item level
  capped at `MaxBudgetItemLevel` (100) so test and custom items do not roll WotLK-sized values. A
  crit roll (10 %) gives 1.5x and shows in gold. An item does not repeat a stat while another fits.
- **Specialization.** Roles and primary attributes come from `ChrSpecs.dbc` (role flags and
  PrimaryStats) plus CoA's curated tank/healer lists. CoA classes use their active specialization;
  the original classes map their dominant talent tree onto ChrSpecs 64-93; Hero (Wildcard) may roll
  everything. Without a specialization only universal affixes roll.
- **Stats applied.** Through the core's own item stat lifecycle (`OnPlayerAfterApplyItemMods`):
  equip, unequip, destroy, durability 0 and repair, plus a full sync at login.
- **Reforge.** `/reforge`, or the Reforge Master (entry 601107, spawn with `.npc add 601107`): pay
  gold to reroll one line. The current value is always offered first; the item locks to that line.

## Affix pool (`data/sql/db-world/mod_item_affixes_template.sql`)

| Affix | Who | Notes |
|---|---|---|
| Stamina, Crit, Haste, Hit Rating | everyone | Ratings apply to melee, ranged and spell. |
| Strength, Agility, Intellect, Spirit | spec's primary stats | Main stat full weight, other listed attributes (and Spirit for healers) `SecondaryAttributeWeightPct`. |
| Spell Power | caster, healer | blue+ |
| Mp5 | caster, healer with mana | |
| Attack Power, Expertise | physical (melee/tank) | blue+ |
| Ranged Attack Power | physical ranged | blue+ |
| Armor | tank, armor slots | |
| Dodge, Defense, Parry Rating | tank | blue+ |

Removed for CoA: Hp5, Armor Penetration, Move Speed, Damage Reduction, pet affixes.

## Install

1. Clone into `modules/mod-item-affixes` (branch `coa`), re-run CMake and build the worldserver.
2. Copy `conf/mod_item_affixes.conf.dist` to the worldserver's module config directory.
3. The DB updater applies `data/sql` to the world DB and to each realm's characters DB.
4. Players install `addon/ItemAffixes` (required for tooltips) and optionally
   `addon/ItemAffixesToast` in `Interface/AddOns`.

Startup log: `mod-item-affixes: loaded N specialization role profile(s) from ChrSpecs.dbc.` and
`mod-item-affixes: loaded N affix template(s).`

## GM commands

| Command | Effect |
|---|---|
| `.affix info` | Affixes of every equipped item. |
| `.affix spec` | The specialization profile (spec id, roles, primary stats) new rolls use. |
| `.affix reroll <equipSlot>` | Replace every affix of an equipped item (1-based slot, see `.affix info`). |
| `.affix reforge status/roll/pick` | Reforge an equipped item without the addon. |

## Addon protocol

Prefix `AFXM`, whispered to the player themself; the server clears the request text so it never
echoes back (no core patch needed). Requests: `CONFIG`, `ALLDATA`, `DATA|bag|slot`, `PEEK|uid`,
`PEEKUNIT|name|slot`, `PEEKUNITALL|name`, `PEEKAUCTION|...`, `TRADEPEEK|slot`, `MYTRADEPEEK|slot`,
`REFORGE_STATUS/ROLL/PICK/PREVIEW|bag|slot|...`. Inspect lookups only answer for players within
100 yards in the same map; the auction lookup only accepts valid, escaped character names.

## Credits

Original module by Nevaden; ItemAffixesToast by Rykaerdoe. MIT License.
