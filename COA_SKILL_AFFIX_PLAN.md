# CoA class skill affixes: teknisk plan

Status: **fase 3a + merge (3c) implementert i kode 2026-11-24, ikke bygd/testet** (se §8). Kartlagt 2026-11-23 mot modul-branch `coa` (`a541658`), CoA-core i
`/opt/coa/azerothcore` og DBC-er i `/opt/coa/server-data/dbc` (serverens kopi; klientens kopi er ikke sammenlignet).
Bygger på `COA_PLAN.md` §1, §3, §7 fase 3. Merket **[V]** = verifisert i kode/data, **[A]** = antakelse/må testes.

## 1. Slik virket upstream (`d313190`)

- `affix_type 0` (SPELLMOD): rad i `affix_template` med `spellmod_op/type/value` (opptil 4 effekter),
  `spell_family`, `spell_family_flags0..2` og `carrier_spell_id` (= selve mål-spellen, f.eks. Fireball 133)
  (`git show d313190:src/ItemAffix.cpp` l. 494–553; `class_affixes/mage.json`). [V]
- Påføring: `ApplyAffixes` lager `new SpellModifier(nullptr)` med `op/type/value`, `mask = flag96(family flags)`,
  `spellId = carrier` og kaller `player->AddSpellMod(mod, true)`; fjerning `AddSpellMod(mod, false)` som sletter
  moden fordi `ownerAura == nullptr` (d313190 l. 1805–1860). Crit/2H skalerte verdien via `rolled_value`
  150/200/250. [V]
- Kjernen matcher via `SpellInfo::IsAffected(affectSpell->SpellFamilyName, mod->mask)`: familien til carrier
  må være lik mål-spellens og maska må dele minst én bit med `SpellFamilyFlags`. [V] (`SpellInfo.cpp:1459`)
- Rulling: familie → klasse (`SpellFamilyToClass`, d313190 l. 452) og `PlayerKnowsCarrierSpell` (alle ranks i
  kjeden) filtrerte på mottakeren (l. 1101–1118). [V]
- Core-patcher som trengtes: **Patch 1** null-guard i `Player::ApplyModToSpell` fordi `ownerAura` er null
  (`CORE_PATCHES.md`). Tooltip på item kom fra patchet `SpellItemEnchantment.dbc` (`enchant_id` per affix,
  klient-MPQ). Øvrige patcher gjaldt gems/imprints. [V]
- Kjent upstream-feil: periodiske effekter krever `SPELLMOD_DOT` (22), ikke `SPELLMOD_DAMAGE` (0), og carrier
  må være spellen som faktisk gjør skaden (commit `5b060bb`). [V]

**CoA-coren i dag:** `Player::ApplyModToSpell` deref-er fortsatt `mod->ownerAura` uten null-sjekk
(`Player.cpp:10899–10906`), og kalles fra `ApplySpellMod` med ikke-null `spell` (l. 10599, 10625). Upstream-
mønsteret (`ownerAura = nullptr`) vil krasje ved cast uten core-patch. [V]

## 2. CoA-spells: family/flags og alternativer

### 2.1 Data (script: `extract.py` + `analyze.py`, se §6)
- Hver CoA-klasse har egen `SpellFamilyName` = klasse-id + 6 (Barbarian 18 … Spiritmage 38); >98 % av
  CharacterAdvancement-spellene per klasse ligger i klassens familie. [V]
- 743 aktive (ikke-passive, rank 1) spells i CharacterAdvancement for klasse 12–32 (701 Ability- og 2915
  Talent-entries totalt; 295 entries har >1 rank). [V]
  - 154 (21 %) har **ingen** family flags → kan ikke treffes med maske.
  - 370 (50 %) har minst én bit ingen annen navngitt spell i familien har → maske-unik.
  - 219 (29 %) deler alle bitene sine med andre spells (typisk grupper: alle "Build:"-spells hos Tinker deler
    `0x40000000`, Witch Doctor-ward/idol deler `0x2000`, Necromancer "Raise/Animate" deler `0x8`/`0x2`).
  - 242 bruker trigger-effekter (64/142/periodic trigger) → skaden ligger i en annen spell-id; 58 er summons
    (skaden gjøres av en creature, ikke spilleren); 1 har `SPELL_ATTR3_IGNORE_CASTER_MODIFIERS`.
