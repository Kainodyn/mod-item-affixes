#!/usr/bin/env bash
# mod-item-affixes -- INSTALL Step 2 of 3: Load Data
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SCRIPTS_ROOT="$SCRIPT_DIR/.."
MODULE_ROOT="$SCRIPT_DIR/../.."
SQL_WORLD="$MODULE_ROOT/data/sql/db-world"
SQL_IMPRINTS="$SQL_WORLD/imprints"

echo "============================================================"
echo " mod-item-affixes -- INSTALL Step 2 of 3: Load Data"
echo " Applies affix, imprint, and spell data to the world DB."
echo "============================================================"
echo

CONFIG="$SCRIPTS_ROOT/config.sh"
if [ ! -f "$CONFIG" ]; then
    echo "ERROR: scripts/config.sh not found."
    exit 1
fi
source "$CONFIG"

# affix_template.sql / talent_affix_def.sql are generated from the JSON
# definitions and are NOT committed (see .gitignore), so pwsh is required
# unless they were already generated earlier on this machine.
if command -v pwsh &>/dev/null; then
    echo "Regenerating SQL from JSON definitions (pwsh found)..."
    pwsh -File "$SCRIPTS_ROOT/build_affixes.ps1"
    pwsh -File "$SCRIPTS_ROOT/build_talent_affixes.ps1"
    echo "  SQL regenerated."
elif [ -f "$SQL_WORLD/affix_template.sql" ] && [ -f "$SQL_WORLD/talent_affix_def.sql" ]; then
    echo "WARNING: pwsh not found -- using previously generated SQL."
    echo "         It may be stale if the JSON definitions changed since it was generated."
else
    echo "ERROR: pwsh (PowerShell Core) not found, and the generated SQL files"
    echo "       affix_template.sql / talent_affix_def.sql do not exist."
    echo "       They are built from JSON and are not part of the repo."
    echo "       Install PowerShell Core: https://learn.microsoft.com/powershell/scripting/install/installing-powershell-on-linux"
    exit 1
fi
echo

echo "Applying world DB data to $DB_WORLD..."
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_WORLD/affix_template.sql"
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_WORLD/talent_affix_def.sql"

echo "Applying spec-tree data to $DB_WORLD..."
for f in "$SQL_WORLD"/affix_spec_tree_*.sql; do
    mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$f"
done

echo "Applying imprint data to $DB_WORLD..."
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_IMPRINTS/imprint_def.sql"
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_IMPRINTS/imprint_rune_items.sql"
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_IMPRINTS/imprint_apply_spell.sql"
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_IMPRINTS/spell_script_names_imprint.sql"
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_IMPRINTS/spell_dbc_celestial_resonance.sql"
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_IMPRINTS/spell_dbc_vanishing_backstab.sql"
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_IMPRINTS/spell_dbc_arcane_shot_variants.sql"
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_IMPRINTS/empyrean_echo_creature.sql"
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_IMPRINTS/holy_nova_beacon_creature.sql"
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_IMPRINTS/imprint_ancient_tiger_creature.sql"
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_IMPRINTS/stampede_rhino_creature.sql"

echo "Applying Reforge Master NPC to $DB_WORLD..."
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_WORLD/reforge_master_npc.sql"
echo "  All data applied."
echo

echo "============================================================"
echo " Step 2 complete. Start the worldserver to verify."
echo " Client patch (MPQ/DBC) is Windows-only:"
echo "   scripts/install/3-patch-client.bat"
echo "============================================================"
