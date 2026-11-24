# CoA class skill affixes: oversikt

Generert av `tools/skill_affixes/gen_overview.py` fra `tools/skill_affixes/out/reviews/*.json`.
Status: forslag (ikke i DB/build). Plan og beslutninger: `COA_SKILL_AFFIX_PLAN.md`.

Targeting: `mask` = unik family-bit (klient får spellmod), `spell_id` = server-side på spell-id,
`hook_cd` = cooldown kortes ned etter cast. Verdier er per affix før crit roll; ingen stack-tak.

## Sammendrag

| Klasse | Affixer | mask | spell_id | hook_cd | Avvist |
|---|---|---|---|---|---|
| [Barbarian](#12-barbarian) | 35 | 27 | 4 | 4 | 2 |
| [Witchdoctor](#13-witchdoctor) | 31 | 15 | 5 | 11 | 17 |
| [Demonhunter](#14-demonhunter) | 31 | 23 | 3 | 5 | 3 |
| [Witchhunter](#15-witchhunter) | 40 | 10 | 23 | 7 | 8 |
| [Stormbringer](#16-stormbringer) | 35 | 11 | 15 | 9 | 2 |
| [Fleshwarden](#17-fleshwarden) | 26 | 4 | 13 | 9 | 6 |
| [Guardian](#18-guardian) | 31 | 14 | 6 | 11 | 3 |
| [Monk](#19-monk) | 32 | 19 | 3 | 10 | 1 |
| [Sonofarugal](#20-sonofarugal) | 44 | 23 | 8 | 13 | 2 |
| [Ranger](#21-ranger) | 40 | 21 | 6 | 13 | 5 |
| [Chronomancer](#22-chronomancer) | 33 | 20 | 5 | 8 | 6 |
| [Necromancer](#23-necromancer) | 22 | 7 | 8 | 7 | 20 |
| [Pyromancer](#24-pyromancer) | 30 | 20 | 4 | 6 | 7 |
| [Cultist](#25-cultist) | 31 | 16 | 4 | 11 | 9 |
| [Starcaller](#26-starcaller) | 34 | 16 | 6 | 12 | 7 |
| [Suncleric](#27-suncleric) | 41 | 11 | 16 | 14 | 6 |
| [Tinker](#28-tinker) | 28 | 10 | 13 | 5 | 18 |
| [Prophet](#29-prophet) | 41 | 7 | 20 | 14 | 7 |
| [Reaper](#30-reaper) | 29 | 17 | 4 | 8 | 6 |
| [Wildwalker](#31-wildwalker) | 39 | 19 | 9 | 11 | 5 |
| [Spiritmage](#32-spiritmage) | 34 | 15 | 11 | 8 | 1 |
| **Totalt** | **707** | 325 | 186 | 196 | 141 |

## 12 BARBARIAN

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Ancestral Roar (500918) | Ancestral Roar: +10% healing | damage +10 % | mask | Blue | 100 | high |
| Born in Blood (560521) | Born in Blood: +10% periodic healing | dot +10 % | mask | Epic | 100 | high |
| Defiance (806228) | Defiance: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 100 | medium |
| Headhunter's Spear (804137) | Headhunter's Spear: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Hodir's Wrath (800152) | Hodir's Wrath: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Jawbreaker (802792) | Jawbreaker: -2 sec cooldown | cooldown -2 s | mask | Blue | 80 | high |
| Maiming Spear (804139) | Maiming Spear: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Outrage (801759) | Outrage: -20 sec cooldown | cooldown -20 s | mask | Epic | 100 | high |
| Skull Smash (560532) | Skull Smash: -15 sec cooldown | cooldown -15 s | mask | Blue | 60 | high |
| Thick Skull (801549) | Thick Skull: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | medium |
| Warband (560883) | Warband: -10 sec cooldown | cooldown -10 s | mask | Blue | 80 | high |

### Tactics

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Barbed Spear (560881) | Barbed Spear: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Berserker Axe (806960) | Berserker Axe: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Berserker Rush (560518) | Berserker Rush: -4 sec cooldown | cooldown -4 s | mask | Blue | 100 | high |
| Guts Spilled (520577) | Guts Spilled: +8% damage | damage +8 % | mask | Blue | 80 | high |
| Gutspiller (805832) | Gutspiller: +10% periodic damage | dot +10 % | mask | Blue | 100 | high |
| Impaling Spear (500984) | Impaling Spear: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Spite (520541) | Spite: +8% damage | damage +8 % | spell_id | Blue | 100 | low |

### Brutality

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Brutal Swing (500913) | Brutal Swing: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Crush (500915) | Crush: -0.2 sec cast time | casting time -0.2 s | mask | Blue | 80 | high |
| Crush (500915) | Crush: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Decapitate (804414) | Decapitate: +5% critical strike chance | critical chance +5 | mask | Epic | 80 | high |
| Decapitate (804414) | Decapitate: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Killing Spree (850022) | Killing Spree: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Storm of Steel (800628) | Storm of Steel: +8% damage | damage +8 % | mask | Blue | 100 | high |

### Ancestry

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Ale of The God-King (805780) | Ale of The God-King: -2 sec cooldown | cooldown -2 s | mask | Blue | 60 | medium |
| Ancestral Evolution (806229) | Ancestral Evolution: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 80 | medium |
| Breath of The North (800838) | Breath of The North: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Clanlord's Totem (804737) | Clanlord's Totem: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 80 | medium |
| Frozen Tankard (805815) | Frozen Tankard: -4 sec cooldown | cooldown -4 s | mask | Blue | 100 | high |
| Keg Smash (805808) | Keg Smash: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Ramhorn Rage (805804) | Ramhorn Rage: -15 sec cooldown | cooldown -15 s | mask | Blue | 80 | high |
| Splash Zone (705156) | Splash Zone: -4 sec cooldown | cooldown -4 s | mask | Blue | 80 | medium |
| Tavern Brawl! (804456) | Tavern Brawl!: -30 sec cooldown | cooldown -30 s | mask | Epic | 100 | high |
| War Cry (500995) | War Cry: -20 sec cooldown | cooldown -20 s | mask | Blue | 100 | high |

<details><summary>Avvist (2)</summary>

| Spell | Grunn |
|---|---|
| Axe Twirling (800401) | toggle passive aura (dur=-1, ADD_FLAT_MODIFIER/-15% damage); no meaningful op |
| Cheers! (805810) | utility link (3 sec cd, 1800 sec buff); no meaningful number to improve |

</details>

## 13 WITCHDOCTOR

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Amphibimorph (500952) | Amphibimorph: -15 sec cooldown | cooldown -15 s | mask | Blue | 60 | high |
| Slither (500947) | Slither: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 60 | medium |
| Veil of Darkness (802100) | Veil of Darkness: -10 sec cooldown | cooldown -10 s | mask | Blue | 80 | high |
| Veil of Darkness (806473) | Veil of Darkness: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Vol'jin's Vigil (504465) | Vol'jin's Vigil: -15 sec cooldown | cooldown -15 s | mask | Blue | 100 | high |

### Shadowhunting

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Frenzied Spirits (560748) | Frenzied Spirits: -3 sec cooldown | cooldown -3 s | mask | Blue | 100 | high |
| Reclamation (806288) | Reclamation: -0.15 sec cast time | casting time -0.15 s | mask | Blue | 80 | high |
| Reclamation (806288) | Reclamation: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Shadowstalker (807040) | Shadowstalker: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 80 | medium |
| Spirit Eclipse (802717) | Spirit Eclipse: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Spirit Glaive (806289) | Spirit Glaive: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Spirit Shock (807743) | Spirit Shock: -4 sec cooldown | cooldown -4 s | hook_cd | Blue | 80 | medium |

### Voodoo

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Bad Juju (802087) | Bad Juju: -0.25 sec cast time | casting time -0.25 s | spell_id | Blue | 80 | medium |
| Bad Juju (802087) | Bad Juju: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Hexfire (807042) | Hexfire: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Malignant Jinx (803678) | Malignant Jinx: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 60 | medium |
| Puppeteer's Grasp (707209) | Puppeteer's Grasp: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 80 | medium |
| Shadow Puppet (801797) | Shadow Puppets: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Soul Marionette (704497) | Soul Marionette: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 80 | low |
| Trapped Spirits (504452) | Trapped Spirits: +8% damage | damage +8 % | mask | Blue | 80 | high |
| Voice of Bwonsamdi (801716) | Voice of Bwonsamdi: -10 sec cooldown | cooldown -10 s | mask | Blue | 100 | high |

### Brewing

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Arcane Brew (801689) | Arcane Brew: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 60 | medium |
| Base: Beast Blood (705870) | Base: Beast Blood: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 80 | medium |
| Base: Crystal Water (500962) | Base: Crystal Water: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 80 | medium |
| Ingredient: Jungle Shrooms (802703) | Jungle Shrooms: +8% healing | damage +8 % | mask | Blue | 100 | high |
| Master Mixologist (705864) | Master Mixologist: -15 sec cooldown | cooldown -15 s | hook_cd | Epic | 80 | medium |
| Mojo Beam (500950) | Mojo Beam: -2 sec cooldown | cooldown -2 s | mask | Blue | 80 | high |
| Mojo Beam (500950) | Mojo Beam: +10% periodic healing | dot +10 % | mask | Blue | 100 | high |
| Potion Toss (801661) | Potion Toss: +8% healing | damage +8 % | mask | Blue | 100 | high |
| Rage Brew (503750) | Rage Brew: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 60 | medium |
| Splash Potion (802710) | Splash Potion: +8% healing | damage +8 % | spell_id | Blue | 100 | medium |

<details><summary>Avvist (17)</summary>

| Spell | Grunn |
|---|---|
| Call of Sseratus (681222) | summon (Serpent Wards via child 572899) |
| Cleansing Idol (504840) | summon (is_summon=self); damage/heal by creature |
| Cursed Effigy (706542) | summon (is_summon=self); damage/heal by creature |
| Death Draught (602220) | utility (feign death); no meaningful op |
| Fool's Play (681007) | summons a copy of the target via SCRIPT_EFFECT; no player-owned number |
| Healing Ward (500957) | summon (is_summon=self); damage/heal by creature |
| Ingredient: Bloodthistle (801664) | ingredient toggle (dur=-1), aura % buff; no meaningful op |
| Ingredient: Frog Bones (801663) | ingredient toggle (dur=-1), aura % buff; no meaningful op |
| Mimic Ward (707162) | summon (is_summon=self); damage/heal by creature |
| Mirage (501136) | summon (is_summon=self): leaves a copy behind |
| Mojo: Fish Bones (705850) | utility: resets Potion Toss/Splash Potion and swaps ingredients (choice talent) |
| Mojo: Frog Shrooms (705851) | utility: resets Potion Toss/Splash Potion and swaps ingredients (choice talent) |
| Mojo: Jungle Thistle (500472) | utility: resets Potion Toss/Splash Potion and swaps ingredients (choice talent) |
| Spirit Link Idol (706369) | summon (is_summon=self); damage/heal by creature |
| Swift Idol (804226) | summon (is_summon=self); damage/heal by creature |
| Voodoo Cauldron (804684) | summon (cauldron); periodic heal tied to summoned object |
| War Golem (800330) | summon (is_summon=self); damage/heal by creature |

</details>

## 14 DEMONHUNTER

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Annihilation (803904) | Annihilation: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | high |
| Felbane (525001) | Felbane: -4 sec cooldown | cooldown -4 s | mask | Blue | 60 | high |
| Hateforged Barrier (705129) | Hateforged Barrier: +10% absorb | effect1 +10 % | mask | Blue | 100 | high |
| Hateforged Barrier (705129) | Hateforged Barrier: -4 sec cooldown | cooldown -4 s | mask | Blue | 80 | high |
| Illidan's Guile (806109) | Illidan's Guile: -30 sec cooldown | cooldown -30 s | mask | Blue | 80 | high |
| Manaburn (805248) | Manaburn: -3 sec cooldown | cooldown -3 s | mask | Blue | 60 | high |
| Reckoning (802058) | Reckoning: -15 sec cooldown | cooldown -15 s | hook_cd | Epic | 100 | medium |
| Skull of Gul'dan (800225) | Skull of Gul'dan: -10 sec cooldown | cooldown -10 s | mask | Blue | 80 | high |
| Whispers of the Pit (805235) | Whispers of the Pit: -8 sec cooldown | cooldown -8 s | mask | Blue | 80 | high |

### Felblood

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Bane of Chaos (704368) | Bane of Chaos: +10% periodic damage | dot +10 % | spell_id | Blue | 100 | medium |
| Felbreak (800203) | Felbreak: -2.5 sec cooldown | cooldown -2.5 s | mask | Blue | 80 | high |
| Felwrath (520236) | Felwrath: -0.15 sec cast time | casting time -0.15 s | mask | Blue | 80 | high |
| Felwrath (520236) | Felwrath: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Ruin (801895) | Ruin: -0.25 sec cast time | casting time -0.25 s | mask | Blue | 80 | high |
| Ruin (801895) | Ruin: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Sargeras Embrace (800355) | Sargeras Embrace: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 80 | medium |
| Sargeras Embrace (800355) | Sargeras Embrace: +10% periodic damage | dot +10 % | spell_id | Blue | 100 | medium |

### Slaying

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Annihilan Strike (801913) | Annihilan Strike: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Azzinoth's Assault (520262) | Azzinoth's Assault: +5% critical strike chance | critical chance +5 | mask | Epic | 60 | high |
| Azzinoth's Assault (520262) | Azzinoth's Assault: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Felhoof Charge (800204) | Felhoof Charge: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Fury Unleashed (555738) | Fury Unleashed: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 80 | medium |
| Fury Unleashed (555739) | Fury Unleashed: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Immolation (800208) | Immolation Aura: +8% damage | damage +8 % | mask | Blue | 100 | high |

### Demonology

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Burning Hatred (805239) | Burning Hatred: -15 sec cooldown | cooldown -15 s | mask | Epic | 100 | high |
| Demonic Will (800209) | Demonic Will: -6 sec cooldown | cooldown -6 s | mask | Blue | 100 | high |
| Felrend (800210) | Felrend: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Infernal Whipcrack (805243) | Infernal Whipcrack: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 80 | medium |
| Tyrannical Resolve (804823) | Tyrannical Resolve: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 100 | medium |
| Tyrant's Gaze (805240) | Tyrant's Gaze: -1.5 sec cooldown | cooldown -1.5 s | mask | Blue | 80 | high |
| Tyrant's Gaze (805240) | Tyrant's Gaze: +8% damage | damage +8 % | mask | Blue | 100 | high |

<details><summary>Avvist (3)</summary>

| Spell | Grunn |
|---|---|
| Carve (800206) | channel damage done by script (no damage effect in visible children); no meaningful op |
| Fury of the Illidari (561216) | damage done by script (PERIODIC_DUMMY), charges without cd field; no targetable damage spell |
| Infernal (560284) | summon (Infernal; is_summon=self) |

</details>

## 15 WITCHHUNTER

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Brand of the Unworthy (501380) | Brand of the Unworthy: -6 sec cooldown | cooldown -6 s | hook_cd | Blue | 50 | high |
| Burrow Bolt (802269) | Burrow Bolt: -4 sec cooldown | cooldown -4 s | mask | Blue | 80 | high |
| Slayer's Mark (804068) | Slayer's Mark: -9 sec cooldown | cooldown -9 s | hook_cd | Blue | 100 | high |
| Smoke Grenade (805756) | Smoke Grenade: -12 sec cooldown | cooldown -12 s | mask | Blue | 70 | high |
| Vampiric Tonic (802276) | Vampiric Tonic: -12 sec cooldown | cooldown -12 s | hook_cd | Blue | 100 | high |
| Vault (500085) | Vault: -1.5 sec cooldown | cooldown -1.5 s | mask | Blue | 80 | high |
| Witchblood Tonic (802278) | Witchblood Tonic: -12 sec cooldown | cooldown -12 s | mask | Blue | 100 | high |
| Witching Shroud (805770) | Witching Shroud: -9 sec cooldown | cooldown -9 s | mask | Blue | 60 | high |

### Boltslinger

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Bolt and Dash (520670) | Bolt and Dash: -4.5 sec cooldown | cooldown -4.5 s | hook_cd | Blue | 80 | high |
| Damnation (520062) | Damnation: +3% critical strike chance | critical chance +3 | spell_id | Blue | 80 | high |
| Damnation (520062) | Damnation: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Daring Escape (500086) | Daring Escape: -3 sec cooldown | cooldown -3 s | mask | Blue | 80 | high |
| Heartseeking Bolt (805754) | Heartseeking Bolt: -0.3 sec cast time | casting time -0.3 s | spell_id | Blue | 100 | high |
| Heartseeking Bolt (805754) | Heartseeking Bolt: +10% damage | damage +10 % | spell_id | Epic | 100 | high |
| Tormentor (800162) | Tormentor: +8% damage | damage +8 %; dot +8 % | spell_id | Blue | 100 | high |
| Twilight Frenzy (805751) | Twilight Frenzy: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Witchbane (800165) | Witchbane: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |

### Darkness

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Darkflock (520270) | Darkflock: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Houndmaster's Call (802273) | Houndmaster's Call: -2 sec cooldown | cooldown -2 s | mask | Blue | 70 | high |
| Quickdraw (804193) | Quickdraw: +3% critical strike chance | critical chance +3 | spell_id | Blue | 80 | high |
| Quickdraw (804193) | Quickdraw: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Rearmament (805738) | Rearmament: -30 sec cooldown | cooldown -30 s | hook_cd | Blue | 70 | high |
| Repulse (680236) | Repulse: +8% damage | damage +8 % | spell_id | Blue | 100 | high |

### Inquisition

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Darkslayer's Lantern (802139) | Darkslayer's Lantern: -12 sec cooldown | cooldown -12 s | mask | Blue | 100 | high |
| Dusk Blade (802020) | Dusk Blade: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Fiery Judgement (802140) | Fiery Judgement: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Flourish (805365) | Flourish: -6 sec cooldown | cooldown -6 s | hook_cd | Blue | 100 | high |
| Purifier's Edge (802019) | Purifier's Edge: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Pyro Tonic (802277) | Pyro Tonic: -12 sec cooldown | cooldown -12 s | mask | Blue | 100 | high |
| Smite Evil (802004) | Smite Evil: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Stake (680537) | Stake: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Torchlight (503662) | Torchlight: +8% damage | damage +8 % | spell_id | Blue | 100 | high |

### Witchknight

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Chains of Darkness (680521) | Chains of Darkness: -18 sec cooldown | cooldown -18 s | hook_cd | Blue | 70 | high |
| Desecrate (680518) | Desecrate: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Gaze of the Black Knight (802138) | Gaze of the Black Knight: +8% absorb | effect3 +8 % | spell_id | Blue | 80 | medium |
| Gaze of the Black Knight (802138) | Gaze of the Black Knight: -12 sec cooldown | cooldown -12 s | mask | Blue | 100 | high |
| Knight's Seal (680498) | Knight's Seal: +8% periodic damage | dot +8 % | spell_id | Blue | 100 | high |
| March of the Black King (681095) | March of the Black King: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Noctis Blade (802280) | Noctis Blade: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Witchblight (680494) | Witchblight: +8% damage | damage +8 % | spell_id | Blue | 100 | high |

<details><summary>Avvist (8)</summary>

| Spell | Grunn |
|---|---|
| Death Trap (681450) | summon (Death Trap, SUMMON effect) |
| Decimate (804194) | is_summon=self; buffs hounds (ASCENSION_A_TO_SUMMONS) |
| Houndmaster's Whistle (801343) | summon pet |
| Hunt (802006) | mobility charge; charges-based recharge, no cd field, no meaningful op |
| Inquisitor's Trap (681445) | summon (trap creature) |
| Shadow Trap (680238) | summon (trap creature) |
| Unleash the Hounds (807918) | summon (Lesser Shadow Hound) |
| Witching Idol (804051) | summon (Witching Idol) |

</details>

## 16 STORMBRINGER

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Body of Lightning (500041) | Body of Lightning: -9 sec cooldown | cooldown -9 s | mask | Blue | 100 | high |
| Charge (804826) | Charge: -6 sec cooldown | cooldown -6 s | hook_cd | Blue | 100 | high |
| Cloudburst (801838) | Cloudburst: -3.5 sec cooldown | cooldown -3.5 s | mask | Blue | 60 | high |
| Exhale (705672) | Exhale: -9 sec cooldown | cooldown -9 s | mask | Blue | 50 | high |
| Eye of the Storm (806307) | Eye of the Storm: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Fog (806100) | Fog: -9 sec cooldown | cooldown -9 s | hook_cd | Blue | 60 | high |
| Invigorating Winds (705661) | Invigorating Winds: -12 sec cooldown | cooldown -12 s | mask | Blue | 60 | high |
| Storm Ascendance (681110) | Storm Ascendance: -12 sec cooldown | cooldown -12 s | mask | Blue | 100 | high |
| Storm Ascendance (681110) | Storm Ascendance: +1.5 sec duration | duration +1.5 s | spell_id | Epic | 80 | high |
| Stormcloak (804833) | Stormcloak: -3 sec cooldown | cooldown -3 s | mask | Blue | 80 | high |
| Thunder King (804591) | Thunder King: -18 sec cooldown | cooldown -18 s | hook_cd | Blue | 100 | high |

### Wind

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Aerodynamics (806124) | Aerodynamics: +1.5 sec duration | duration +1.5 s | spell_id | Blue | 60 | high |
| Flurry (707543) | Flurry: -1 sec cooldown | cooldown -1 s | hook_cd | Blue | 80 | medium |
| Gale (804036) | Gale: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Kiss of the Clouds (500039) | Kiss of the Clouds: +8% absorb | effect1 +8 % | spell_id | Blue | 100 | medium |
| Nimbus (704212) | Nimbus: -30 sec cooldown | cooldown -30 s | hook_cd | Blue | 50 | high |
| Skyfall (804830) | Skyfall: -9 sec cooldown | cooldown -9 s | mask | Blue | 100 | high |
| Surge of Might (520083) | Surge of Might: -6 sec cooldown | cooldown -6 s | hook_cd | Blue | 80 | high |
| Tailwind (804035) | Tailwind: +1.5 sec duration | duration +1.5 s | spell_id | Blue | 80 | high |
| Tempest's Call (560567) | Tempest's Call: -30 sec cooldown | cooldown -30 s | hook_cd | Epic | 70 | high |
| Unshackle (706625) | Unshackle: -12 sec cooldown | cooldown -12 s | mask | Blue | 100 | high |
| Windsurf (500042) | Windsurf: -9 sec cooldown | cooldown -9 s | mask | Blue | 80 | high |

### Gifts

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Deluge (806400) | Deluge: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Drown (806407) | Drown: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Mystic Thunder (504846) | Mystic Thunder: -3 sec cooldown | cooldown -3 s | hook_cd | Blue | 80 | medium |
| Stormcloud (801859) | Stormcloud: -6 sec cooldown | cooldown -6 s | mask | Blue | 100 | high |
| Stormflow (567555) | Stormflow: +8% periodic damage | dot +8 % | spell_id | Blue | 100 | high |
| Stormfury (532751) | Stormfury: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Thunder Ward (800098) | Thunder Ward: -30 sec cooldown | cooldown -30 s | hook_cd | Blue | 70 | high |

### Lightning

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Arm of Thorim (801847) | Arm of Thorim: +10% damage | damage +10 % | spell_id | Epic | 100 | high |
| Forked Lightning (801851) | Forked Lightning: -0.2 sec cast time | casting time -0.2 s | spell_id | Blue | 100 | high |
| Forked Lightning (801851) | Forked Lightning: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Lightning Cage (560030) | Lightning Cage: -18 sec cooldown | cooldown -18 s | mask | Blue | 100 | high |
| Ride the Lightning (800099) | Ride the Lightning: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Volt (500928) | Volt: +8% periodic damage | dot +8 % | spell_id | Blue | 100 | high |

<details><summary>Avvist (2)</summary>

| Spell | Grunn |
|---|---|
| Lexicon of Servitude (520087) | summon support only (next Summon: Air Elemental instant/free); summons excluded |
| Wind Gate (504401) | summon (Wind Gate) |

</details>

## 17 FLESHWARDEN

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Black Shield (805679) | Black Shield: -12 sec cooldown | cooldown -12 s | mask | Blue | 100 | high |
| Burning Rage (520295) | Burning Rage: -6 sec cooldown | cooldown -6 s | hook_cd | Blue | 70 | high |
| Chains of Malice (803185) | Chains of Malice: -9 sec cooldown | cooldown -9 s | hook_cd | Blue | 80 | high |
| Chainwhip (800081) | Chainwhip: -2 sec cooldown | cooldown -2 s | mask | Blue | 80 | high |
| Demon Heart (805669) | Demon Heart: -6 sec cooldown | cooldown -6 s | hook_cd | Blue | 100 | high |
| Juggernaut (520294) | Juggernaut: -12 sec cooldown | cooldown -12 s | mask | Blue | 100 | high |
| Juggernaut (520294) | Juggernaut: +8% periodic healing | dot +8 % | spell_id | Blue | 70 | high |
| Suffuse (801063) | Suffuse: +8% periodic healing | dot +8 % | spell_id | Blue | 100 | high |
| Xorothian Sigil (801061) | Xorothian Sigil: -6 sec cooldown | cooldown -6 s | mask | Blue | 70 | high |

### Hellfire

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Doom (802602) | Doom: -6 sec cooldown | cooldown -6 s | hook_cd | Blue | 70 | high |
| Hellfire Form (805696) | Hellfire Form: -12 sec cooldown | cooldown -12 s | hook_cd | Blue | 100 | high |
| Hellfire Form (805696) | Hellfire Form: +1.5 sec duration | duration +1.5 s | spell_id | Epic | 80 | high |
| Hellmaw (806965) | Hellmaw: +8% damage | damage +8 %; dot +8 % | spell_id | Blue | 100 | high |
| Hellstorm (802342) | Hellstorm: -3 sec cooldown | cooldown -3 s | hook_cd | Blue | 80 | high |
| Hellstorm (802342) | Hellstorm: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Pestilence of Apocalypse (804786) | Pestilence of Apocalypse: +8% Unleash damage | damage +8 %; dot +8 % | spell_id | Blue | 80 | medium |
| Seeking Flame (805671) | Seeking Flame: +3% critical strike chance | critical chance +3 | spell_id | Blue | 80 | high |
| Seeking Flame (805671) | Seeking Flame: +8% damage | damage +8 % | spell_id | Blue | 100 | high |

### Defiance

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Curse of Xoroth (804169) | Curse of Xoroth: -9 sec cooldown | cooldown -9 s | hook_cd | Blue | 80 | high |
| Curse of Xoroth (804169) | Curse of Xoroth: +8% periodic damage | dot +8 % | spell_id | Blue | 100 | high |
| Sacrificial Circle (805677) | Sacrificial Circle: -6 sec cooldown | cooldown -6 s | hook_cd | Blue | 70 | high |

### War

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Burning Blade (524920) | Burning Blade: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Decimation (524913) | Decimation: -12 sec cooldown | cooldown -12 s | hook_cd | Blue | 100 | high |
| Gore (805555) | Gore: +3% critical strike chance | critical chance +3 | spell_id | Blue | 80 | high |
| Gore (805555) | Gore: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Pestilence of Death (801054) | Pestilence of Death: +8% Unleash damage | damage +8 % | spell_id | Blue | 80 | medium |

<details><summary>Avvist (6)</summary>

| Spell | Grunn |
|---|---|
| Call: Hellfire Abyssal (805074) | summon (child 704247 Hellfire Abyssal) |
| Call: Hellfire Imp (804883) | summon (child 805966 Hellfire Imp) |
| Chains of Xoroth (706756) | summon (chains object) |
| Hellbound Charge (807247) | mobility (charges, no cd field); DUMMY speed buff, no meaningful op |
| Implosion (524897) | summon (SUMMON eff0, Hellfire Imps) |
| Legion's Presence (804879) | is_summon=self; buffs imps (ASCENSION_A_TO_SUMMONS) |

</details>

## 18 GUARDIAN

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Brace (800313) | Brace: -20 sec cooldown | cooldown -20 s | hook_cd | Epic | 100 | high |
| Brace (800313) | Brace: +1 sec duration | duration +1 s | spell_id | Epic | 80 | high |
| Counter Stance (802188) | Counter Stance: -20 sec cooldown | cooldown -20 s | mask | Blue | 100 | high |
| Counter Stance (802188) | Counter Stance: +10% retaliation damage | effect2 +10 % | spell_id | Blue | 70 | medium |
| Glorious Arena (300983) | Glorious Arena: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | high |
| Grand Entrance (802870) | Grand Entrance: -5 sec cooldown | cooldown -5 s | mask | Blue | 100 | high |
| Grand Entrance (802874) | Grand Entrance: +8% damage | damage +8 %; dot +8 % | mask | Blue | 100 | high |
| Knight's Calling (504151) | Knight's Calling: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | high |
| Net Throw (802304) | Net Throw: -2 sec cooldown | cooldown -2 s | mask | Blue | 100 | high |
| Press the Attack (801219) | Press the Attack: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 100 | high |
| Press the Attack (704157) | Press the Attack: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Reflective Shield (300927) | Reflective Shield: -2 sec cooldown | cooldown -2 s | hook_cd | Blue | 80 | high |
| Shrug It Off (802198) | Shrug It Off: -4 sec cooldown | cooldown -4 s | hook_cd | Blue | 100 | high |

### Gladiator

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Centurion Strike (802286) | Centurion Strike: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Final Verdict (500258) | Final Verdict: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Spear Throw (500463) | Spear Throw: +8% damage | damage +8 % | mask | Blue | 100 | high |

### Inspiration

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Battle Drums (803683) | Battle Drums: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 100 | high |
| Champion's Presence (520835) | Champion's Presence: +2 sec duration | duration +2 s | spell_id | Epic | 100 | high |
| Harrowing Melody (572821) | Harrowing Melody: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Hero's March (801774) | Hero's March: -15 sec cooldown | cooldown -15 s | mask | Blue | 100 | high |
| Inspiring Presence (705352) | Inspiring Presence: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 70 | high |
| Liberation (803963) | Liberation: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | high |
| Song of Battle (803956) | Song of Battle: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 100 | high |
| Song of Steel (704540) | Song of Steel: +10% attack and spell power bonus | all effects +10 % | mask | Blue | 80 | medium |
| Sound of War (572612) | Sound of War: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Voice of an Angel (704420) | Voice of an Angel: -30 sec cooldown | cooldown -30 s | hook_cd | Blue | 100 | high |

### Protection

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Bastion (802283) | Bastion: -30 sec cooldown | cooldown -30 s | mask | Epic | 100 | high |
| Hammer of the Law (704418) | Hammer of the Law: -5 sec cooldown | cooldown -5 s | mask | Blue | 100 | high |
| Hammer of the Law (704418) | Hammer of the Law: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Heavy Blow (803129) | Heavy Blow: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Hold the Line (803830) | Hold the Line: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | high |

<details><summary>Avvist (3)</summary>

| Spell | Grunn |
|---|---|
| Banner of Conquest (500261) | summon via child 500264 (is_summon=child) |
| Banner of Swiftness (500259) | summon via child 800704 (is_summon=child) |
| Standard of Rallying (500263) | summon (is_summon=self): Standard is a creature |

</details>

## 19 MONK

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Blade of Faith (803872) | Blade of Faith: +4% critical strike chance | critical chance +4 | mask | Blue | 80 | high |
| Blade of Faith (803872) | Blade of Faith: +8% periodic damage | dot +8 % | mask | Blue | 100 | high |
| Divine Force (806153) | Divine Force: -15 sec cooldown | cooldown -15 s | mask | Blue | 100 | high |
| Glory (801481) | Glory: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | high |
| Interdict (560116) | Interdict: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | high |
| Libram of Fervor (805423) | Libram of Fervor: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 100 | high |
| Libram of Tenacity (801461) | Libram of Tenacity: -20 sec cooldown | cooldown -20 s | mask | Blue | 100 | high |
| Profound Enlightenment (680953) | Profound Enlightenment: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 60 | high |
| Silverhand Incantation (801465) | Silverhand Incantation: -10 sec cooldown | cooldown -10 s | mask | Blue | 100 | high |
| Silverhand Incantation (801501) | Silverhand Incantation: +8% damage and healing | damage +8 % | mask | Blue | 100 | medium |
| Testament of Fortitude (500678) | Testament of Fortitude: -20 sec cooldown | cooldown -20 s | hook_cd | Epic | 100 | high |
| Testament of Fortitude (500678) | Testament of Fortitude: +1.5 sec duration | duration +1.5 s | spell_id | Blue | 80 | high |
| Testament of Will (801455) | Testament of Will: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 70 | high |

### Discipline

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Absolution (800424) | Absolution: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 60 | high |
| Barrier of Light (805417) | Barrier of Light: -10 sec cooldown | cooldown -10 s | mask | Blue | 100 | high |
| Eternal Blessing (705300) | Eternal Blessing: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | high |
| Reckoning (805421) | Reckoning: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Sacred Swing (705293) | Sacred Swing: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Tome of Light (805422) | Tome of Light: -30 sec cooldown | cooldown -30 s | hook_cd | Epic | 100 | high |

### Fighting

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Force of Golganneth (500694) | Force of Golganneth: -15 sec cooldown | cooldown -15 s | mask | Blue | 100 | high |
| Libram of Zeal (801466) | Libram of Zeal: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 100 | high |
| Norgannon's Wrath (524739) | Norgannon's Wrath: -3 sec cooldown | cooldown -3 s | mask | Blue | 100 | high |
| Norgannon's Wrath (524740) | Norgannon's Wrath: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Righteous Upheaval (501562) | Righteous Upheaval: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Vindication (801446) | Vindication: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Zealotry (801450) | Zealotry: +8% damage | damage +8 % | mask | Blue | 80 | high |

### Runes

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Argent Blade (804929) | Argent Blade: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Crusader's Brand (300513) | Crusader's Brand: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | high |
| Divine Charge (527023) | Divine Charge: -20 sec cooldown | cooldown -20 s | mask | Blue | 100 | high |
| Righteous Tempest (805409) | Righteous Tempest: -10% Energy cost | cost -10 % | mask | Blue | 70 | high |
| Righteous Tempest (805410) | Righteous Tempest: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Testament of Strength (300514) | Testament of Strength: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 100 | high |

<details><summary>Avvist (1)</summary>

| Spell | Grunn |
|---|---|
| Testament of Hope (1397742) | summon (is_summon=self): copies are creatures |

</details>

## 20 SONOFARUGAL

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Blood Pact (801955) | Blood Pact: -15 sec cooldown | cooldown -15 s | mask | Blue | 100 | high |
| Blood Veil (504263) | Blood Veil: +10% absorb cap | effect2 +10 % | spell_id | Blue | 100 | medium |
| Fleshcraft (801952) | Fleshcraft: -30 sec cooldown | cooldown -30 s | mask | Epic | 100 | high |
| Hemostasis (681304) | Hemostasis: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | high |
| Liquify (806310) | Liquify: -15 sec cooldown | cooldown -15 s | mask | Blue | 100 | high |
| Scarlet Delirium (801074) | Scarlet Delirium: -0.2 sec cast time | casting time -0.2 s | mask | Blue | 100 | high |
| Scarlet Delirium (801074) | Scarlet Delirium: +8% periodic damage | dot +8 % | mask | Blue | 100 | high |
| Shadow Howl (806177) | Shadow Howl: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 70 | high |
| Transfusion (705734) | Transfusion: -20 sec cooldown | cooldown -20 s | mask | Blue | 100 | high |
| Wicked Howl (804207) | Wicked Howl: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | high |

### Fleshweaver

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Arterial Bind (681082) | Arterial Bind: +8% periodic healing | dot +8 % | spell_id | Blue | 80 | high |
| Crimson Tide (504282) | Crimson Tide: +8% periodic damage | dot +8 % | mask | Blue | 100 | high |
| Dark Liturgy (800781) | Dark Liturgy: +8% healing | damage +8 % | mask | Blue | 100 | high |
| Darkfallen Lament (524180) | Darkfallen Lament: +8% healing | damage +8 % | spell_id | Blue | 80 | high |
| Gorge (804805) | Gorge: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 100 | high |
| Heartbreak (520314) | Heartbreak: +8% damage and healing | damage +8 % | spell_id | Blue | 100 | high |
| Hemal Excision (803681) | Hemal Excision: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 70 | high |
| Hemoglobe (524907) | Hemoglobe: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 100 | high |
| Infuse (681403) | Infuse: -3 sec cooldown | cooldown -3 s | mask | Blue | 100 | high |
| Purify Blood (680679) | Purify Blood: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 70 | high |
| Red Thirst (520493) | Red Thirst: +2 sec duration | duration +2 s | spell_id | Epic | 100 | high |
| Vampyr's Kiss (504275) | Vampyr's Kiss: +8% periodic damage | dot +8 % | mask | Blue | 100 | high |

### Blood

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Aneurysm (806099) | Aneurysm: -3 sec cooldown | cooldown -3 s | hook_cd | Blue | 100 | high |
| Atherann's Anguish (680680) | Atherann's Anguish: -10 sec cooldown | cooldown -10 s | mask | Blue | 100 | high |
| Blood Craving (800780) | Blood Craving: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 100 | high |
| Keleseth's Calamity (560249) | Keleseth's Calamity: -0.5 sec cast time | casting time -0.5 s | mask | Blue | 100 | high |
| Keleseth's Calamity (560249) | Keleseth's Calamity: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Malediction (800776) | Malediction: -3 sec cooldown | cooldown -3 s | hook_cd | Blue | 80 | high |
| Malediction (704120) | Malediction: +8% periodic damage | dot +8 % | spell_id | Blue | 100 | high |
| Sacrificial Rite (800785) | Sacrificial Rite: -15 sec cooldown | cooldown -15 s | mask | Blue | 100 | high |
| Valanar's Vengeance (560315) | Valanar's Vengeance: -0.5 sec cast time | casting time -0.5 s | mask | Blue | 100 | high |
| Valanar's Vengeance (560315) | Valanar's Vengeance: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Vampiric Fang (804726) | Vampiric Fang: +8% damage | damage +8 % | mask | Blue | 100 | high |

### Ferocity

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Aortic Assault (806212) | Aortic Assault: -2 sec cooldown | cooldown -2 s | mask | Blue | 100 | high |
| Bloodsurge (553267) | Bloodsurge: -20 sec cooldown | cooldown -20 s | hook_cd | Epic | 100 | high |
| Endure the Curse (681190) | Endure the Curse: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | high |
| Final Embrace (524865) | Final Embrace: -20 sec cooldown | cooldown -20 s | mask | Epic | 100 | high |
| Hemoburst (572855) | Hemoburst: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Reave (800490) | Reave: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Veinburst (504260) | Veinburst: +8% damage | damage +8 % | mask | Blue | 100 | high |

### Packleader

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Blood Howl (800782) | Blood Howl: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 60 | high |
| Eternal Resolve (801962) | Eternal Resolve: -20 sec cooldown | cooldown -20 s | hook_cd | Epic | 100 | high |
| Night Hunter's Howl (500124) | Night Hunter's Howl: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Rotclaw (804197) | Rotclaw: +8% damage | damage +8 %; dot +8 % | mask | Blue | 100 | high |

<details><summary>Avvist (2)</summary>

| Spell | Grunn |
|---|---|
| Blood Bond (504627) | Blood Bond: 1800 s bond utility; no meaningful numeric op (CD irrelevant, proc values scripted) |
| Monstrous Howl (804811) | summon via child 801958 Call of the Darkwing (is_summon=child) |

</details>

## 21 RANGER

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Adrenaline Rush (520578) | Adrenaline Rush: -12 sec cooldown | cooldown -12 s | mask | Blue | 100 | high |
| Briar Veil (800084) | Briar Veil: -15 sec cooldown | cooldown -15 s | mask | Epic | 100 | high |
| Crippling Shot (804725) | Crippling Shot: -1.5 sec cooldown | cooldown -1.5 s | hook_cd | Blue | 70 | high |
| Cutthroat (800359) | Cutthroat: -6 sec cooldown | cooldown -6 s | mask | Blue | 100 | high |
| Cutthroat (800359) | Cutthroat: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Guise (520753) | Guise: -20 sec cooldown | cooldown -20 s | hook_cd | Epic | 100 | high |
| Knockout (801435) | Knockout: -2 sec cooldown | cooldown -2 s | mask | Blue | 80 | high |
| Natural Disguise (520585) | Natural Disguise: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 60 | high |
| Onslaught (801951) | Onslaught: -15 sec cooldown | cooldown -15 s | hook_cd | Epic | 100 | high |
| Snapseed (804027) | Snapseed: -3 sec cooldown | cooldown -3 s | mask | Blue | 100 | high |
| Snatch (803115) | Snatch: -6 sec cooldown | cooldown -6 s | hook_cd | Blue | 70 | high |
| Survival Potion (802839) | Survival Potion: -8 sec cooldown | cooldown -8 s | mask | Blue | 80 | high |
| Woodland Adept (570015) | Woodland Adept: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 70 | high |

### Archery

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Brutal Shot (570014) | Brutal Shot: -3 sec cooldown | cooldown -3 s | mask | Blue | 100 | high |
| Brutal Shot (570014) | Brutal Shot: +8% damage | damage +8 % | mask | Epic | 100 | high |
| Deadshot (520227) | Deadshot: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Hunting Shot (801191) | Hunting Shot: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Incendiary Shot (524870) | Incendiary Shot: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Neurotoxin Arrow (500071) | Neurotoxin Arrow: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 70 | high |
| Precision Shot (500075) | Precision Shot: -0.3 sec cast time | casting time -0.3 s | mask | Blue | 100 | high |
| Precision Shot (500075) | Precision Shot: +8% damage | damage +8 % | mask | Blue | 100 | high |

### Survival

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Backstep (800685) | Backstep: -3 sec cooldown | cooldown -3 s | mask | Blue | 100 | high |
| Emerald Arrow (804712) | Emerald Arrow: -0.4 sec cast time | casting time -0.4 s | mask | Blue | 100 | high |
| Emerald Arrow (804712) | Emerald Arrow: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Freedom (520628) | Freedom: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 50 | high |
| Horn of Alacrity (806360) | Horn of Alacrity: -6 sec cooldown | cooldown -6 s | hook_cd | Blue | 70 | high |
| Horn of War (800086) | Horn of War: -6 sec cooldown | cooldown -6 s | hook_cd | Blue | 70 | high |
| Thalassian Brand (560805) | Thalassian Brand: +8% periodic damage | dot +8 % | spell_id | Blue | 100 | high |
| Whipvine Arrow (806342) | Whipvine Arrow: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | high |
| Woodland Arrow (806368) | Woodland Arrow: +8% damage | damage +8 % | spell_id | Blue | 100 | high |

### Dueling

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Assault (803108) | Assault: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Dirty Blades (680276) | Dirty Blades: +1 sec duration | duration +1 s | mask | Blue | 70 | medium |
| Hookshot (800360) | Hookshot: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Outmaneuver (557325) | Outmaneuver: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 50 | low |
| Quills (560966) | Quills: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Ravage (803851) | Ravage: +8% bleed damage | dot +8 % | spell_id | Blue | 100 | high |
| Skewer (800093) | Skewer: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Throatpunch (500617) | Throatpunch: -1.5 sec cooldown | cooldown -1.5 s | hook_cd | Blue | 80 | high |
| Viper's Bite (803104) | Viper's Bite: -3 sec cooldown | cooldown -3 s | hook_cd | Blue | 100 | high |
| Viper's Bite (803105) | Viper's Bite: +8% damage | damage +8 % | mask | Blue | 100 | high |

<details><summary>Avvist (5)</summary>

| Spell | Grunn |
|---|---|
| Battle Screech (705070) | Battle Screech: raid buff without cooldown; utility |
| Command Aura (524600) | Command Aura: damage procs from allies (537249) and Exhaustion 573255 dur 60000 = cd, so CD reduction is useless |
| Falcon's Call (804715) | summon (is_summon=child: Falcon 800251) |
| Frenzy (520492) | Frenzy: allies get Worn Out 804457 dur 300000 = cd, so CD reduction is useless; haste buff utility |
| Skirmish (802039) | Skirmish: no cd/duration in data, crit/cost values are scripted per Advantage; no meaningful op |

</details>

## 22 CHRONOMANCER

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Accelerated Recovery (800857) | Accelerated Recovery: +8% healing | dot +8 % | spell_id | Blue | 100 | high |
| Anomaly Spike (503826) | Anomaly Spike: +8% damage | damage +8 % | mask | Blue | 100 | medium |
| Desynchronization (561310) | Desynchronization: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 60 | high |
| Dimensional Divergence (802790) | Dimensional Divergence: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 60 | high |
| Fortify Timeline (804491) | Fortify Timeline: +8% healing | damage +8 % | mask | Blue | 100 | high |
| Gravity Bomb (801281) | Gravity Bomb: -12 sec cooldown | cooldown -12 s | mask | Blue | 100 | high |
| Gravity Bomb (801282) | Gravity Bomb: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Infinite Shield (520457) | Infinite Shield: -2 sec cooldown | cooldown -2 s | mask | Blue | 100 | high |
| Infinite Shield (520458) | Infinite Shield: +8% healing | damage +8 % | spell_id | Blue | 100 | high |
| Mass Babify (520855) | Mass Babify: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 60 | high |
| Temporal Anomaly (806315) | Temporal Anomaly: -15 sec cooldown | cooldown -15 s | mask | Epic | 100 | high |
| Temporal Focus (806165) | Temporal Focus: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 70 | high |
| Timeguard (804441) | Timeguard: -15 sec cooldown | cooldown -15 s | mask | Blue | 80 | high |
| Waves of Time (801277) | Waves of Time: -3 sec cooldown | cooldown -3 s | mask | Blue | 60 | high |

### Displacement

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Buy Time (520188) | Buy Time: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 60 | high |
| Continuum Restoration (801271) | Continuum Restoration: -12 sec cooldown | cooldown -12 s | mask | Blue | 80 | high |
| Correct the Mistake (572352) | Correct the Mistake: +8% healing | damage +8 % | spell_id | Blue | 100 | high |
| Displacement (806727) | Displacement: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 60 | high |
| Fabric of Time (806299) | Fabric of Time: -2 sec cooldown | cooldown -2 s | mask | Blue | 100 | high |
| Fabric of Time (806299) | Fabric of Time: +8% healing | damage +8 % | mask | Blue | 100 | high |
| Ripple (806296) | Ripple: -4 sec cooldown | cooldown -4 s | mask | Blue | 100 | high |
| The Vast Infinite (706083) | The Vast Infinite: -30 sec cooldown | cooldown -30 s | hook_cd | Epic | 80 | high |

### Duality

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Chromatic Shard (801292) | Chromatic Shard: -0.35 sec cast time | casting time -0.35 s | mask | Blue | 100 | high |
| Chromatic Shard (801292) | Chromatic Shard: -4 sec cooldown | cooldown -4 s | mask | Blue | 100 | medium |
| Chromatic Shard (801292) | Chromatic Shard: +8% damage | damage +8 % | mask | Epic | 100 | high |
| Incarnation of Chaos (570067) | Incarnation of Chaos: -20 sec cooldown | cooldown -20 s | mask | Epic | 100 | high |
| Melt Reality (806335) | Melt Reality: +8% periodic damage | dot +8 % | mask | Blue | 100 | high |
| Slipstream (707555) | Slipstream: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 50 | low |

### Time

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Arc Collision (524853) | Arc Collision: +8% periodic damage | dot +8 % | spell_id | Blue | 100 | high |
| Flux Emitter (804435) | Flux Emitter: -2 sec cooldown | cooldown -2 s | mask | Blue | 60 | high |
| Paradox Cannon (806203) | Paradox Cannon: -2 sec cooldown | cooldown -2 s | mask | Blue | 70 | high |
| Shatter Echo (804503) | Shatter Echo: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Singularity Core (804437) | Singularity Core: +8% damage | damage +8 % | mask | Blue | 100 | high |

<details><summary>Avvist (6)</summary>

| Spell | Grunn |
|---|---|
| Aeon of Oblivion (806293) | Aeon of Oblivion: ignore_caster_mods |
| Aeon of Protection (806292) | Aeon of Protection: Epoch modifier toggle (cd 5000, dur -1); values only modify Epoch, no meaningful op |
| Aeon of Renewal (806290) | Aeon of Renewal: Epoch modifier toggle (cd 5000, dur -1); HoT 560355 amount derived from healing done (scripted) |
| Aether Compression (804436) | Aether Compression: utility (cast while moving), no numeric effect worth an affix |
| Destabilize Time (680971) | Destabilize Time: utility debuff without cooldown; stacking scripted |
| Unstable Chronoglass (503836) | summon (Unstable Chronoglass, is_summon=self) |

</details>

## 23 NECROMANCER

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Bone Tithe (802121) | Bone Tithe: -15 sec cooldown | cooldown -15 s | mask | Epic | 100 | high |
| Command: Hook (504316) | Command: Hook: -3 sec cooldown | cooldown -3 s | hook_cd | Blue | 60 | medium |
| Death's Due (807796) | Death's Due: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 100 | high |
| Death's Due (808016) | Death's Due: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Fetid Ward (680388) | Fetid Ward: +10% damage converted to healing | effect1 +10 % | spell_id | Blue | 70 | medium |
| Glacial Tap (805369) | Glacial Tap: -1.5 sec cooldown | cooldown -1.5 s | mask | Blue | 100 | high |
| Glacial Tap (805369) | Glacial Tap: +10% Runic Power generated | effect1 +10 % | mask | Blue | 100 | high |
| Mass Grave (803741) | Mass Grave: -5 sec cooldown | cooldown -5 s | hook_cd | Blue | 70 | high |

### Death

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Flesh to Worms (500338) | Flesh to Worms: +8% periodic damage | dot +8 % | mask | Blue | 100 | high |
| Lichplague (802132) | Lichplague: -4 sec cooldown | cooldown -4 s | hook_cd | Blue | 100 | medium |
| Lichplague (802132) | Lichplague: +8% periodic damage | dot +8 % | spell_id | Epic | 100 | high |
| Mutation (500342) | Mutation: -20 sec cooldown | cooldown -20 s | hook_cd | Epic | 80 | high |
| Plague of Undeath (801945) | Plague of Undeath: -0.15 sec cast time | casting time -0.15 s | spell_id | Blue | 100 | high |
| Plague of Undeath (801945) | Plague of Undeath: +8% periodic damage | dot +8 % | spell_id | Blue | 100 | high |
| Virulency (801938) | Virulency: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 60 | low |

### Rime

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Command: Bonefreeze (504489) | Command: Bonefreeze: -2 sec cooldown | cooldown -2 s | hook_cd | Blue | 60 | medium |
| Glacial Impact (707592) | Glacial Impact: +4% critical strike chance | critical chance +4 | spell_id | Blue | 70 | high |
| Glacial Impact (707592) | Glacial Impact: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Heartchill (801739) | Heartchill: -4 sec cooldown | cooldown -4 s | mask | Blue | 80 | high |
| Ice Barrage (801760) | Ice Barrage: -0.2 sec cast time | casting time -0.2 s | mask | Blue | 100 | high |
| Ice Barrage (801760) | Ice Barrage: -1.5 sec cooldown | cooldown -1.5 s | mask | Blue | 100 | high |
| Ice Barrage (803779) | Ice Barrage: +8% damage | damage +8 % | spell_id | Blue | 100 | high |

<details><summary>Avvist (20)</summary>

| Spell | Grunn |
|---|---|
| Animate: Bone Construct (531130) | summon (is_summon=self) |
| Animate: Bone Wraith (805032) | summon (is_summon=self) |
| Animate: Frost Wyrm (805428) | summon (is_summon=self) |
| Animate: Plaguefather (805048) | summon (is_summon=self) |
| Animate: Skeletal Archer (805040) | summon (is_summon=self) |
| Animate: Tomb King (805044) | summon (is_summon=self) |
| Army of the North (561138) | summon (is_summon=self) |
| Command: Blight (504050) | Command: Blight: minion command (summon-based), no cooldown |
| Create Frozen Reliquary (803772) | Create Frozen Reliquary: item creation utility, no numbers |
| Foul Invocation (804371) | is_summon flagged (ASCENSION_APPLY_AURA_TO_SUMMONS); form buff |
| Lich Form (500981) | Lich Form: permanent form (dur -1); only a 10% damage aura, too small/odd to modify |
| March of the Dead (707007) | March of the Dead: SCRIPT_EFFECT; damage done by scripted marching dead (creatures) |
| Phylactery (500933) | summon (is_summon=self) |
| Raise: Abomination (500335) | summon (is_summon=self) |
| Raise: Banshee (504861) | summon (is_summon=self) |
| Raise: Decaying Colossus (500989) | summon (is_summon=self) |
| Raise: Gargoyle (500329) | summon (is_summon=self) |
| Raise: Ghoul (500971) | summon (is_summon=self) |
| Raise: Skeletal Mage (500331) | summon (is_summon=self) |
| Summoning Ritual (805038) | summon (is_summon=self) |

</details>

## 24 PYROMANCER

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Cataclysm (520218) | Cataclysm: -12 sec cooldown | cooldown -12 s | mask | Blue | 75 | high |
| Circle of Fire (800807) | Circle of Fire: -15 sec cooldown | cooldown -15 s | mask | Blue | 75 | high |
| Circle of Fire (680842) | Circle of Fire: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Dragon Leap (806611) | Dragon Leap: -4 sec cooldown | cooldown -4 s | hook_cd | Blue | 100 | medium |
| Ember Touch (800818) | Ember Touch: +8% healing | damage +8 % | mask | Blue | 100 | high |
| Essence of Malygos (802120) | Essence of Malygos: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | high |
| Gaze of Ysera (806148) | Gaze of Ysera: -12 sec cooldown | cooldown -12 s | mask | Blue | 75 | high |
| Grace of Alexstrasza (802167) | Grace of Alexstrasza: -30 sec cooldown | cooldown -30 s | mask | Blue | 75 | high |
| Ignis Ultimatus (680369) | Ignis Ultimatus: -8 sec cooldown | cooldown -8 s | mask | Epic | 100 | high |
| Ignis Ultimatus (680371) | Ignis Ultimatus: +8% damage and healing | damage +8 % | spell_id | Epic | 100 | high |
| Inferno Barrier (504380) | Inferno Barrier: +10% absorb | effect1 +10 % | spell_id | Blue | 100 | medium |
| Meteor (500135) | Meteor: -4 sec cooldown | cooldown -4 s | mask | Blue | 100 | medium |
| Meteor (804076) | Meteor: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Volcanic Shell (805477) | Volcanic Shell: +10% absorb | effect1 +10 % | mask | Blue | 100 | medium |
| Volcanic Shell (805477) | Volcanic Shell: -15 sec cooldown | cooldown -15 s | mask | Blue | 75 | high |

### Destruction

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Cleansing Flames (681145) | Cleansing Flames: -10% cost | cost -10 % | mask | Blue | 75 | high |
| Cleansing Flames (681142) | Cleansing Flames: +8% damage and healing | damage +8 % | mask | Blue | 100 | high |
| Emberheart (680378) | Emberheart: -15 sec cooldown | cooldown -15 s | mask | Epic | 100 | high |
| Sunstrider Array (804230) | Sunstrider Array: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 100 | high |

### Incineration

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Fired Up! (704823) | Fired Up!: -8 sec cooldown | cooldown -8 s | mask | Epic | 100 | high |
| Ignite (800791) | Ignite: -10% cast time | casting time -10 % | mask | Blue | 75 | high |
| Ignite (800791) | Ignite: +10% periodic damage | dot +10 % | mask | Epic | 100 | high |
| Pillar of Flame (805496) | Pillar of Flame: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Pyroclasm (520019) | Pyroclasm: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | high |

### Draconic

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Aspect's Blessing (802168) | Aspect's Blessing: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 100 | high |
| Breath of Neltharion (511109) | Breath of Neltharion: -12 sec cooldown | cooldown -12 s | hook_cd | Blue | 50 | high |
| Death From Above (520402) | Death From Above: -12 sec cooldown | cooldown -12 s | hook_cd | Blue | 100 | high |
| Destroyer's Maw (802107) | Destroyer's Maw: -0.2 sec cast time | casting time -0.2 s | mask | Blue | 75 | high |
| Destroyer's Maw (802107) | Destroyer's Maw: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Firefall (801905) | Firefall: +8% damage | damage +8 % | mask | Blue | 100 | high |

<details><summary>Avvist (7)</summary>

| Spell | Grunn |
|---|---|
| Burn Impurities (520149) | dispel utility without numbers or cooldown |
| Firestorm (802791) | summon (SUMMON tornado) |
| Kael's Command (800816) | summon-based (Phoenix heals, is_summon=self) |
| Neltharion's Resolve (520488) | niche fear-ward utility; only hook_cd possible, low value |
| Phoenix Dive (706854) | Phoenix Dive: DUMMY scripted, executed by Phoenix pet; no cd/mask |
| Phoenix Egg (800796) | summon (Phoenix) |
| Roaring Pyre (704278) | summon (Roaring Pyre) |

</details>

## 25 CULTIST

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Abyssal Ward (804670) | Abyssal Ward: -12 sec cooldown | cooldown -12 s | hook_cd | Blue | 100 | high |
| Corrupt Mind (560109) | Corrupt Mind: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 75 | high |
| Dark Infusion (570263) | Dark Infusion: +8% healing | damage +8 % | spell_id | Blue | 100 | low |
| Embrace the Void (582591) | Embrace the Void: -30 sec cooldown | cooldown -30 s | hook_cd | Blue | 100 | high |
| Mass Nightmare (805114) | Mass Nightmare: -20 sec cooldown | cooldown -20 s | mask | Blue | 75 | high |
| Twisted Seal (525065) | Twisted Seal: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 75 | high |
| Vision of Doom (520388) | Vision of Doom: -30 sec cooldown | cooldown -30 s | hook_cd | Epic | 100 | high |
| Void Shield (500715) | Void Shield: +10% absorb | effect1 +10 % | mask | Blue | 100 | medium |

### Influence

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Black Blood (804153) | Malevolence: +10% Black Blood healing | dot +10 % | mask | Blue | 100 | high |
| Dark Veil (520345) | Dark Veil: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | high |
| Eldritch Eye (802049) | Eldritch Eye: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | high |
| Forbidden Ritual (800105) | Forbidden Ritual: +10% absorb | effect1 +10 % | mask | Blue | 100 | medium |
| Forbidden Ritual (800105) | Forbidden Ritual: -8 sec cooldown | cooldown -8 s | mask | Blue | 75 | medium |
| Hand of Yogg-Saron (704476) | Hand of Yogg-Saron: -15 sec cooldown | cooldown -15 s | mask | Epic | 100 | high |
| Malevolence (500714) | Malevolence: +8% damage | damage +8 % | spell_id | Blue | 100 | high |

### Corruption

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Ancient Curse (500712) | Ancient Curse: -4 sec cooldown | cooldown -4 s | hook_cd | Blue | 100 | high |
| Ancient Curse (500712) | Ancient Curse: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Darkwither (800446) | Darkwither: +10% periodic damage | dot +10 % | mask | Epic | 100 | high |
| Eldritch Devastation (802727) | Eldritch Devastation: +8% damage | dot +8 %; damage +8 % | mask | Blue | 100 | high |
| Obliteration Beam (805572) | Obliteration Beam: -3 sec cooldown | cooldown -3 s | mask | Blue | 100 | medium |
| Obliteration Beam (805573) | Obliteration Beam: +8% damage | damage +8 % | mask | Blue | 100 | medium |
| Psychic Suppression (800432) | Psychic Suppression: -12 sec cooldown | cooldown -12 s | mask | Blue | 75 | high |
| Psychic Suppression (800432) | Psychic Suppression: +10% periodic damage | dot +10 % | mask | Blue | 100 | high |

### Godblade

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Empire's Grasp (500704) | Empire's Grasp: -5 sec cooldown | cooldown -5 s | mask | Blue | 75 | high |
| End Times (805115) | End Times: -12 sec cooldown | cooldown -12 s | hook_cd | Blue | 100 | high |
| Netherstrike (806222) | Netherstrike: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Rift (806250) | Rift: -6 sec cooldown | cooldown -6 s | mask | Blue | 100 | high |
| Voidborne (681104) | Voidborne: -20 sec cooldown | cooldown -20 s | hook_cd | Epic | 100 | high |

### Bulwark

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Crushing Dissonance (804056) | Crushing Dissonance: -4 sec cooldown | cooldown -4 s | hook_cd | Blue | 75 | high |
| Dreadfall (806175) | Dreadfall: -3 sec cooldown | cooldown -3 s | hook_cd | Blue | 75 | high |
| Twilight Shieldtoss (524876) | Twilight Shieldtoss: +8% damage | damage +8 % | spell_id | Blue | 100 | high |

<details><summary>Avvist (9)</summary>

| Spell | Grunn |
|---|---|
| Abyssal Covenant (500751) | persistent link buff (1800 s), no meaningful op |
| Devour Curse (800402) | dispel utility with minor self-heal |
| Eldritch Obelisk (560321) | summon-based (buffs Tentacles) |
| Entropic Singularity (800430) | slow-zone utility, no damage |
| Grasp of Zek'voz (573028) | 6 s slow utility; 15% CD cap <1 s, no damage |
| Hallucination (560301) | summon (Hallucination copies) |
| Hour of Twilight (255275) | summon child (Apocalyptic Skies 301988) |
| Tentacle of N'Zoth (572103) | summon (Tentacle of NZoth) |
| Tentacle of Y'shaarj (802044) | summon (Tentacle of Yshaarj) |

</details>

## 26 STARCALLER

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Elune's Presence (680774) | Elune's Presence: -20 sec cooldown | cooldown -20 s | hook_cd | Epic | 100 | high |
| Moonwell (804739) | Moonwell: -15 sec cooldown | cooldown -15 s | mask | Blue | 100 | high |
| Reverse Magic (570231) | Reverse Magic: -5 sec cooldown | cooldown -5 s | mask | Blue | 75 | high |
| Shooting Star (800505) | Shooting Star: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | medium |
| Starshatter (801135) | Starshatter: -12 sec cooldown | cooldown -12 s | hook_cd | Blue | 100 | high |
| Touch of Moonlight (574328) | Touch of Moonlight: +8% healing | damage +8 % | mask | Blue | 100 | high |
| Vial of Moonwell Water (804652) | Vial of Moonwell Water: -20 sec cooldown | cooldown -20 s | mask | Blue | 75 | high |
| Warden's Blade (805508) | Warden's Blade: +8% damage | damage +8 % | mask | Blue | 100 | high |

### Tides

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Celestial Form (801231) | Celestial Form: -8 sec cooldown | cooldown -8 s | hook_cd | Epic | 100 | high |
| Moonflow (575030) | Moonflow: +8% healing | damage +8 % | mask | Blue | 100 | high |
| Moonlit Slumber (805546) | Moonlit Slumber: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 75 | high |
| Prayer of Elune (801987) | Prayer of Elune: -4 sec cooldown | cooldown -4 s | mask | Blue | 75 | high |
| Prayer of Elune (801987) | Prayer of Elune: +8% healing | damage +8 % | mask | Blue | 100 | high |
| Stellar Convergence (572784) | Stellar Convergence: -12 sec cooldown | cooldown -12 s | hook_cd | Blue | 100 | high |

### Moonbow

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Arrow of the Goddess (563725) | Arrow of the Goddess: -8 sec cooldown | cooldown -8 s | mask | Epic | 100 | high |
| Arrow of the Goddess (563725) | Arrow of the Goddess: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Arrows In The Night (520481) | Arrows In The Night: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 100 | high |
| Drawstring of Elune (801975) | Drawstring of Elune: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | high |
| Starcall (800497) | Starcall: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Starfire Shot (801978) | Starfire Shot: -0.25 sec cast time | casting time -0.25 s | spell_id | Blue | 75 | high |
| Starfire Shot (801978) | Starfire Shot: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Trueshot (562319) | Trueshot: +8% damage | damage +8 % | mask | Blue | 100 | high |

### Hydromancy

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Astral Blade (805563) | Astral Blade: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Avatar of Vengeance (680822) | Avatar of Vengeance: -12 sec cooldown | cooldown -12 s | hook_cd | Epic | 100 | high |
| Fan of Knives (680703) | Fan of Knives: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Halt (807741) | Halt: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 75 | high |
| Sentinel Glaive (800506) | Sentinel Glaive: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Shadowsong's Mandate (805439) | Shadowsong's Mandate: -15 sec cooldown | cooldown -15 s | mask | Epic | 100 | high |
| Umbral Blade (574165) | Umbral Blade: +8% damage | damage +8 % | mask | Blue | 100 | high |

### Astralwarfare

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Blanket of Stars (804381) | Blanket of Stars: -5 sec cooldown | cooldown -5 s | hook_cd | Blue | 100 | high |
| Chosen of the Moon (680755) | Chosen of the Moon: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 100 | high |
| Moonlit Bulwark (805433) | Moonlit Bulwark: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 100 | high |
| Starburst (801996) | Starburst: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Starsweep (805550) | Starsweep: +8% damage | damage +8 % | spell_id | Blue | 100 | high |

<details><summary>Avvist (7)</summary>

| Spell | Grunn |
|---|---|
| Aspect of the Cosmos (801123) | Aspect stance (dur -1), proc utility; shared Aspect cooldown |
| Aspect of the Goddess (802203) | Aspect stance (dur -1), passive-like; shared Aspect cooldown |
| Astral Aegis (806155) | shared Aegis category cooldown; proc damage via script, low value |
| Astral Reconstitution (801155) | niche mana-conversion utility, scripted proc |
| Bonds of Justice (680705) | tether utility; grip damage scripted (DUMMY) |
| Cosmic Shift (801134) | stealth utility without numbers |
| Infused Aegis (704777) | damage child 707759 has ignore_caster_mods; shared Aegis category cooldown |

</details>

## 27 SUNCLERIC

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Circle of Valor (520647) | Circle of Valor: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 80 | medium |
| Circle of Valor (520648) | Circle of Valor: +8% healing | damage +8 % | spell_id | Blue | 100 | high |
| Glare (805583) | Glare: -15 sec cooldown | cooldown -15 s | mask | Blue | 100 | high |
| Horusath Blast (500154) | Horusath Blast: -3 sec cooldown | cooldown -3 s | mask | Blue | 100 | high |
| Horusath Blast (500154) | Horusath Blast: +8% damage | damage +8 % | spell_id | Epic | 100 | high |
| New Day (800600) | New Day: -60 sec cooldown | cooldown -60 s | hook_cd | Blue | 80 | medium |
| Radiance (800054) | Radiance: -15 sec cooldown | cooldown -15 s | mask | Blue | 100 | high |
| Radiance (800038) | Radiance: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Solar Invocation: Conquest (800764) | Solar Invocation: Conquest: -4 sec cooldown | cooldown -4 s | mask | Blue | 80 | high |
| Solar Invocation: Resplendence (806159) | Solar Invocation: Resplendence: -4 sec cooldown | cooldown -4 s | hook_cd | Blue | 100 | medium |
| Solar Invocation: Revelation (503651) | Solar Invocation: Revelation: -4 sec cooldown | cooldown -4 s | hook_cd | Blue | 70 | medium |
| Solar Prayer (806479) | Solar Prayer: -10 sec cooldown | cooldown -10 s | mask | Blue | 100 | high |
| Sun Stride (680700) | Sun Stride: -3 sec cooldown | cooldown -3 s | mask | Blue | 60 | high |

### Piety

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Dawnsear (500146) | Dawnsear: -0.25 sec cast time | casting time -0.25 s | spell_id | Blue | 100 | medium |
| Dawnsear (500146) | Dawnsear: +6% damage | damage +6 % | spell_id | Blue | 100 | high |
| Radiant Flame (807058) | Radiant Flame: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Rapture (806477) | Rapture: -0.25 sec cast time | casting time -0.25 s | mask | Blue | 100 | high |
| Rapture (806477) | Rapture: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Solar Concord (301266) | Solar Concord: -4 sec cooldown | cooldown -4 s | hook_cd | Blue | 80 | medium |
| Sun Down (572753) | Sun Down: +8% damage | damage +8 % | spell_id | Epic | 100 | high |
| Sunwell (560123) | Sunwell: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 50 | medium |

### Valkyr

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Champion of the Sun (800612) | Champion of the Sun: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | medium |
| Divine Retribution (680624) | Divine Retribution: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | high |
| Glorious Execution (800691) | Glorious Execution: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Justice (524857) | Justice: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Pendant of the Sun (680630) | Pendant of the Sun: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 60 | medium |
| Sunslam (805639) | Sunslam: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Sunstorm (760379) | Sunstorm: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | medium |
| Valkyr's Calling (520024) | Valkyr's Calling: -20 sec cooldown | cooldown -20 s | mask | Blue | 100 | high |

### Seraphim

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Calm (804057) | Calm: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 50 | medium |
| Dawnbreak (800654) | Dawnbreak: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Hammer of Kings (804752) | Hammer of Kings: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Justicar's Wrath (806980) | Justicar's Wrath: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Scroll of Hope (680646) | Scroll of Hope: -30 sec cooldown | cooldown -30 s | hook_cd | Blue | 80 | medium |
| Solar Nova (680621) | Solar Nova: +8% damage | damage +8 % | spell_id | Blue | 100 | high |

### Blessings

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Blessing of Absolution (804249) | Blessing of Absolution: -2 sec cooldown | cooldown -2 s | mask | Blue | 70 | high |
| Blessing of Purity (804253) | Blessing of Purity: -30 sec cooldown | cooldown -30 s | hook_cd | Blue | 100 | medium |
| Blessing of Retribution (804254) | Blessing of Retribution: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 50 | medium |
| Blessing of Triumph (804250) | Blessing of Triumph: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 100 | medium |
| Daybreak (500147) | Daybreak: +8% healing | damage +8 % | spell_id | Blue | 100 | high |
| Solar Invigoration (806123) | Solar Invigoration: -10 sec cooldown | cooldown -10 s | mask | Blue | 100 | high |

<details><summary>Avvist (6)</summary>

| Spell | Grunn |
|---|---|
| Bless (804247) | 5 min maintenance buff on one ally; heal child 805360 is HEAL_PCT with v=0 (src sql); no meaningful op |
| Holy Form (805301) | form toggle (dur -1), 10 s cd irrelevant; no meaningful op |
| Mercy (504848) | dispel utility (abolish disease), no numbers to modify |
| Seraphic Bulwark (560095) | charge-based (2 charges, scripted recharge), no cooldown field; block-value buff only |
| Sun Gate (802161) | summon (is_summon=self, Sun Gate object) |
| Vow of the Valkyr (807749) | Vow stance aura (dur -1), no cooldown or direct numbers to modify |

</details>

## 28 TINKER

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Air Strike (801744) | Air Strike: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 100 | medium |
| Air Strike (801745) | Air Strike: +8% damage | damage +8 % | spell_id | Epic | 100 | high |
| Auto Resuscitation Device (520445) | Auto Resuscitation Device: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 100 | medium |
| Kinetic Shield (806224) | Kinetic Shield: -30 sec cooldown | cooldown -30 s | mask | Blue | 100 | high |
| Macro-Gravity Zone (707104) | Macro-Gravity Zone: -15 sec cooldown | cooldown -15 s | mask | Blue | 70 | high |
| Med Pack (800347) | Med Pack: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | high |
| Med Pack (800347) | Med Pack: +8% periodic healing | dot +8 % | spell_id | Blue | 100 | high |
| Nanobot Reconstruction (801809) | Nanobot Reconstruction: +8% periodic healing | dot +8 % | spell_id | Blue | 100 | high |

### Firearms

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Bomb Toss (801005) | Bomb Toss: -1 sec cooldown | cooldown -1 s | mask | Blue | 80 | high |
| Bomb Toss (801005) | Bomb Toss: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Build: Spider Bomb (500535) | Build: Spider Bomb: -2 sec cooldown | cooldown -2 s | mask | Blue | 100 | high |
| Hyperblast Barrage (500249) | Hyperblast Barrage: -4 sec cooldown | cooldown -4 s | hook_cd | Blue | 100 | medium |
| Hyperblast Barrage (500249) | Hyperblast Barrage: +8% damage | damage +8 % | spell_id | Epic | 100 | high |
| Rockadier (801827) | Rockadier: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 70 | medium |
| Rocket Launcher (500235) | Rocket Launcher: -0.25 sec cast time | casting time -0.25 s | mask | Blue | 100 | high |
| Rocket Launcher (500220) | Rocket Launcher: +3% critical strike chance | critical chance +3 | spell_id | Blue | 70 | medium |
| Rocket Launcher (500220) | Rocket Launcher: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |

### Mechanics

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Combustion (801388) | Mechsuit: Combustion: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Makeshift Dynamite (504527) | Makeshift Dynamite: -0.2 sec cast time | casting time -0.2 s | spell_id | Blue | 70 | medium |
| Makeshift Dynamite (504527) | Makeshift Dynamite: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Mechsuit: Activate Jets (801389) | Mechsuit: Activate Jets: -1 sec cooldown | cooldown -1 s | mask | Blue | 60 | high |
| Mechsuit: Combustion (801387) | Mechsuit: Combustion: -1 sec cooldown | cooldown -1 s | mask | Blue | 60 | high |
| Mechsuit: Laser Beam (805372) | Mechsuit: Laser Beam: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Overclocked Machine (806757) | Overclocked Machine: -15 sec cooldown | cooldown -15 s | mask | Blue | 100 | high |

### Invention

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Combat Symbiosis (805305) | Combat Symbiosis: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 70 | medium |
| Nanobot Swarm (573054) | Nanobot Swarm: +8% damage | damage +8 % | spell_id | Blue | 70 | medium |
| Zap! (680196) | Zap!: -2 sec cooldown | cooldown -2 s | mask | Blue | 100 | high |
| Zap! (680196) | Zap!: +8% healing | damage +8 % | spell_id | Blue | 100 | high |

<details><summary>Avvist (18)</summary>

| Spell | Grunn |
|---|---|
| Build: Alarm Beacon (560754) | summon (Alarm Beacon) |
| Build: Battery Recharge Station (805308) | summon (Battery Recharge Station) |
| Build: Destructo-Bot (804673) | summon (Destructo-Bot) |
| Build: Firepot Drone (500600) | scripted drone (creature) with charges; no cooldown field, damage not by player |
| Build: Noise Box (807723) | summon (Noise Box) |
| Build: Oil-Spill Pylon (707257) | summon child (Oil-Spill Pylon) |
| Build: Repulsion Unit (706904) | summon (Repulsion Unit) |
| Build: Restorative Beacon (801801) | summon (Restorative Beacon) |
| Build: Scrapmaw (500242) | summon pet (Scrapmaw) |
| Build: Spider Bomb Factory (802052) | summon (Spider Bomb Factory) |
| Deathball (500236) | summon (Deathballs) |
| My Greatest Invention! (560744) | summon (Greatest Invention) |
| Nanobot Cleanser (502537) | dispel utility, no numbers to modify |
| Rocket Boots (500241) | charge-based movement (3 charges, scripted recharge), no cooldown/number to modify |
| Scrapper (525039) | out-of-combat scrap/mana generation, no meaningful op |
| Stim Augmentation (653239) | 1-hour weapon augmentation (enchant), no meaningful op |
| Tracer Augmentation (653245) | 1-hour weapon augmentation (enchant), no meaningful op |
| Upgrade! (800349) | only upgrades summoned Sentry Turrets (DUMMY); summon-based |

</details>

## 29 PROPHET

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Burrow (806154) | Burrow: -30 sec cooldown | cooldown -30 s | mask | Blue | 60 | high |
| Celerity (804962) | Celerity: -10 sec cooldown | cooldown -10 s | mask | Blue | 60 | high |
| Green Salve (800902) | Green Salve: +8% periodic healing | dot +8 % | spell_id | Blue | 100 | high |
| Green Salve (803529) | Green Salve: +8% healing | damage +8 % | spell_id | Blue | 80 | high |
| Hive Instinct (804968) | Hive Instinct: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | medium |
| Lifeblood (804963) | Lifeblood: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 100 | medium |
| Pinch (704235) | Pinch: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 60 | medium |
| Shadra's Balm (800901) | Shadra's Balm: +8% periodic healing | dot +8 % | spell_id | Blue | 100 | high |
| Shadra's Lair (804978) | Shadra's Lair: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 70 | medium |
| Spindlebind (800887) | Spindlebind: +8% damage | damage +8 % | spell_id | Blue | 80 | high |
| Toxic Stride (504347) | Toxic Stride: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 100 | medium |
| Weakening Venom (706000) | Weakening Venom: +8% damage | damage +8 % | spell_id | Blue | 60 | high |

### Fortitude

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Barbed Stinger (803208) | Barbed Stinger: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Catalyst (800895) | Catalyst: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 100 | medium |
| Expulsion (805094) | Expulsion: +8% periodic damage | dot +8 % | spell_id | Blue | 100 | high |
| Impale (560248) | Impale: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Molt (805102) | Molt: -5 sec cooldown | cooldown -5 s | mask | Blue | 100 | high |
| Regrow Exoskeleton (803197) | Regrow Exoskeleton: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | high |
| Venomtip Poison (803195) | Venomtip Poison: +8% damage | damage +8 % | spell_id | Blue | 100 | high |

### Stalking

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Contagion (503851) | Contagion: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 100 | medium |
| Corrosive Toxin (807342) | Withering Venom: +8% damage | damage +8 % | spell_id | Blue | 80 | high |
| Facemelter (800871) | Facemelter: +8% periodic damage | dot +8 % | spell_id | Blue | 100 | high |
| Nerubian Sting (800882) | Nerubian Sting: +8% periodic damage | dot +8 % | spell_id | Blue | 100 | high |
| Noxious Empowerment (804964) | Noxious Empowerment: -20 sec cooldown | cooldown -20 s | mask | Blue | 100 | high |
| Rotfang (804977) | Rotfang: +8% periodic damage | dot +8 % | spell_id | Blue | 100 | high |
| Widowmaker (804961) | Widowmaker: +8% damage | damage +8 % | spell_id | Epic | 100 | high |
| Withering Venom (706962) | Withering Venom: +8% periodic damage | dot +8 % | spell_id | Epic | 100 | high |

### Venom

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Mycosis (804986) | Mycosis: -2 sec cooldown | cooldown -2 s | mask | Blue | 80 | high |
| Mycosis (804986) | Mycosis: +8% damage | damage +8 % | spell_id | Blue | 100 | high |
| Sepsis Bloom (504867) | Sepsis Bloom: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | medium |
| Serpent Lord's Ring (503923) | Serpent Lord's Ring: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 100 | medium |
| Spore (804983) | Spore: +8% periodic damage | dot +8 % | spell_id | Blue | 100 | high |
| Wilt (804982) | Wilt: +8% periodic damage | dot +8 % | spell_id | Blue | 100 | high |

### Vizier

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Alkahest (504342) | Alkahest: +8% healing | damage +8 % | spell_id | Blue | 100 | high |
| Chrysalis (808082) | Chrysalis: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 60 | medium |
| Extraction (805884) | Extraction: -30 sec cooldown | cooldown -30 s | hook_cd | Blue | 100 | medium |
| Mending Mist (800899) | Mending Mist: +8% periodic healing | dot +8 % | spell_id | Blue | 100 | high |
| Mycelial Replenishment (706021) | Mycelial Replenishment: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 100 | medium |
| Serpent Lord's Amulet (800914) | Serpent Lord's Amulet: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 100 | medium |
| Shadra's Aid (504352) | Shadra's Aid: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 100 | medium |
| Toxic Communion (680767) | Toxic Communion: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | high |

<details><summary>Avvist (7)</summary>

| Spell | Grunn |
|---|---|
| Adrenal Venom (805775) | 2-hour venom toggle aura (proc haste), no cooldown/number worth modifying |
| Blight Antidote (520153) | dispel utility, no numbers to modify |
| Carapace Regeneration (805931) | charge-based toggle (dur -1, scripted recharge), no cooldown field |
| Decay (800910) | summon child (Mushroom 680764); Wilt/Spore have own affixes |
| Envenomed Weapons (300855) | 30-min party weapon buff; poison jets are cast by allies, caster mods do not apply |
| Fungal Assailant (504344) | summon (Fungarian) |
| Locust Swarm (560247) | summon (is_summon=self, locusts) |

</details>

## 30 REAPER

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Ghastly Screech (806146) | Ghastly Screech: -10 sec cooldown | cooldown -10 s | mask | Blue | 100 | high |
| Ghastly Screech (806147) | Ghastly Screech: +10% damage | damage +10 % | spell_id | Blue | 100 | medium |
| Jailer's Bargain (805718) | Jailer's Bargain: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | medium |
| Limbo (800845) | Limbo: -20 sec cooldown | cooldown -20 s | mask | Epic | 100 | high |
| Masochistic Rage (803030) | Masochistic Rage: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 100 | medium |
| Soul Tap (807397) | Soul Tap: -4 sec cooldown | cooldown -4 s | hook_cd | Blue | 80 | medium |
| Tormented Souls (500483) | Tormented Souls: -3 sec cooldown | cooldown -3 s | hook_cd | Blue | 100 | medium |
| Withering Touch (573071) | Withering Touch: -2 sec cooldown | cooldown -2 s | hook_cd | Blue | 80 | medium |
| Wraithblade (805258) | Wraithblade: -4 sec cooldown | cooldown -4 s | mask | Blue | 100 | medium |
| Wraithblade (805258) | Wraithblade: +3% critical strike chance | critical chance +3 | mask | Blue | 100 | high |
| Wraithblade (805258) | Wraithblade: +8% damage | damage +8 % | mask | Epic | 100 | high |

### Soul

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Deathchaser (805190) | Deathchaser: +10% periodic damage | dot +10 % | mask | Blue | 100 | high |
| Dirge (801311) | Dirge: +3% critical strike chance | critical chance +3 | spell_id | Blue | 100 | medium |
| Dirge (801311) | Dirge: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Endbringer (800922) | Endbringer: -20 sec cooldown | cooldown -20 s | mask | Epic | 100 | high |
| Ghostly Weapon (804474) | Ghostly Weapon: +10% damage | damage +10 % | mask | Blue | 100 | high |
| Ghostly Weapon (803997) | Ghostly Weapon: +2 sec duration | duration +2 s | mask | Blue | 80 | high |
| Sepulchral Renewal (561102) | Sepulchral Renewal: -60 sec cooldown | cooldown -60 s | hook_cd | Epic | 80 | medium |
| Shade (573038) | Shade: -20 sec cooldown | cooldown -20 s | hook_cd | Epic | 100 | medium |

### Reaping

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Crow's Harvest (504011) | Crow's Harvest: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Doomrend (800172) | Doomrend: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Harvest Time (803995) | Harvest Time: -20 sec cooldown | cooldown -20 s | mask | Epic | 100 | high |
| Harvest Time (803995) | Harvest Time: +2 sec duration | duration +2 s | mask | Blue | 100 | high |
| Siphon Essence (806125) | Siphon Essence: -3 sec cooldown | cooldown -3 s | hook_cd | Blue | 100 | medium |
| Siphon Essence (806125) | Siphon Essence: +10% damage | damage +10 % | spell_id | Blue | 80 | medium |
| Slaughter (500373) | Slaughter: +10% damage | damage +10 % | mask | Blue | 100 | high |

### Domination

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Bolstered Form (680337) | Bolstered Form: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | high |
| Dreadwake (803992) | Dreadwake: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Requiem (805717) | Requiem: +8% damage | damage +8 % | mask | Blue | 100 | high |

<details><summary>Avvist (6)</summary>

| Spell | Grunn |
|---|---|
| Harvesting Grounds (705413) | summon (is_summon=self) |
| Scythe Rush (500359) | gap-closer utility; no cooldown in DBC, no damage effect |
| Sinister Litany (805185) | instant resource generation (Reaped Souls), no cooldown/damage to modify |
| Spectral Scythe (500484) | summon (is_summon=self); damage by creatures |
| Spectral Warden (805716) | summon (is_summon=self) |
| Veilwalk (803990) | movement utility; no cooldown in DBC (charges/recharge scripted), no damage |

</details>

## 31 WILDWALKER

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Bearskin (800094) | Bearskin: -30 sec cooldown | cooldown -30 s | mask | Epic | 100 | high |
| Boulder Dash (500692) | Boulder Dash: -20 sec cooldown | cooldown -20 s | mask | Epic | 100 | high |
| Boulder Dash (500693) | Boulder Dash: +10% damage | damage +10 % | spell_id | Blue | 80 | medium |
| Bramblepatch (806415) | Bramblepatch: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 60 | medium |
| Dreamslip (805867) | Dreamslip: -20 sec cooldown | cooldown -20 s | hook_cd | Epic | 60 | medium |
| Geode (804002) | Rock Barrier: +10% Geode damage | damage +10 % | mask | Blue | 100 | high |
| Grove Guardian (504824) | Grove Guardian: +10% damage and healing | effect1 +10 % | spell_id | Blue | 100 | medium |
| Primal Convergence (800181) | Primal Convergence: -20 sec cooldown | cooldown -20 s | hook_cd | Epic | 100 | medium |
| Primal Rush (500696) | Primal Rush: +10% damage | damage +10 % | mask | Blue | 100 | high |
| Protective Roar (802782) | Protective Roar: -15 sec cooldown | cooldown -15 s | mask | Blue | 100 | high |
| Stone Grip (800145) | Stone Grip: -10% cast time | casting time -10 % | spell_id | Blue | 60 | medium |
| Wildheart (803980) | Wildheart: -30 sec cooldown | cooldown -30 s | mask | Epic | 100 | high |

### Life

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Ancient of Lore (805105) | Ancient of Lore: -20 sec cooldown | cooldown -20 s | mask | Blue | 100 | high |
| Ancient of War (504222) | Ancient of War: -20 sec cooldown | cooldown -20 s | hook_cd | Blue | 100 | medium |
| Earthmother's Binding (805107) | Earthmother's Binding: +8% healing | damage +8 % | mask | Blue | 100 | high |
| Neptulon's Wrath (807467) | Neptulon's Wrath: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 100 | medium |
| Sacred Grove (800180) | Sacred Grove: +10% healing | damage +10 % | mask | Blue | 100 | high |
| Seismic Wave (805462) | Seismic Wave: +8% healing and damage | damage +8 % | spell_id | Blue | 100 | medium |
| Spirit Charge (800144) | Spirit Charge: +8% healing and damage | damage +8 % | spell_id | Blue | 100 | medium |

### Primal

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Bring Me Their Bones (806552) | Bring Me Their Bones: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 60 | medium |
| Frenzied Roar (800133) | Frenzied Roar: -8 sec cooldown | cooldown -8 s | hook_cd | Blue | 100 | medium |
| Rylak's Bite (706342) | Rylak's Bite: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Savage Frenzy (806549) | Savage Frenzy: -20 sec cooldown | cooldown -20 s | hook_cd | Epic | 100 | medium |
| Throat Clamp (500764) | Throat Clamp: -2 sec cooldown | cooldown -2 s | hook_cd | Blue | 60 | medium |
| Totemic Smash (800178) | Totemic Smash: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |

### Mountainking

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Bashed (680949) | Bash: +10% damage | damage +10 % | mask | Blue | 60 | high |
| Earthen Avatar (680421) | Earthen Avatar: -20 sec cooldown | cooldown -20 s | mask | Epic | 100 | high |
| Judgement of The Three Hammers (520468) | Judgement of The Three Hammers: +10% damage | damage +10 % | spell_id | Blue | 100 | low |
| Mountain Fury (806186) | Mountain Fury: +10% damage | damage +10 % | spell_id | Blue | 100 | medium |
| Mountain Hammer (681130) | Mountain Hammer: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | high |
| Quake (803974) | Quake: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Seismic Smash (300693) | Seismic Smash: +8% damage | damage +8 % | mask | Blue | 100 | high |

### Geomancy

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Cave In (500615) | Cave In: -3 sec cooldown | cooldown -3 s | hook_cd | Blue | 100 | medium |
| Earth's Embrace (806143) | Earth's Embrace: -30 sec cooldown | cooldown -30 s | mask | Epic | 80 | high |
| Golem Form (805335) | Golem Form: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | medium |
| Magma Fissure (802793) | Magma Fissure: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Magma Geode (803140) | Eruption: +10% damage | damage +10 % | mask | Blue | 100 | high |
| Seismic Tremor (680442) | Seismic Tremor: +10% periodic damage | dot +10 % | mask | Blue | 100 | high |
| Terrasurge (681119) | Terrasurge: +8% damage | damage +8 % | mask | Blue | 100 | high |

<details><summary>Avvist (5)</summary>

| Spell | Grunn |
|---|---|
| Flourishing Growth (805442) | raid buff; duration (20 sec) already exceeds cooldown (10 sec), nothing meaningful to modify |
| Primal Awakening (807560) | raid haste with 300 sec Worn Out on allies; cooldown reduction is meaningless, no other numeric op |
| Primal Shred (500940) | pet force-casts 504669; damage by creature, player spellmods do not apply |
| Primal Totem (504229) | summon (is_summon=self) |
| Primal Weapons (537218) | weapon-based buff toggle; utility without numbers to modify |

</details>

## 32 SPIRITMAGE

### Class

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Convergence (560241) | Convergence: +10% damage | damage +10 % | spell_id | Blue | 100 | medium |
| Glacial Rune (805730) | Glacial Rune: +10% damage | damage +10 % | spell_id | Blue | 100 | medium |
| Granite Resolve (520229) | Granite Resolve: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | high |
| Permafrost Rune (804060) | Permafrost Rune: -8 sec cooldown | cooldown -8 s | mask | Blue | 100 | medium |
| Phase Out (500671) | Phase Out: -15 sec cooldown | cooldown -15 s | mask | Blue | 100 | high |
| Power Engraving (500296) | Power Engraving: -30 sec cooldown | cooldown -30 s | mask | Epic | 100 | high |
| Speed Rune (801103) | Speed Rune: -15 sec cooldown | cooldown -15 s | mask | Blue | 60 | high |
| Warding Rune (804232) | Warding Rune: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 100 | medium |
| Warpdagger (500287) | Warpdagger: -4 sec cooldown | cooldown -4 s | mask | Blue | 100 | high |
| Zenith (712325) | Zenith: -6 sec cooldown | cooldown -6 s | mask | Blue | 100 | high |

### Runic

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Fist of the Ancients (712326) | Fist of the Ancients: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Fists of Power (805796) | Fists of Power: -6 sec cooldown | cooldown -6 s | hook_cd | Blue | 100 | medium |
| Guarding Rune (500464) | Guarding Rune: -15 sec cooldown | cooldown -15 s | hook_cd | Blue | 80 | medium |
| Ley Power (801096) | Ley Power: -10 sec cooldown | cooldown -10 s | hook_cd | Blue | 100 | medium |
| Runic Brand (712322) | Runic Brand: +10% damage | damage +10 % | mask | Blue | 100 | high |
| Runic Tempest (560036) | Runic Tempest: -20 sec cooldown | cooldown -20 s | hook_cd | Epic | 100 | medium |

### Arcane

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Eye of the Beholder (500121) | Eye of the Beholder: -20 sec cooldown | cooldown -20 s | hook_cd | Epic | 100 | medium |
| Frigid Blast (500118) | Frigid Blast: -0.15 sec cast time | casting time -0.15 s | mask | Blue | 80 | high |
| Frigid Blast (500118) | Frigid Blast: +8% damage | damage +8 % | mask | Blue | 100 | high |
| Glyphic Overload (520099) | Glyphic Overload: -6 sec cooldown | cooldown -6 s | hook_cd | Blue | 100 | medium |
| Glyphic Ruin (801179) | Glyphic Ruin: -0.25 sec cast time | casting time -0.25 s | spell_id | Blue | 100 | medium |
| Glyphic Ruin (801179) | Glyphic Ruin: +8% damage | damage +8 % | spell_id | Epic | 100 | medium |
| Manuscription (525302) | Manuscription: +10% periodic damage | dot +10 % | spell_id | Blue | 100 | medium |
| Primordial Pulse (520111) | Primordial Pulse: +10% damage | damage +10 % | spell_id | Blue | 100 | low |
| Silencing Rune (802631) | Silencing Rune: -10 sec cooldown | cooldown -10 s | mask | Blue | 100 | medium |
| Thaumaturgy (804550) | Thaumaturgy: -2 sec cooldown | cooldown -2 s | mask | Blue | 100 | high |
| Thaumaturgy (804550) | Thaumaturgy: +8% damage | damage +8 % | mask | Blue | 100 | high |

### Riftblade

| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |
|---|---|---|---|---|---|---|
| Fracture (803018) | Fracture: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Genesis (500501) | Genesis: -20 sec cooldown | cooldown -20 s | hook_cd | Epic | 100 | medium |
| Hoarfrost (801104) | Hoarfrost: +10% periodic damage | dot +10 % | spell_id | Blue | 100 | medium |
| Hurricane (645435) | Hurricane: -4 sec cooldown | cooldown -4 s | mask | Blue | 100 | high |
| Primordial Fury (806543) | Primordial Fury: -10 sec cooldown | cooldown -10 s | mask | Blue | 100 | high |
| Smolder (801087) | Smolder: +8% damage | damage +8 % | spell_id | Blue | 100 | medium |
| Turbulence (706526) | Turbulence: +10% damage | damage +10 % | spell_id | Blue | 100 | medium |

<details><summary>Avvist (1)</summary>

| Spell | Grunn |
|---|---|
| Echo Rune (500270) | summon (is_summon=self) |

</details>
