#!/usr/bin/env bash
# mod-item-affixes -- UPDATE: All
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SCRIPTS_ROOT="$SCRIPT_DIR/.."
MODULE_ROOT="$SCRIPT_DIR/../.."
SQL_WORLD="$MODULE_ROOT/data/sql/db-world"
SQL_IMPRINTS="$SQL_WORLD/imprints"

echo "============================================================"
echo " mod-item-affixes -- UPDATE: All"
echo " Run after: git pull"
echo "============================================================"
echo

CONFIG="$SCRIPTS_ROOT/config.sh"
if [ ! -f "$CONFIG" ]; then echo "ERROR: scripts/config.sh not found."; exit 1; fi
source "$CONFIG"

echo "[1/2] Generating and applying affix data..."
if ! command -v pwsh &>/dev/null; then
    echo "ERROR: pwsh (PowerShell Core) not found."
    echo "       affix_template.sql / talent_affix_def.sql are generated from the JSON"
    echo "       definitions and are not part of the repo, so they must be regenerated"
    echo "       after every pull or JSON edit -- otherwise the DB keeps the old data."
    echo "       Install: https://learn.microsoft.com/powershell/scripting/install/installing-powershell-on-linux"
    exit 1
fi
echo "Regenerating SQL from JSON..."
pwsh -File "$SCRIPTS_ROOT/build_affixes.ps1"
pwsh -File "$SCRIPTS_ROOT/build_talent_affixes.ps1"
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_WORLD/affix_template.sql"
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$SQL_WORLD/talent_affix_def.sql"
echo "  Done."
echo

echo "Applying spec-tree data..."
for f in "$SQL_WORLD"/affix_spec_tree_*.sql; do
    [ -e "$f" ] || continue
    mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$f"
done
echo "  Done."
echo

echo "[2/2] Applying imprint data..."
for f in "$SQL_IMPRINTS"/*.sql; do
    [ -e "$f" ] || continue
    # imprint_rune_item.sql (singular) is a legacy single-entry (item 601001)
    # superseded by imprint_rune_items.sql (plural, current 602001+ set) --
    # re-applying it would reintroduce a dead item_template row.
    [ "$(basename "$f")" = "imprint_rune_item.sql" ] && continue
    echo "  Applying $(basename "$f")..."
    mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$DB_WORLD" < "$f"
done
echo "  Done. (Client patch rebuild requires Windows -- see update/client-patch.bat)"
echo

echo "All updates applied. Restart worldserver to apply changes."
