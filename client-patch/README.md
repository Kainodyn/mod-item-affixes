# ItemAffixesPatch — client patcher (classless branch)

Adds mod-item-affixes' custom spells (Imprints and spell-swap variants) to a
WoW 3.3.5a client that already has the
[mod-classless-wildcard](https://github.com/DustinHendrickson/mod-classless-wildcard)
client patch, and installs the `ItemAffixes` and `ItemAffixesToast` addons.

## Use

1. Run the **mod-classless-wildcard** client installer first.
2. Unzip `ItemAffixesPatch.zip` into your WoW folder, so you get
   `World of Warcraft\ItemAffixesPatch\`, beside `Wow.exe`.
3. Close World of Warcraft.
4. Windows: double-click `ItemAffixesPatch\ItemAffixesPatch.bat`.
   macOS/Linux: `./ItemAffixesPatch/ItemAffixesPatch.sh`.

Needs Python 3.7+, the same Python the Wildcard installer uses. Nothing else.

**Run it again every time you run the Wildcard installer.** That installer
rebuilds its archives from the client's original files, which drops these
spells again. Running the patcher twice is safe; it adds nothing that is
already there.

Options: `--dry-run` (show what would change), `--uninstall` (take the spells
and the addons out again), `--no-addon`, and a WoW folder path when the
patcher is not inside it.

## What it changes

The Wildcard installer writes the highest-priority patch archives,
`Data\patch-Z.MPQ` and `Data\<locale>\patch-<locale>-Z.MPQ` (the letter is
read from `ClasslessWildcard-install.json`). They hold its own `Spell.dbc` and
`SkillLineAbility.dbc`, so an archive from this module could not outrank them.
The patcher therefore appends its rows to those two DBCs inside Wildcard's
archives. Every other file in them is copied byte for byte.

- `Spell.dbc`: 35 spells (600002–600003, 699001–699019, 699100–699113).
- `SkillLineAbility.dbc`: 21 rows (50000–50020), the spellbook tabs for them.

The patcher stops without writing anything if one of those ids is already used
by a different row.

## Rebuilding

`affix_dbc_rows.json` is generated; do not edit it. After changing
`imprints/custom_spells.json`:

```
python3 tools/build_client_patch.py /path/to/server/data/dbc
```

This runs upstream's `tools/patch_custom_spells.ps1` (needs `pwsh`) against a
temporary copy of the server DBCs and writes the rows plus
`dist/ItemAffixesPatch.zip`.
