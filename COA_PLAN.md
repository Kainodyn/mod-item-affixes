# mod-item-affixes for Conquest of Azeroth: kartlegging og forslag

Branch `coa` i `Kainodyn/mod-item-affixes`. Base: `d313190` (Nevaden-upstream via Kainodyn-forken).
Kartlagt 2026-11-23 mot CoA-core `c9be83a58f` (`live`-build på NamdalNode). Fase 1 er implementert på
branchen (se §8); dokumentet beholder forslaget og beslutningene den bygger på.

## 1. Hva modulen er

| Del | Filer | CoA-vurdering |
|---|---|---|
| Stat-affixer (generics, `affix_type 1`) | `affixes/generics_defs.json`, `ItemAffix.cpp` | **Behold.** Kjernen i CoA-versjonen. |
| Spellmod-affixer / "class skills" (`affix_type 0`, ~645 stk) | `affixes.json`, `class_affixes/*.json`, `SPELLS_REFERENCE.csv` | **Fjern.** Laget for de 10 WotLK-klassene (SpellFamily + family flags). Nesten alle spillere er CoA-klasser 12–32. |
| Spell swap (`affix_type 2`) | hunter-JSON, `CollectAndApplySpellSwaps` | **Fjern.** Krever custom spells i DBC. |
| Talent-affixer | `talent_affixes/`, `item_talent_affix`, `LoadTalentAffixDefs` | **Fjern.** Leser `GetTalentMap()`/talent tabs; CoA-talenter er `CharacterAdvancement`/`ChrSpecs` og vises ikke der. |
| Imprints | `src/Imprints/`, `imprints/`, `data/sql/db-world/imprints/`, `item_imprint` | **Fjern helt** (ikke tema nå). Krever Spell.dbc-patch og klient-MPQ. |
| Gem-affixer | `item_gem_affix`, `OnSocketGem` | **Fjern** (krever core-hook `OnPlayerSocketGem` som CoA ikke har). |
| Crit rolls (×1,5, gull linje) | `ItemAffix.cpp` | **Behold.** |
| D3-modus (auto-roll) / Manual-modus (Alt+klikk) | `ItemAffix.cpp`, addon | **D3 som eneste modus** i første fase. Manual kan beholdes i koden, men av. |
| Reforge (NPC 601107 + `/reforge`) | `ReforgeMasterNPC.cpp`, `ReforgeUI.lua` | **Behold** (gullsluk). ID 601107 er ledig i `acore_world`. |
| Player Progression (meta-XP, `/prog`) | `PlayerProgression*.cpp`, `ProgressionUI.lua` | **Av i fase 1** (`ProgressionEnabled = 0`), vurderes senere. Se §5. |
| Klient-DBC/MPQ-patching | `tools/`, `scripts/patch_*.ps1`, `3-patch-client.bat` | **Fjern.** Bare spellmod/imprint brukte dem; stat-affixer har `enchant_id 0`. |
| Core-patcher | `CORE_PATCHES.md`, `apply_core_patches.ps1` | **Fjern behovet.** Se §4. |

## 2. Forslag per stat-affix

Rolle og hovedstat hentes fra spillerens aktive CoA-spec (se §3), ikke fra klasse/talent tree.
Verdier = item-budsjett (iLvl × slot × kvalitet × `StatMultiplier`), som i upstream.

