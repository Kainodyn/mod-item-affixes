#!/bin/sh
# mod-item-affixes client patcher (macOS / Linux). Close World of Warcraft first.
# Run after the mod-classless-wildcard installer, and again after every run of it.
cd "$(dirname "$0")" || exit 1
exec python3 ./ItemAffixesPatch.py "$@"