- Core har allerede kollisjons-workarounds per spell (`SpellInfo.cpp:1586–1634`, f.eks. "copied Dark Veil mask
  collides with Hammer" for familie 31). Flagg er altså ikke pålitelig unike. [V]

| Klasse | Aktive (Ability/Talent) | 0 flags | Unik bit | Kun delte | Per spec (0 = klassetab) |
|---|---|---|---|---|---|
| 12 Barbarian | 33 (10/23) | 4 | 23 | 6 | 0:10 1:7 2:5 3:11 |
| 13 Witch Doctor | 43 (12/31) | 12 | 15 | 16 | 0:11 4:8 5:8 6:16 |
| 14 Demon Hunter | 27 (8/19) | 3 | 19 | 5 | 0:8 7:6 8:6 9:7 |
| 15 Witch Hunter | 44 (8/36) | 4 | 26 | 14 | 0:11 10:7 11:8 12:10 97:8 |
| 16 Stormbringer | 35 (9/26) | 10 | 19 | 6 | 0:10 13:13 14:7 15:5 |
| 17 Fleshwarden | 26 (7/19) | 6 | 11 | 9 | 0:9 16:6 17:7 18:4 |
| 18 Guardian | 28 (11/17) | 13 | 11 | 4 | 0:10 19:3 20:11 21:4 |
| 19 Monk | 27 (8/19) | 6 | 16 | 5 | 0:11 22:6 23:5 24:5 |
| 20 Son of Arugal | 42 (13/29) | 5 | 22 | 15 | 0:10 25:12 26:8 27:7 99:5 |
| 21 Ranger | 40 (8/32) | 11 | 18 | 11 | 0:12 28:7 29:12 30:9 |
| 22 Chronomancer | 34 (12/22) | 7 | 18 | 9 | 0:12 31:10 32:5 33:7 |
| 23 Necromancer | 35 (14/21) | 3 | 11 | 21 | 0:14 34:6 35:8 36:7 |
| 24 Pyromancer | 30 (9/21) | 7 | 17 | 6 | 0:12 37:9 38:4 39:5 |
| 25 Cultist | 35 (7/28) | 8 | 15 | 12 | 0:11 40:7 41:6 42:7 96:4 |
| 26 Starcaller | 38 (10/28) | 10 | 16 | 12 | 0:11 43:7 44:6 45:8 100:6 |
| 27 Sun Cleric | 42 (8/34) | 7 | 18 | 17 | 0:10 46:7 47:9 48:7 98:9 |
| 28 Tinker | 37 (12/25) | 6 | 18 | 13 | 0:11 49:9 50:8 51:9 |
| 29 Prophet | 45 (8/37) | 16 | 25 | 4 | 0:13 52:9 53:7 54:7 101:9 |
| 30 Reaper | 28 (11/17) | 6 | 19 | 3 | 0:11 55:6 56:6 57:5 |
| 31 Wildwalker | 42 (7/35) | 8 | 21 | 13 | 0:12 58:9 59:7 60:7 95:7 |
| 32 Spiritmage | 32 (8/24) | 2 | 12 | 18 | 0:10 61:6 62:8 63:8 |

"Unik bit" sammenligner mot *alle* spells i familien med annet navn (også triggered barn/NPC-varianter), så
tallet er konservativt. Spec 95–101 er fjerde-specs (finnes i `ChrSpecs.dbc`).

### 2.2 Spell-id-basert targeting i CoA-coren (uten core-patch)
- `SpellModifier::targetSpellId` (CoA-felt, `Player.h:192`): `IsAffectedBySpellMod` avviser alle andre spell-id-er
  (`SpellInfo.cpp:1577`). Med carrier i familie 0 og `mask = 0` gir `IsAffected(0, 0)` true
  (`SpellInfo.cpp:1461`), så moden treffer **nøyaktig én spell-id**. Brukes allerede av
  `AscensionTinkerHacking.cpp:50–60`. [V]
- GlobalScript-hooks: `OnIsAffectedBySpellModCheck` (false ⇒ tvinger "affected", `SpellInfo.cpp:1637`) og
  `OnSpellModFamilyMask(affectSpell, checkSpell, mod, bool& affected)` (kan overstyre resultatet begge veier,
  `SpellInfo.cpp:1661`). Gir sett-basert targeting (alle ranks + triggered barn) med én mod. [V]
- Eierskap: `new SpellModifier(aura)` med en ekte `Aura*` er core-mønsteret (`spell_druid.cpp:602–620`, Idol of the
  Emerald Queen; `AuraEffect` sletter bare sin egen `m_spellmod`, `SpellAuraEffects.cpp:462`). [V]
- Server-side spells: `spell_dbc` lastes inn i `sSpellStore` (`DBCStores.cpp:381`); CoA har 4091 rader, mange
  passive dummy-auraer (`Attributes 448`, aura 4). ID-område 9979000–9979999 er ledig i Spell.dbc og
  `spell_dbc`. Passive auraer sendes ikke til klienten (`Aura::CanBeSentToClient`, `SpellAuras.cpp:1097`). [V]
- Andre hooks: `UnitScript::ModifySpellDamageTaken(target, attacker, damage, spellInfo)` (`Unit.cpp:1700`),
  `ModifyPeriodicDamageAurasTick` (`SpellAuraEffects.cpp:6643/6764/6975`), `ModifyHealReceived`
  (`Unit.cpp:9303`; arg-rekkefølgen `this, GetTarget()` må verifiseres), `ModifySpellEffectBaseValue`
  (`SpellInfo.cpp:523`), `AllSpellScript::OnSpellCritChance` (`Spell.cpp:8664`), `OnCalcMaxDuration`
  (`SpellAuras.cpp:815`), `PlayerScript::OnPlayerSpellCooldownCalculated` (kun varsling, `Player.cpp:11857`),
  `Player::ModifySpellCooldown` (sender `SMSG_MODIFY_COOLDOWN`, `Player.cpp:11986`). [V]
- Klientpakker: `AddSpellMod` sender `SMSG_SET_FLAT/PCT_SPELL_MODIFIER` **bare for bitene i maska**, i
  Ascension-layout med `SpellFamilyName` (`Player.cpp:10736–10770`, `SendSpellModifier` l. 10694). `mask = 0`
  ⇒ ingen pakke ⇒ klienten vet ingenting. [V]

## 3. Realistiske affix-typer (uten klient-DBC og core-patch)

| Type | Mekanisme | Klient | Vurdering |
|---|---|---|---|
| % direkte skade/heal | `SPELLMOD_DAMAGE` PCT | server-autoritativ | **Ja.** Må peke på spellen som gjør skaden (triggered barn). |
| % DoT/HoT | `SPELLMOD_DOT` PCT | server | **Ja.** Egen op (upstream-feil `5b060bb`). |
| % absorb/effekt | `SPELLMOD_EFFECT1/2/3` eller `ALL_EFFECTS` PCT | server | Ja, men per spell-effekt; reviewer må velge riktig effekt. |
| Crit chance | `SPELLMOD_CRITICAL_CHANCE` FLAT (eller PCT>0, CoA-spesial `Player.cpp:10589`) | server | **Ja.** |
| Cast time | `SPELLMOD_CASTING_TIME` FLAT/PCT | cast bar fra `SMSG_SPELL_START` | **Ja.** Spell-tooltip viser DBC-tid. [A] |
| Duration | `SPELLMOD_DURATION` | aura-varighet sendes | **Ja.** Ikke for spells med varighet −1 (`Player.cpp:10522`). |
| Radius / jump targets | `SPELLMOD_RADIUS` / `JUMP_TARGETS` | server | Ja, lav vekt. |
| Cooldown | `SPELLMOD_COOLDOWN` FLAT | klienten holder egen cooldown | **Bare i maske-modus** (klient får spellmod-pakke) eller med `ModifySpellCooldown` etter cast. Ellers viser/blokkerer klienten full CD. [A] |
| Cost | `SPELLMOD_COST` PCT | klienten sjekker ressurs lokalt | Maske-modus; i spell-id-modus kun uskadelig når spilleren har nok ressurs. [A] |
| Range | `SPELLMOD_RANGE` | klienten sjekker range lokalt | **Kun maske-modus**, ellers "out of range"-blokk. |
| GCD | `SPELLMOD_GLOBAL_COOLDOWN` | klient-GCD | **Nei.** |
| Proc chance / PPM / charges | `CHANCE_OF_SUCCESS`, `PROC_PER_MINUTE`, `CHARGES` | server | Senere fase; CoA-procs er ofte script (`AscensionTalentDataProcs.cpp`). |
| Summons/pets (wards, builds, raises) | spillerens spellmods gjelder ikke creature-caster | — | Utelat, eller `ModifySpellDamageTaken` med `attacker->GetOwner()` (fase 4). |

Tooltip: item-linja vises av addonen (tekst bygges server-side i `BuildAffixDisplayString`), ingen
`enchant_id`/DBC. Spell-tooltipen endres bare hvis Ascension-klienten bruker mottatte spellmod-pakker i tooltips
(maske-modus). [A, klienttest]

## 4. Anbefalt mekanisme

> **Status 2026-11-24:** punkt 3 (maske-modus) og `hook_cd` er fjernet. Alle affixer bruker spell-id-modus (punkt 4,
> `targetSpellId` per mål-spell og alle ranks av den), cooldown via `SPELLMOD_COOLDOWN`, og vises i klienten med
> per-spiller streamede Spell-rader (eneste modus; klienttest A/B bestått 2026-11-24, se `COA_SKILL_TOOLTIP_PLAN.md`).
> Carrier-aura-livssyklusen (punkt 1, 2, 5) er uendret.

**Hybrid, alt eid av én skjult carrier-aura per spiller:**
1. Ny server-side spell i `spell_dbc` (f.eks. 9979001): passiv, skjult, `SPELL_AURA_DUMMY`, familie 0. Legges på ved
   login når spilleren har minst én skill-affix utstyrt (`AddAura`), fjernes når ingen. Ingen klient-DBC.
2. Per affix-effekt: `new SpellModifier(carrierAura)` og `AddSpellMod(mod, true)`; fjerning `AddSpellMod(false)` +
   `delete mod` (moden eies av modulen, ikke `AuraEffect`). Løser `ApplyModToSpell`-null-deref uten patch.
3. **Maske-modus** når generatoren beviser en unik bit for mål-spellen (alle ranks + barn): `mod->spellId =` mål-spellen
   (riktig familie i pakken), `mask =` den unike biten. Klienten får spellmod-pakke → CD/cost/range konsistent. [A]
4. **Spell-id-modus** ellers: `mod->spellId = carrier`, `mask = 0`, og `targetSpellId` per konkret id, eller ett mod
   + `OnSpellModFamilyMask` som slår opp `checkSpell->Id` i et `unordered_set` når `mod->spellId == carrier`
   (billig early-out). Kun ops fra §3 som er server-autoritative.
5. Sikkerhet: `UnitScript::OnAuraRemove` på carrier ⇒ fjern alle mods **før** auraen slettes (ellers dingler
   `ownerAura` i `m_appliedMods`). Logout/`RemoveAllAuras`, død (passive beholdes), prestige-reset må testes.

Alternativ uten carrier-aura: kun hooks (`ModifySpellDamageTaken`, `OnSpellCritChance`, `ModifySpellCooldown`).
Fungerer for skade/crit/CD, men ikke cast time/cost/duration/radius og er mer kode. Brukes som supplement for summons.

## 5. Datamodell og integrasjon i modulen

- **`affix_template`** (world): nye kolonner `affix_type` (1 = stat, default; 0 = skill), `class_id`, `spec_id`
  (0 = klassetab/alle specs), `target_spell` (rank 1, for visning), `targeting` (0 mask, 1 spell-id, 2 hook_cd;
  legacy, ignoreres av modulen siden 2026-11-24), `mask0..2` (legacy, ignoreres), `effects` som `op1,type1,value1 … op3` (som upstream), `value_scale` (0/1 skaleres av crit/2H).
- **`affix_skill_target`** (world, generert): `(affix_id, spell_id)` – alle ranks og triggered barn som skal treffes.
- **`item_affix`** (characters, begge DB-er inkl. `acore_characters_wc`): uendret skjema; `affix_id` peker på
  skill-raden, `rolled_value` = endelig verdi (etter tier/crit), `is_crit` som i dag.
- **Rulling** (`RollContext` i `ItemAffix.cpp`): skill-pool filtreres på `player->getClass()` og spec
  (`CoASpecRoles::Resolve` / `GetAscensionActiveSpecialization`) + `HasSpell` på en rank (upstream-logikk). Forslag:
  maks 1 skill-affix per item, kun blå+, egen `SkillAffixChance`-config. Originale klasser (1–11) og Hero (94): ingen
  skill-affixer i første omgang.
- **Påføring**: utvid `ApplyItemAffixes/RemoveItemAffixes` (kalt fra `OnPlayerAfterApplyItemMods`) og
  `ItemAffixPlayerData` med `activeSpellMods[itemGuid]`. Samme livssyklus som stats (equip/unequip/destroy/
  durability/repair, full sync ved login). Mods påføres uansett om spilleren kjenner spellen; de treffer bare ved cast.
- **Spec-bytte**: `SwitchSpecialization` lærer/fjerner spells (`AscensionCompat.cpp:2747+`), ingen egen hook funnet.
  Ikke nødvendig å re-applye; addonen viser affixen grå "(inactive)" når spilleren ikke har spellen (ny state `I` i
  `DATA`-payload, sjekkes server-side ved `SendItemStatus` og `OnPlayerLearnSpell/ForgotSpell`).
- **Visning**: tekst `"<Spell>: +15% damage"`; `|` er feltskille i payload (navn med `:` er ok,
  `ItemAffixes.lua:85`). Pass på 255-tegns grensen for addon-meldinger med 4–5 lange linjer.
- **Reforge**: samme pool-filter som rulling (spillerens klasse/spec).
- **Bots**: `Handles()` ekskluderer allerede bots; ingen carrier-aura på bots.

## 6. Arbeidsflyt: gjennomgang av alle spells

### 6.1 Kandidat-generator (`tools/gen_skill_candidates.py`, ny)
Prototype finnes i scratch: `extract.py` (leser DBC via `apps/coa-dbc/client_dbc.py` + `schemas.py`, samme
klasse/spec-mapping som `AscensionCoATalentData.cpp:140–260`) og `analyze.py`. Generatoren skal:
1. Lese `CharacterAdvancement` (felt 5–9 ranks, 32 class type, 33 tab, 1 type), `ChrSpecs` (1 klasse-token, 2 tab-token,
   24 SignatureSpell = **spell-id**, ikke entry-id), `CharacterAdvancementClassTypes/TabTypes`, `Spell`,
   `SpellCastTimes`, `SpellDuration`, `SpellRadius`; tab 87 = klassetab (spec 0).
2. Overlegge `acore_world.spell_dbc` (129 id-er overstyrer Spell.dbc), `coa_spell_fix`, `spell_bonus_data`, og tekst
   fra `coa_client_spell_description` (4489 rader).
3. Følge `EffectTriggerSpell[0..2]` (felt 116–118) rekursivt for å finne spellen som faktisk gjør skade/heal.
4. Per kandidat skrive: class, spec, entry, type (Ability/Talent), rank-ids, navn, beskrivelse, passive, cooldown
   (`RecoveryTime`/`CategoryRecoveryTime`), GCD, cost (`PowerType`, `ManaCost`, `ManaCostPercentage`), cast time,
   duration, radius, effekter/aura-typer/basepoints, family + flags, `mask_unique_bit` (eller null), trigger-barn,
   summon-flagg, `ignore_caster_mods`.
5. Output: `out/skill_candidates/<class>.json` + CSV. Forventet ~26–45 aktive kandidater per klasse (743 totalt);
   talenter (117–179 per klasse) tas med som `passive=true` for kontekst, ikke som mål.
Verifiser utvalgte spells med `coa-dbc-viewer record/map` og `.spellinfo` i spillet.

### 6.2 Én reviewer-agent per klasse
Input: klassens kandidatfil + `ascension-spell-parity.md`. Output (én fil per klasse, validert av script):

```json
{
  "class_id": 23,
  "class": "NECROMANCER",
  "reviewed_at": "YYYY-MM-DD",
  "affixes": [
    {
      "key": "necro_glacial_tap_cd",
      "name": "Glacial Tap: -2 sec cooldown",
      "spec_id": 0,
      "target_spell": 805369,
      "affected_spells": [805369],
      "targeting": "mask|spell_id",
      "mask": [0, 262144, 0],
      "effects": [{ "op": "COOLDOWN", "type": "FLAT", "value": -2000 }],
      "scales_with_crit": true,
      "min_quality": 3,
      "weight": 100,
      "evidence": "Spell.dbc 805369 RecoveryTime=12000; flags[1]=0x40000 unique in family 29",
      "client_sensitive": true,
      "confidence": "high|medium|low",
      "notes": ""
    }
  ],
  "rejected": [{ "spell": 500989, "reason": "summon; damage by creature" }]
}
```
Regler for reviewer: aldri finn på verdier (fra DBC/DB/kode, jf. core `general.md`); kun ops fra §3; `targeting =
mask` bare når generatoren har gitt `mask_unique_bit`; `COOLDOWN/COST/RANGE` kun med `mask`; ≤ 4 affixer per spell,
~10–20 per klasse; magnitude-tak (forslag: damage ≤ 10 %, CD ≤ 15 % av base, cast ≤ 0,5 s, crit ≤ 5 %).
Et merge-script lager `affix_template`/`affix_skill_target`-SQL og avviser ukjente spell-id-er, op-er og maske-konflikter.

## 7. Risikoer, åpne spørsmål og faser

**Risikoer**
- Dinglende `ownerAura` hvis carrier fjernes før mods → krasj. Krever test av logout, død, reset, `RemoveAllAuras`.
- Klient/server-avvik for CD/cost/range i spell-id-modus; maske-modus forutsetter at Ascension-klienten bruker pakkene [A].
- CoA-spesialkode i `IsAffectedBySpellMod` per `mod->spellId` (familie 31/33/35 …) kan slå inn i maske-modus.
- Triggered/summon-skade: affix gjør ingenting hvis feil spell-id velges (upstream-feilen gjentatt).
- Balanse: spellmods stacker over 16 slots og gjelder PvP; trenger tak per spell.
- Data endres ved CoA-oppdateringer (`live`-tags): generator + valideringsscript må kjøres på nytt, og affixer med
  forsvunne spells må deaktiveres (ikke slettes fra `item_affix`).
- `OnSpellModFamilyMask`/`OnIsAffectedBySpellModCheck` kjøres for alle spellmod-sjekker; early-out på carrier-id er påkrevd.

**Åpne spørsmål til Hallvar**
1. Skal skill-affixer bare rulle for mottakerens aktive spec (+ klassetab), eller for hele klassen?
2. Krav om at spilleren kjenner spellen ved rulling (upstream), eller også ulærte spells?
3. Hvilke typer i første omgang: kun server-autoritative (damage/DoT/crit/cast/duration), eller også CD/cost?
4. Maks antall skill-affixer per item og minste kvalitet (forslag: 1, blå+).
5. OK å legge én server-side carrier-spell i `spell_dbc` (world-DB, ingen klientendring)?
6. Skal originale klasser (1–11) og Hero få noe, eller kun CoA-klasser 12–32?
7. Summon-baserte spells: utelate, eller hook-basert skade for eierens summons senere?

**Beslutninger (Hallvar, 2026-11-23)**
1. Skill-affixer rulles for **hele klassen** (alle specs + klassetab), ikke bare aktiv spec.
2. Spilleren **må kjenne spellen** (en rank) når affixen rulles/reforges.
3. **Cooldown og cost er med fra start.** Teknisk: CD/cost/range bare i maske-modus (unik bit), ellers CD via
   `ModifySpellCooldown` etter cast (targeting `hook_cd`); cost i spell-id-modus tas ikke med.
4. Maks **1 skill-affix per item**, kun **blå+** (`min_quality >= 3`).
5. Én server-side carrier-spell i `spell_dbc` er OK.
6. **Kun CoA-klasser 12–32.** Ingen for originale klasser (1–11) eller Hero (94).
7. **Summon-baserte spells utelates.**
8. (2026-11-24) **Behold alle 705 affixer** foreløpig, ingen trimming til ~30 per klasse.
9. (2026-11-24) Necromancer `Command: Hook` og `Command: Bonefreeze` får `hook_cd`-affix (kun cooldown).
10. (2026-11-24) **DURATION** (`SPELLMOD_DURATION`, varighet på buff/debuff/DoT/HoT/skjold/CC): maks +25 % per affix
    (FLAT ≤ 25 % av base).
11. (2026-11-24) **Ingen stack-tak** på noen type: samme affix fra flere items summeres fritt (kostnaden er tapte
    stats/andre affixer i de slotene). Erstatter +100 %-taket i punkt 10. Kun teknisk gulv i koden: cooldown,
    cast-tid og cost klippes ved 0 (aldri negative verdier), ikke et balansetak.

**Faser**
1. **Fase 3a – infrastruktur**: carrier-aura, `affix_type 0`, mods på/av via `OnPlayerAfterApplyItemMods`, GM-kommando
   for å gi en bestemt skill-affix. Én manuell test-affix per op (damage, DoT, crit, cast, duration) på én klasse.
   Verifiser med `.spellinfo`, combat log og relogg/spec-bytte/død. Samtidig: fiks `.affix`-sikkerhetsnivå (`COA_PLAN.md` §9).
2. **Fase 3b – klienttest maske-modus**: CD/cost på én maske-unik spell; sjekk action bar-CD, tooltip og lokal blokkering.
   Avgjør om CD/cost tas med.
3. **Fase 3c – generator + reviewer-agenter**: kandidatfiler for 21 klasser, én reviewer per klasse, merge/validering,
   Hallvar godkjenner listene.
4. **Fase 3d – rulling/Reforge/addon**: pool-filter, `I`-state, tuning, deretter test-realm før live.
5. **Fase 4 (valgfri)**: summon-/pet-skade via hooks, proc/charges-typer.

## 8. Status implementering (2026-11-24, branch `coa`, uncommitted)

**Implementert** (ikke bygd, ikke applisert, ikke testet i spillet):
- `.affix`-sikkerhet (`COA_PLAN.md` §9): `info`/`spec`/`skills`/`reforge status` = `SEC_GAMEMASTER`,
  `reroll`/`give`/`reforge roll|pick` = `SEC_ADMINISTRATOR`. Nye: `.affix give <slot> <id|key> [crit]`, `.affix skills`.
- `tools/skill_affixes/merge_reviews.py` → `data/sql/db-world/mod_item_affixes_template_skill.sql` (707 affixer, 731
  target-rader). Stabile id-er i `tools/skill_affixes/affix_ids.json` (112000–132033). Filnavnet sorterer etter
  `mod_item_affixes_template.sql` (updater sorterer på filnavn). Carrier 9979001 (ledig i Spell.dbc og `spell_dbc`),
  kolonnemønster fra 9978010.
- C++: skill-affixer lastes ved oppstart (validering mot `sSpellMgr`; uten `affix_type`-kolonnen kjører modulen
  stat-only). Én carrier-aura per spiller; mods `new SpellModifier(carrier)`, eies og slettes av modulen; summeres per
  (spell/maske, op, type) over alle items; CD/cast/cost klippes ved 0. Mods fjernes før carrieren i
  `UnitScript::OnAuraRemove` og `OnPlayerLogout`; fjernet utenfra (RemoveAllAuras, `.unaura`) → gjenopprettes etter
  0,5 s via `m_Events`. Død beholder passive auraer.
- Targeting (**erstattet 2026-11-24**: kun `targetSpellId` + streamede rader, se nederst) – opprinnelig: `mask` = `mod->spellId` = target_spell + unik bit; `spell_id` = carrier, mask 0, `targetSpellId` per id
  (ingen global hook, `OnSpellModFamilyMask` ikke brukt); `hook_cd` = `AllSpellScript::OnSpellCast` (etter
  `SendSpellCooldown`) → `ModifySpellCooldown` på castens ferske CD-entry (< 1 s gammel) og på ferske entries med
  samme category når spellen har `CategoryRecoveryTime`; hopper over item-cast, `TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD`
  og `IsCooldownStartedOnEvent`. Global atomic teller gir early-out når ingen spiller har CD-kutt.
- Rulling/Reforge: `ItemAffixes.SkillAffixChance` (20), `ItemAffixes.SkillAffixes.Enable` (1). `rolled_value` = 100/150
  (prosent av malverdiene). Addon: state `I` → grått "(inactive)"; oppdateres 1 s etter learn/forget spell.

**Verifisert:** alle `.cpp` `-fsyntax-only` uten warnings mot live-byggets flagg (compile_commands.json, game-PCH);
`validate_reviews.py` OK; SQL-kolonner mot `DESCRIBE`; `DESCRIBE INSERT/DELETE` for `spell_dbc`-raden; `codestyle-sql.py`
og `codestyle-cpp.py` OK; merge gir identisk fil ved ny kjøring.

**Ikke verifisert:** full build/link, SQL applisert av updateren, oppstart, Lua-syntaks (ingen Lua-parser), og alt i
spillet: carrier/`ownerAura`-livssyklus (logout, død, `RemoveAllAuras`, prestige, spec-bytte), om Ascension-klienten
bruker spellmod-pakker for CD/cost i maske-modus, `SMSG_MODIFY_COOLDOWN` på action bar (også category), at mods treffer
riktige (trigger-)spells, tooltip-lengde ≤ 255 med lange navn.

**Oppdatert 2026-11-24 (streaming eneste modus):** klienttest A/B i `COA_SKILL_TOOLTIP_PLAN.md` bestått.
`ItemAffixes.SkillAffixes.StreamSpellRows` er fjernet (står nøkkelen i conf: `LOG_WARN` ved oppstart, ignoreres).
Maske-stien og `hook_cd` (`AllSpellScript::OnSpellCast`, `ModifySpellCooldown`, atomic teller) er fjernet; alle
affixer er `SpellModifier` på carrieren med `targetSpellId` på hver `affix_skill_target`-spell og alle ranks av den
(`spell_ranks`; tidligere bare for maske-affixer), cooldown via `SPELLMOD_COOLDOWN`. `targeting`/`mask0..2` står
fortsatt i `affix_template` (generert SQL uendret), men leses ikke. `validate_reviews.py` sjekker ikke lenger maske-bits
eller COST/RANGE/COOLDOWN-mot-targeting; `merge_reviews.py` avviser ikke lenger maske-mål som er spesialtilfeller i
`SpellInfo::IsAffectedBySpellMod`. `ItemAffixes.SkillAffixChance` default i conf.dist er 20 (live bruker 40).