| Affix | Upstream | Forslag | Begrunnelse |
|---|---|---|---|
| Stamina | alle | **Behold**, alle | Universell. |
| Strength / Agility / Intellect | alle (fritt) | **Behold, men gate på spec**: spec-ens hovedattributt får høy vekt; andre attributter i spec-ens `PrimaryStats` lav vekt; resten ikke | Unngå Str på casters. `ChrSpecs.PrimaryStats` finnes allerede i core (`AscensionSpecLoot::ActivePrimaryStats`). |
| Spirit | alle | **Behold**, kun spec med Spirit i `PrimaryStats` eller healer-rolle | Mange CoA-healere har Spirit som hovedstat. |
| Crit / Haste / Hit Rating | alle | **Behold**, alle | Fan-out til melee/ranged/spell som WotLK-kombinert rating. |
| Attack Power | PHYSICAL | **Behold**, melee + tank (fysisk) | |
| Ranged AP | RANGED | **Behold**, ranged-spec | Ranged-flagget finnes i ChrSpecs. |
| Spell Power | CASTER/HEALER | **Behold**, caster + healer | |
| Expertise | PHYSICAL | **Behold**, melee + tank | |
| Armor Pen | PHYSICAL/RANGED | **Behold**, fysisk dps, lav vekt | |
| Mp5 | CASTER/HEALER | **Behold**, kun hvis spilleren har mana (`POWER_MANA`) | CoA-klasser bruker ulike ressurser. |
| Hp5 | alle | **Behold med lav vekt** eller fjern | Svak stat; mest fyll. |
| Armor (flat) | TANK, armor | **Behold**, tank, kun armor-slots | |
| Dodge / Parry / Defense | TANK | **Behold**, tank, blå+ | |
| Block Rating / Block Value | — | **Ny (valgfri)**: tank, kun skjold | Mangler i upstream; enkelt via `ApplyRatingMod(CR_BLOCK)` / block value. |
| Spell Penetration | — | **Ny (valgfri)**: caster, lav vekt | PvP-relevant. |
| Damage Reduction % | TANK, blå+ | **Fjern** (eller hard cap 3 % totalt) | Prosent-DR stacker over 16 slots og gjelder også PvP. |
| Move Speed % | boots, blå+ | **Fjern i fase 1**; ev. senere som ikke-stablende aura | Implementert med `SetSpeed`, som overskrives av `UpdateSpeed` ved aura/mount-endringer. Feilutsatt. |
| Life Leech | allerede fjernet upstream | **Ikke ta inn** | |
| Pet-affixer (cooldown, health, damage, DR, attack speed) | hunter/warlock-JSON | **Fase 2**, gate på pet-specs (f.eks. Necromancer Animation, Witch Hunter Houndmaster, Tinker Mechanics, Hunter BM) via config-liste | Generisk kode, men pet-specs må listes manuelt. |
| Max Health (flat) | kun Progression | **Ikke som item-affix** | |

### Slots og verdier (forslag til start)
- Grønn 1, blå 2, epic 3, legendary 4, +1 for 2H (som upstream). Hvit/grå: ingen.
- `StatMultiplier = 1.5` (dist-fila har 3.0, README sier 1.0). Ved Classic iLvl 66–92 gir dette ca. +10–25
  stat per affix på epics; tunes etter klienttest.
- **iLvl-tak**: clamp budsjett-iLvl til f.eks. 100 for items med `RequiredLevel <= 60`. DB har ~100 items med
  iLvl 200–350 og reqlvl ≤ 60 (test-/custom-items som "Daniel Sword"); de ville ellers gitt WotLK-tall.
- Crit roll 10 %, ×1,5.

## 3. Rolle- og spec-oppløsning (må skrives om)

Upstream bruker `GetDominantTalentTree()` + hardkodet tabell for 10 klasser (`GetAutoRole`, `GetAutoMainStat`).
For CoA-klasser returnerer den −1, så alle får "any"-rolle.

Forslag: les aktiv spec med `GetAscensionActiveSpecialization(player)` og rolle-flaggene i `ChrSpecs.dbc`
(felt 11–15 = melee, ranged, caster, tank, healer; felt 8–10 = PrimaryStats; felt 4–7 = armor type).
Alle 101 specs har flagg, inkludert de originale klassene (spec 64–93) og Hero (94, alle roller → "any").
Implementering: modulen leser `ChrSpecs.dbc` selv ved oppstart (samme fil som core), eller en generert
tabell i en header som valideres mot `AscensionCompatData::CoASpecializations` ved oppstart.
Ingen spec (lavt nivå, før spec er valgt): roll bare universelle affixer (Stamina, Crit, Haste, Hit).

