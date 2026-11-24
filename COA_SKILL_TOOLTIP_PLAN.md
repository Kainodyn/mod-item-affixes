# CoA skill affixes: riktig spell-info in-game

Status 2026-11-24: §6 A implementert i kode bak config (default av), **ikke bygget, ikke klienttestet** – se
"Status og klienttest" nederst. §1–5 er kartleggingen. [V] = verifisert i kode/data, [A] = antatt (krever klienttest).
Grunnlag: modul `coa` @ `0f1f034`, core `c9be83a58f`, server-DBC `/opt/coa/server-data/dbc` (sha256 lik
installasjonsmanifestet `install-manifest-server-0.261116.112.json`).

## 1. Hvordan klienten får spell-data og tooltip-tekst

- Statiske Spell-rader ligger i klientens MPQ-er; `patch-ZG.MPQ` vinner for `Spell.dbc` (`docs/coa/client-data.md:13-19`,
  ADR 1012). Serveren streamer ikke lenger statiske rader ved login (PR #12775). [V]
- **Per-spiller / runtime-rader sendes fortsatt**: `SMSG_PATCH_SPELL` = `0x092A` (`src/server/coa/AscensionCompat.cpp:217`),
  "Rows that differ per player or over time cannot be baked and are still sent" (`client-data.md:28-31`). [V]
  - Format (`BuildSpellRow`, `AscensionCompat.cpp:4669-4677`): 170 × uint32 (`SPELL_CLIENT_RECORD_DWORDS`, alle
    234 DBC-felt med de fire lokaliserte strengene kollapset til én slot hver, `SPELL_WIRE_SLOT` l. 4336-4341), deretter
    4 strenger som `uint32 len + bytes`: Name (felt 136), Description (170), Rank (153), ToolTip (187) (l. 4301-4312).
  - Kilde for raden: `ClientSpellRow(spellId)` (l. 4524-4545): forberedt rad (Spell-store + `spell_dbc` +
    `coa_client_spell_description`) eller rå rad fra serverens `Spell.dbc` (`ReadClientSpellRow`, `IndexClientSpells`
    l. 4547+). Altså kan **enhver** spell bygges som komplett rad. [V]
  - Brukes midt i økten i produksjon: `SendSpellNoticeRow` (setter `SPELL_ATTR0_DO_NOT_DISPLAY`, l. 4139-4151) og
    `SendSwapPlacementRow` (endrer Rank-tekst, l. 4153-4167), kalt fra player-hooks (l. 8818-8828). Core endrer også
    tall-felt i streamede rader, f.eks. `CastingTimeIndex` til en eksisterende `SpellCastTimes`-rad
    (`RangedPreparationCastTimeIndex`, l. 4751/4815). [V]
  - Gating: `CoA.SendDisplayPatches = 1` (`/opt/coa/etc/modules/coa.conf:286`), ikke bots (`ReceivesPatchRows` l. 4264). [V]
  - `coa_client_spell_description` (ID, Description, ToolTip) overstyrer tekst i den forberedte raden
    (`LoadClientSpellDescriptions` l. 5034; `WorldDatabase.cpp:164`) – global, ikke per spiller. [V]
- Ikke offentlig API: alt ligger i `AscensionDisplayPatchService` (anonym klasse i `AscensionCompat.cpp:4082`). Eneste
  eksport er `AscensionClientFileSpellRows()` (`AscensionClientSpellRows.h:8`). Modulen trenger en liten core-patch. [V]
- At klientens tooltip/cooldown/cost-sjekk leser den re-streamede raden umiddelbart: [A] – svært sannsynlig (raden *er*
  klientens spell-record; notice-raden endrer synlighet midt i økten), men må klienttestes.

**Konklusjon:** serveren kan sende en per-spiller modifisert Spell-rad (RecoveryTime, ManaCost, CastingTimeIndex,
RangeIndex, DurationIndex, EffectBasePoints, Description …) hver gang affixene endres. Ingen MPQ nødvendig.

## 2. Bruker klienten SMSG_SET_FLAT/PCT_SPELL_MODIFIER?

- Server sender pakkene kun for bitene i `mod->mask`, Ascension-layout `mode=0, eff, op, value, SpellFamilyName`;
  klienten slår opp `SpellFamilyName * 0x11A0 + eff * 31 + op` (`Player.cpp:10694-10770`, commit `e1544b899e`,
  som begrunnes med at mods "not found by the client" før familien ble sendt). [V for serverside, A for klientbruk]
- Cooldown: for spillere sender serveren **ingen** `SMSG_SPELL_COOLDOWN` ved vanlig cast; bare ved
  `SPELL_AURA_MOD_COOLDOWN` (`Player.cpp:11816-11876`, `needsCooldownPacket`). Klienten regner altså action bar-CD selv
  fra sin record + mottatte spellmods. [V for server; stock 3.3.5-oppførsel]. Derfor: spell_id/hook_cd-mods
  (`mask=0`) er usynlige for klienten; `hook_cd` korrigerer i etterkant med `SMSG_MODIFY_COOLDOWN` (`Player.cpp:11986`).
- Tooltip-tall ($s1/$d/$o) med spellmods i 3.3.5: [A] – kjent fra retail at cost/cast/cooldown-linjer reflekterer
  talenter, mens $s/$o i beskrivelsen i WotLK stort sett viser DBC-verdier. Ikke funnet bevis i core/docs.
- Uansett: maske-modus forutsetter unik bit, og bitene er brukt opp (§3).

## 3. Unike family-bits via klientpatch: ikke gjennomførbart

Script: `tools/skill_affixes/analyze_free_bits.py [--json out]` (leser Spell.dbc + `spell_dbc` (hel rad),
`spell_proc`, `spell_proc_event`, modulens maske-affixer og C++-literaler ved `SpellFamilyFlags/SpellClassMask/flag96/HasFlag`).

| fam | free | free* | recl* | sett som trenger bit | spells |
|---|---|---|---|---|---|
| 18 Barbarian | 2 | 6 | 18 | 18 | 18 |
| 19–23 | 0–2 | 0–6 | 10–22 | 9–28 | 10–30 |
| 24 Pyromancer | 1 | 7 | 25 | 19 | 20 |
| 25–38 | 0–1 | 0–3 | 1–14 | 13–37 | 13–39 |
| **Sum** | | | | **453** | **475** |

free = ingen bruk i flags/EffectSpellClassMask/proc/C++; free* = ignorerer C++-literaler uten kjent familie;
recl* = bit kun brukt i SpellFamilyFlags (ingen maske refererer den) og kan i teorien strippes og gjenbrukes.
0/21 familier holder konservativt, 4/21 med free*+recl*. 12 av 21 familier har alle 96 bitene i bruk i
SpellFamilyFlags alene. 88 spell_id/hook_cd-affixer har allerede en unik bit (generatorens sjekk var strengere).
Risiko ved ny bit: masker som inneholder biten og ikke traff spellen fra før begynner å treffe (talenter, procs,
klient-spellmods); spells med 0 flags (520–1010 per familie) blir i tillegg truffet av wildcard-masker (0–4 per familie).
Server må få samme rad (`spell_dbc` = hel rad, eller ny server-`Spell.dbc`), og en egen MPQ må outranke
`patch-ZG.MPQ` med *hele* `Spell.dbc` og bygges på nytt ved hver CoA-klientoppdatering. Ingen MPQ-verktøy
(`mpqcli`) eller klientdata lokalt; `apps/coa-dbc/client_dbc.py extract|drift` forutsetter en klientinstallasjon.

## 4. Klientpatch-distribusjon (om nødvendig senere)

- Pipeline i core: `apps/coa-dbc/client_dbc.py extract/diff/drift`, editorer i `apps/coa-spells/*.py`, pakking med
  `mpqcli` (`client-data.md` §"Adding or changing a client row"). CDN-publisering skjer utenfor repoet. [V]
- Server-`Spell.dbc` = serverpakkens (sha256 match). Klient-DBC kan ikke sammenlignes lokalt (ingen MPQ funnet). [V]
- Vi eier ikke CoA-CDN-en; venner måtte installere en egen `patch-?.MPQ` manuelt. Ikke anbefalt for dette.

## 5. Addon-supplement

- Hook `GameTooltip:SetSpell(id, book)` (→ `GetSpellLink(id, book)`), `SetAction(slot)` (→ `GetActionInfo`),
  `SetHyperlink("spell:…")` og evt. Ascensions CharacterAdvancement-tooltip med `hooksecurefunc`, legg til linjer
  "Affixes: -1.5 sec cooldown". Data via eksisterende `AFXM`-protokoll (ny melding `SPELLAFX|<spellId>|<tekst>`).
- Begrensning: kun ekstra linjer; fikser ikke tall i tooltipen, action bar-sweep, cost/range-sjekk; krever addon.
  Overflødig hvis §6 fungerer (affix-linja kan ligge i den streamede Description).

## 6. Anbefaling

**A. Per-spiller streamede Spell-rader (hovedløsning).**
1. Core-patch (lokal, `/home/coa/namdalnode/patches/`): eksporter
   `void AscensionSendPlayerSpellRow(Player*, uint32 spellId)` og en registry/hook
   `ClientSpellRowOverride(Player*, spellId, Values&, Strings&)` som **alle** sendere (notice/swap/ny) kaller, så
   temporære swaps ikke overskriver affix-verdiene.
2. Modul: alle affixer blir server-mods med `targetSpellId` (dagens spell_id-modus, også for COOLDOWN/COST/RANGE);
   ingen klient-spellmod-pakker fra affixer → ingen dobbelttelling. `hook_cd` erstattes av `SPELLMOD_COOLDOWN`
   (+ category). Maske-modus kan fases ut.
3. Etter `RebuildSkillMods` (og ved login etter display-patches): for hver berørt spell bygg rad fra
   `ClientSpellRow`, sett `RecoveryTime`/`CategoryRecoveryTime`, `ManaCost`/`ManaCostPercentage`,
   `EffectBasePoints` (EFFECTn/ALL_EFFECTS), `EffectChainTarget` (jump), og nærmeste eksisterende
   `CastingTimeIndex`/`RangeIndex`/`DurationIndex`/`EffectRadiusIndex` (71/323/866/318 rader); legg affix-linje
   (`|cff1eff00Affixes: …|r`) til Description. Server regner verdien slik at klientens egne talent-spellmods på
   toppen gir samme sluttverdi som serveren. Ved fjerning: send original rad.
4. Gir: riktig spellbook/action bar-tooltip (cooldown/cost/cast/range-linjer, $s/$d der feltet endres), riktig
   action bar-sweep og lokal cost/range-sjekk; affix-linje for damage/crit/DoT (ingen felt). [A til klienttest]
5. Arbeid: core-patch ~100 linjer, modul ~250, tester. Risiko: lav–middels (re-stream mid-session, index-avrunding
   for cast/range/duration/radius; range under/over server = "out of range"-mismatch → velg verdier som finnes).

**B. Addon** som fallback/debug (§5). **C. MPQ/unike bits:** droppes (§3).

**Rekkefølge:** (0) klienttest uten kode: `.affix give` + manuelt sendt rad via GM-kommando bak core-patch, sjekk
tooltip, sweep, cost; (1) core-patch + `.affix`-debug; (2) modul-overgang til targetSpellId + rad-overrides;
(3) fjern `hook_cd`/maske-sti; (4) addon-linjer kun hvis (0) viser at Description ikke oppdateres.

## Åpne spørsmål til Hallvar
1. OK med lokal core-patch i `AscensionCompat.cpp` (eksport + override-hook), vedlikeholdt i `patches/` ved hver live-tag?
2. Godta avrunding til eksisterende cast/range/duration/radius-rader, eller justere affix-verdiene til radene?
3. Fase ut maske-modus (1014 affixer) for én mekanisme, eller beholde den til klienttest viser at den virker?

## Beslutninger og avrunding (Hallvar, 2026-11-24)
1. Lokal core-patch i `AscensionCompat.cpp` (eksportert "send Spell-rad for én spiller" + override-hook) er OK; vedlikeholdes i `/home/coa/namdalnode/patches/`.
2. Avrunding til eksisterende rader i SpellCastTimes/SpellDuration/SpellRange/SpellRadius: undersøkt med
   `tools/skill_affixes/analyze_index_rounding.py`. 120 affixer er justert (FLAT-verdi som treffer en eksisterende rad,
   innen tak og 0,5–1,5× opprinnelig bonus; navn og `notes` oppdatert). Etter justering, én kopi (x1):
   cast 72/75 eksakt, duration 347/370, range 128/130, radius 193/206; resten bommer mest på svært små bonuser
   (f.eks. +0,5 yd på 5 yd radius). Stacket (x2/x3) bommer > 90 % med ≤ 10 % av bonusen.
   Bom gjelder bare klientvisning/klientsjekk; serveren regner eksakt med spellmods.
2b. (2026-11-24, Hallvar: "få alle til å treffe") Kravet om 0,5–1,5× er fjernet: nærmeste verdi som treffer en rad
    eksakt innen validator-takene (32 til + 2 som PCT). Én kopi (x1): cast 73/75, duration 370/370, range 130/130,
    radius 205/206 (Crush +7 % = 8,025 yd mot rad 8,0). Keleseth/Valanar cast satt til −1 s (rad 5000 ms; Hallvar godkjente over 500 ms-taket; validator: maks(500 ms, 17 % av cast)).
3. Maske-modus beholdes til den streamede rad-metoden er klienttestet.

## Status og klienttest (2026-11-24)

**Implementert, ikke verifisert** (kun `-fsyntax-only`, codestyle og `validate_reviews.py`; ikke bygget, ikke
klienttestet).

- Core-patch `/home/coa/namdalnode/patches/core-ascension-spell-row-api.patch` (`AscensionClientSpellRows.h`,
  `AscensionCompat.cpp`): `AscensionSpellRow` (170 dword-slots + Name/Description/Rank/ToolTip, `Get/Set(dbcField)`
  med Spell.dbc-kolonnenummer), `AscensionOriginalSpellRow(spellId)`, `AscensionSendPlayerSpellRow(player, spellId,
  edit)` og `AscensionSetSpellRowOverride(fn)`. Alle per-spiller Spell-rader (notice, swap, ny) går gjennom
  `SendSpellRow`, som kaller override-hooken før sending. Ingen hook registrert = uendret atferd.
- Modul: `.affix row` (admin) og `ItemAffixes.SkillAffixes.StreamSpellRows` (default 0). Med 0 registreres ingen
  hook og atferden er som før (maske + `hook_cd`).
- Server-verdi vs klient-rad: raden viser `base` endret av affixene alene (samme formel som
  `Player::ApplySpellMod`); klientens egne talent-spellmods kommer på toppen. Kombinasjon flat-talent + pct-affix
  kan avvike litt. Flat-cost på spells med `ManaCostPercentage` vises ikke (bare pct-cost).

### Testprosedyre for Hallvar (Necromancer)
Forutsetning: core-patch applisert, modulendringene i `/opt/coa/azerothcore/modules/mod-item-affixes`, bygget og
restartet; `CoA.SendDisplayPatches = 1`. GM-konto med admin.

**A. Råtest uten streaming (`StreamSpellRows = 0`)** – virker en rad sendt midt i økten?
1. Logg inn Necromancer som kan Glacial Tap (805369; 12 s CD, 40 % kostnad). Legg den på action bar.
2. `.affix row 805369 RecoveryTime=6000 desc="TEST"` → spellbook- og action bar-tooltip skal vise 6 sec cooldown og
   beskrivelse "TEST". Cast: sweep skal gå 6 s (serveren har fortsatt 12 s, så tidlig recast gir "not ready" –
   forventet i testen).
3. `.affix row 805369 ManaCostPercentage=20` → kostnadslinja i tooltip; sjekk at knappen ikke gråes feil.
4. `.affix row 801760 CastingTimeIndex=4 RangeIndex=5` (Ice Barrage: 2,0 s/30 yd → 1 s/40 yd) → tooltip-linjene;
   sjekk range-farging på action bar mellom 30 og 40 yd (serveren avviser > 30 yd – forventet).
5. `.affix row 805369 EffectBasePoints0=499` → `$s1` i tooltip skal vise 500 (DBC lagrer verdi − 1).
6. `.affix row 805369 reset` og `.affix row 801760 reset` → alt tilbake.
7. Noter for hvert punkt: tooltip (spellbook/action bar), sweep, cost-/range-sjekk, om det krever /reload eller
   åpning av spellbook på nytt.

**B. Streaming (`ItemAffixes.SkillAffixes.StreamSpellRows = 1`, restart)** – kun hvis A viser at radene følges.
1. `.affix give <slot> necro_glacial_tap_cd` (‑1,5 s). `.affix skills` skal vise `row spell 805369: op11-1500 … sent`
   og ingen `mask`-mod (mod-linjene har `mask 0x0:0x0:0x0`).
2. Tooltip: 10.5 sec; cast: sweep 10,5 s og serverens CD 10,5 s (recast etter 10,5 s skal lykkes, ikke tidligere).
3. `.affix give <slot> necro_glacial_tap_rp` (RP +%) → beskrivelsen får grønn linje "Affixes: …".
4. Ta av item → original rad (12 sec). Ta på igjen → 10.5 sec.
5. Relogg → etter ca. 5 s skal riktige rader være på plass. Bytt spec (dual spec) → samme.
6. Sjekk at gamle `hook_cd`-affixer (targeting 2) gir riktig CD uten etterkorrigering, og at ingen verdi telles
   dobbelt (f.eks. kostnad/CD halvert to ganger).
7. Avbrudd: sett `StreamSpellRows = 0` og restart; relogg gir originale rader (MPQ).

### Risikoer
- Klienten kan cache raden eller ignorere re-stream av en allerede lastet rad (test A avklarer).
- Rader ligger i klientminnet til relogg; ved spec-bytte/login sendes originaler for alle spells klassens
  skill-affixer kan treffe (ca. 100 rader, én gang).
- `hook_cd`→`SPELLMOD_COOLDOWN` virker ikke på spells som ignorerer caster-modifiers; mask→targetSpellId dekker alle
  ranks i `affix_skill_target`-målene, men ikke andre spells som masken tilfeldigvis traff.
- Core-patchen må re-appliseres ved hver live-tag (`patches/README.md`).