## 4. Core-avhengigheter (ingen core-patcher)

| Upstream-patch | Brukes til | Forslag |
|---|---|---|
| 1 `ApplyModToSpell` null-guard | spellmod/talent-affixer | Unødvendig når de fjernes. |
| 2 `OnPlayerSocketGem` | gem-affixer | Fjern gem-affixer. |
| 3/4 summon-damage loot/tap | imprint-summons | Unødvendig. |
| 5 ChatHandler-sentinel (`type = 0`) | bare loggstøy | Ikke patch core. Bruk samme mønster som andre CoA-moduler (Abyssal/Manastorm) for addon-meldinger; sjekk at loggen ikke spammes. |
| 6 `OnPlayerBeforeQuestReward` | `D3ExcludeQuestRewards` | Fjern innstillingen (quest-rewards rulles som annen loot), eller bruk eksisterende `PLAYERHOOK_ON_QUEST_REWARD_ITEM`. |

## 5. Player Progression (vurdering)
Separat system, ikke en affix-type. Av i fase 1. Om det slås på senere:
- Fjern noden `UnlockClassAffixes` (ingen class affixes).
- Av: `BossDrops` (overlapper CoA raid-/mythic-loot), `Lifesteal`, `MoveSpeed`, `CharacterXpPct` (realmen kjører x1 bevisst).
- Kandidater: reroll/options/crit-roll-noder og små stat-noder.

## 6. CoA-spesifikke tekniske krav
1. **Playerbots ekskluderes** (`WorldSession::IsBot()`) i alle hooks. Med ~1500 bots ville hver item-store gitt
   synkrone DB-spørringer og millioner av `item_affix`-rader.
2. **Wildcard-realm**: tabellene må også inn i `acore_characters_wc` (begge worldservere laster modulen).
3. **mod-random-enchants overlapper** (samme rolle: tilfeldige stats på loot, `OnLoot/OnCreate/OnQuestReward = 1`).
   Velg én, eller la affixer gjelde og senk random-enchants-sjansene. Forslag: skru av random-enchants når affixer går live.
4. **Addon**: affix-stats er server-side og vises bare i tooltip via addonen `ItemAffixes`. Må klienttestes mot
   Ascension/CoA-UI-et (tooltip-hooks, inspect, AH). Addonen må inn i klientpakken, som Abyssal.
5. **Re-apply-stier**: verifiser at stats ikke dobles/forsvinner ved CoA-spec-bytte (slot-bytte), prestige-reset,
   level-up og relogg. Vurder `PLAYERHOOK_ON_AFTER_APPLY_ITEM_MODS` i stedet for egen equip-bokføring.
6. Synkrone DB-kall i `InitItemSlots`/`SyncAffixes` er akseptable for få ekte spillere når bots er ute, men bør caches.
7. Ingen klient-DBC-endringer.

## 7. Implementeringsfaser
1. **Fase 1 – rydding + CoA-kompatibel stat-modul**: slett spellmod/spell swap/talent/imprint/gem-kode og -data,
   DBC/MPQ-verktøy og core-patch-script; skriv om rolle/spec (§3); bot-ekskludering; iLvl-tak; fjern DR% og
   Move Speed; SQL for begge character-DB-er; `ProgressionEnabled = 0`; `LootMode = 1`.
   Bygg i egen build-katalog, test på testoppsett før live. Ingen deploy uten godkjenning.
2. **Fase 2**: pet-affixer for pet-specs, Block/Spell Pen, Reforge-NPC-spawn, tuning etter klienttest.
   Først: GM-kommandoene (se §9).
3. **Fase 3 (valgfri)**: Player Progression; ev. egne CoA "class skill"-affixer bygget generisk fra
   `ChrSpecs.SignatureSpell` (felt 24) i stedet for håndskrevne spell-lister.

## Beslutninger (Hallvar, 2026-11-23)
- mod-random-enchants skal av når affix-modulen går live (først gjenstår arbeid på affix-modulen).
- `StatMultiplier = 1.5` og slots 1/2/3/4 (+1 for 2H) godkjent.
- Damage Reduction % og Move Speed fjernes.
- Hp5 og Armor Pen fjernes.
- Player Progression tas helt ut av fase 1 (bruker egne spell-ID-er 699100+ og skade-hooks); finnes i git-historikken.

## 8. Status fase 1 (2026-11-23)
Implementert på branch `coa`:
- Fjernet: spellmod/spell swap/talent/imprint/gem-affixer, Player Progression, manuell roll-UI, klient-DBC/MPQ-verktøy,
  install-script og alle core-patcher. Addonen er skrevet om (kun tooltip-visning og Reforge); `/roll` overstyres ikke lenger.
- `CoASpecRoles`: rolle og primærstats fra `ChrSpecs.dbc` (felt 8–15) + CoA sine tank/healer-lister;
  originale klasser via dominant talent tree → spec 64–93; Hero (94) = alle roller.
- Stats følger core-livssyklusen via `OnPlayerAfterApplyItemMods` (equip, unequip, destroy, durability 0, repair) og
  full sync ved login. Upstream mistet stats bl.a. når et utstyrt item ble slettet.
- Bots ekskluderes i alle veier; affix-cache med mutex (hooks kjører på map-tråder); asynkrone DB-skriv i én transaksjon.
- Sikkerhet: SQL-injection i `PEEKAUCTION` (selgernavn rett i spørringen) er fikset; inspect-oppslag kun for spillere innen 100 yards.
- Addon-meldinger «svelges» ved å tømme teksten i stedet for `type = 0` (som ga feillogg uten core-patch).
- Verifisert: alle `.cpp` kompilerer (`-fsyntax-only`, ingen warnings) mot CoA-core med live-byggets flagg.
- Ikke verifisert: full build/link, oppstart, SQL mot DB, Lua-syntaks (ingen Lua-parser tilgjengelig) og klienttest.

## 9. Kjent feil: `.affix`-kommandoene er utilgjengelige (funnet 2026-11-23 på live)
- **Symptom:** `.affix info`, `.affix spec` m.fl. virker ikke i spillet, heller ikke for `ADMIN` (GM-nivå 3).
- **Årsak (arvet fra upstream):** `ItemAffixCommands.cpp` registrerer kommandoene med `rbac::RBAC_PERM_COMMAND_GM`
  (= 371). CoA-coren tolker verdien som sikkerhetsnivå (`Chat.cpp`: `GetSecurity() >= AccountTypes(371)`), så ingen
  konto (maks 3) kommer gjennom; bare konsoll/SOAP.
- **Fiks (neste kodeendring):** bruk `SEC_GAMEMASTER` for `info`/`spec` og `SEC_ADMINISTRATOR` for `reroll` og
  `reforge roll/pick` (de endrer items og trekker gull). Krever rebuild og restart.
- **Alternativ uten rebuild:** rader i `acore_world.command` med ønsket `security` overstyrer koden; leses ved oppstart.
  Foretrekk kodefiksen, så modulen er riktig uten lokal DB-data.
- **Verifisering:** som `ADMIN` gir `.affix spec` spec-id og roller; som konto uten GM-nivå finnes ikke kommandoen.
- Merk: kontoene `HALLVAR`, `HALLVAR1` og `EAGLEEYE` har ingen GM-nivå; test med `ADMIN` eller gi kontoen GM-nivå.
- **Status 2026-11-24:** kodefiksen er gjort på branch `coa` (se `COA_SKILL_AFFIX_PLAN.md` §8); ikke bygd/deployet.
