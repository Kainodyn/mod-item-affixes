#!/usr/bin/env python3
"""Merge the reviewer files out/reviews/*.json into the world SQL for CoA class skill affixes.

Requires validate_reviews.py to pass. Writes data/sql/db-world/mod_item_affixes_template_skill.sql and keeps the
stable id registry affix_ids.json up to date (an affix key keeps its id forever; new keys get the next free id of
their class block 100000 + class_id * 1000 + n; ids of removed keys are never reused).

Usage: merge_reviews.py [--check-db] [--defaults /opt/coa/secrets/my.cnf]
  --check-db  compare every column the SQL writes against DESCRIBE of the live tables (SELECT/DESCRIBE only).
"""
import argparse, glob, json, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
MODULE = os.path.normpath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, HERE)
import validate_reviews  # noqa: E402

OUT_SQL = os.path.join(MODULE, 'data', 'sql', 'db-world', 'mod_item_affixes_template_skill.sql')
ID_FILE = os.path.join(HERE, 'affix_ids.json')
CORE_SPELLINFO = '/opt/coa/azerothcore/src/server/game/Spells/SpellInfo.cpp'

CARRIER_SPELL = 9979001
ID_BASE, ID_CLASS_STRIDE = 100000, 1000
STAT_ID_MAX = 99999
MAX_NAME = 56  # C++ caps the shown text at 60 (MAX_SKILL_TEXT); the crit roll may add digits

# SpellModOp (src/server/game/Spells/SpellDefines.h)
OP_IDS = {'DAMAGE': 0, 'DURATION': 1, 'EFFECT1': 3, 'RANGE': 5, 'RADIUS': 6, 'CRITICAL_CHANCE': 7, 'ALL_EFFECTS': 8,
          'CASTING_TIME': 10, 'COOLDOWN': 11, 'EFFECT2': 12, 'COST': 14, 'JUMP_TARGETS': 17, 'DOT': 22, 'EFFECT3': 23}
TYPE_IDS = {'FLAT': 0, 'PCT': 1}
TARGETING_IDS = {'mask': 0, 'spell_id': 1, 'hook_cd': 2}
MS_OPS = {'COOLDOWN', 'CASTING_TIME', 'DURATION'}

NEW_COLUMNS = [  # (name, definition) added to affix_template, in order
    ('affix_key', "VARCHAR(64) NOT NULL DEFAULT '' COMMENT 'skill affixes: review key'"),
    ('affix_type', "TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '0 = class skill, 1 = stat'"),
    ('class_id', 'TINYINT UNSIGNED NOT NULL DEFAULT 0'),
    ('spec_id', 'SMALLINT UNSIGNED NOT NULL DEFAULT 0'),
    ('target_spell', 'INT UNSIGNED NOT NULL DEFAULT 0'),
    ('known_spell', "INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'spell the player must know to roll it'"),
    ('targeting', "TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 mask, 1 spell_id, 2 hook_cd'"),
    ('mask0', 'INT UNSIGNED NOT NULL DEFAULT 0'),
    ('mask1', 'INT UNSIGNED NOT NULL DEFAULT 0'),
    ('mask2', 'INT UNSIGNED NOT NULL DEFAULT 0'),
]
for _n in (1, 2, 3):
    NEW_COLUMNS += [('op%d' % _n, "TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'SpellModOp'"),
                    ('type%d' % _n, "TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 flat, 1 pct'"),
                    ('value%d' % _n, "INT NOT NULL DEFAULT 0 COMMENT '0 = unused'")]
NEW_COLUMNS.append(('scales_with_crit', 'TINYINT UNSIGNED NOT NULL DEFAULT 0'))
OLD_COLUMNS = ['id', 'name', 'weight', 'stat_op', 'role_mask', 'item_category', 'min_quality', 'requires_mana']
INSERT_COLUMNS = ['id', 'name', 'weight', 'stat_op', 'role_mask', 'item_category', 'min_quality', 'requires_mana',
                  'affix_key', 'affix_type', 'class_id', 'spec_id', 'target_spell', 'known_spell', 'targeting',
                  'mask0', 'mask1', 'mask2', 'op1', 'type1', 'value1', 'op2', 'type2', 'value2', 'op3', 'type3',
                  'value3', 'scales_with_crit']
# Same pattern as the existing passive hidden dummy auras in spell_dbc (e.g. 9978010).
CARRIER_ROW = [('ID', CARRIER_SPELL), ('Attributes', 448), ('CastingTimeIndex', 1), ('DurationIndex', 21),
               ('RangeIndex', 1), ('EquippedItemClass', -1), ('Effect_1', 6), ('ImplicitTargetA_1', 1),
               ('EffectAura_1', 4), ('Name_Lang_enUS', 'Item Affix Skill Carrier'), ('Name_Lang_Mask', 16712190),
               ('SchoolMask', 1)]


def sql_str(s):
    return "'" + s.replace('\\', '\\\\').replace("'", "''") + "'"


def display_number(op, ty, value):
    v = abs(value)
    if op in MS_OPS and ty == 'FLAT':
        return ('%d' % (v // 1000)) if v % 1000 == 0 else ('%.2f' % (v / 1000)).rstrip('0').rstrip('.')
    return '%d' % v


def name_format(a):
    fx = a['effects'][0]
    num = display_number(fx['op'], fx['type'], fx['value'])
    prefix, sep, rest = a['name'].partition(': ')
    m = re.search(r'(?<![\d.])' + re.escape(num) + r'(?![\d.])', rest)
    if not sep or not m or '$' in a['name']:
        return None
    return prefix + sep + rest[:m.start()] + '$v' + rest[m.end():]


def spellmod_case_ids():
    try:
        src = open(CORE_SPELLINFO, encoding='utf-8').read()
    except OSError:
        return None
    s = src.index('bool SpellInfo::IsAffectedBySpellMod')
    e = src.index('\n}\n', s)
    body = src[s:e]
    return {int(x) for x in re.findall(r'case (\d+)', body)} | {int(x) for x in re.findall(r'mod->spellId == (\d+)', body)}


def load_ids():
    if not os.path.exists(ID_FILE):
        return {}
    return {k: int(v) for k, v in json.load(open(ID_FILE, encoding='utf-8'))['ids'].items()}


def assign_ids(ids, affixes):
    for class_id, a in affixes:
        if a['key'] in ids:
            continue
        lo = ID_BASE + class_id * ID_CLASS_STRIDE
        used = [v for v in ids.values() if lo <= v < lo + ID_CLASS_STRIDE]
        nxt = max(used) + 1 if used else lo
        if nxt >= lo + ID_CLASS_STRIDE:
            sys.exit('id block of class %d is full' % class_id)
        ids[a['key']] = nxt
    for k, v in ids.items():
        if v <= STAT_ID_MAX:
            sys.exit('skill affix id %d (%s) collides with the stat id range' % (v, k))
    if len(set(ids.values())) != len(ids):
        sys.exit('duplicate ids in %s' % ID_FILE)
    return ids


def mysql_columns(defaults, table):
    p = subprocess.run(['mysql', '--defaults-extra-file=' + defaults, '--batch', '--skip-column-names', 'acore_world',
                        '-e', 'DESCRIBE `%s`' % table], capture_output=True, text=True)
    if p.returncode:
        sys.exit('mysql failed: ' + p.stderr.strip()[:300])
    return [line.split('\t')[0] for line in p.stdout.splitlines() if line]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--check-db', action='store_true')
    ap.add_argument('--defaults', default='/opt/coa/secrets/my.cnf')
    args = ap.parse_args()

    v = subprocess.run([sys.executable, '-B', os.path.join(HERE, 'validate_reviews.py')], capture_output=True, text=True)
    if v.returncode:
        print(v.stdout[-3000:])
        sys.exit('validate_reviews.py failed; fix the reviews first')

    cand_dir = os.path.join(HERE, 'out', 'candidates')
    case_ids = spellmod_case_ids()
    affixes, errs = [], []
    targets = {}
    for path in sorted(glob.glob(os.path.join(HERE, 'out', 'reviews', '*.json'))):
        if os.path.basename(path).startswith('_'):
            continue
        doc = json.load(open(path, encoding='utf-8'))
        class_id = doc['class_id']
        if not 12 <= class_id <= 32:
            errs.append('%s: class_id %d outside CoA classes 12-32' % (path, class_id))
            continue
        _, spells = validate_reviews.load_candidates(cand_dir, class_id)
        for a in doc['affixes']:
            tgt = spells[a['target_spell']]
            known = tgt['_parent'] if tgt['_kind'] == 'child' else tgt['ranks'][0]
            fmt = name_format(a)
            if not fmt:
                errs.append('%s: name "%s" does not show effect[0] value %s' % (
                    a['key'], a['name'], display_number(a['effects'][0]['op'], a['effects'][0]['type'],
                                                        a['effects'][0]['value'])))
            if '|' in a['name'] or len(a['name']) > MAX_NAME:
                errs.append('%s: name must not contain "|" and be <= %d chars (addon payload)' % (a['key'], MAX_NAME))
            if a['targeting'] == 'mask' and case_ids and a['target_spell'] in case_ids:
                errs.append('%s: target_spell %d is special-cased in SpellInfo::IsAffectedBySpellMod' % (
                    a['key'], a['target_spell']))
            ids = list(a['affected_spells']) or [a['target_spell']]
            expanded = []
            for sid in ids:
                for r in (spells[sid].get('ranks') if spells[sid]['_kind'] == 'candidate' else None) or [sid]:
                    if r not in expanded:
                        expanded.append(r)
            targets[a['key']] = expanded
            affixes.append((class_id, dict(a, _known=known, _fmt=fmt or a['name'])))
    if errs:
        print('\n'.join(errs))
        sys.exit('%d error(s)' % len(errs))

    ids = assign_ids(load_ids(), affixes)

    lines = [
        '-- mod-item-affixes (CoA): class skill affixes. GENERATED by tools/skill_affixes/merge_reviews.py from',
        '-- tools/skill_affixes/out/reviews/*.json; do not edit by hand. Re-applied by the DB updater whenever it',
        '-- changes, so every statement is idempotent. Sorts after mod_item_affixes_template.sql, which recreates',
        '-- affix_template: if that file changes, this one must be re-applied too (delete its row from `updates`).',
        '--',
        '-- affix_type 0 = class skill (1 = stat, the default for the rows of mod_item_affixes_template.sql)',
        '-- targeting  0 = mask (mod->spellId = target_spell, mask0..2 = unique family bit)',
        '--            1 = spell_id (carrier spell, mask 0, targetSpellId per affix_skill_target row)',
        '--            2 = hook_cd (cooldown cut after the cast via Player::ModifySpellCooldown)',
        '-- opN/typeN/valueN  SpellModOp, 0 flat / 1 pct, value (0 = unused effect)',
        '-- name       "$v" is replaced by the rolled value of effect 1',
        '-- id         stable: tools/skill_affixes/affix_ids.json, 100000 + class_id * 1000 + n',
        '',
    ]
    for col, definition in NEW_COLUMNS:
        lines += [
            "SET @sql := IF(NOT EXISTS (SELECT * FROM `information_schema`.`COLUMNS` WHERE `TABLE_SCHEMA` = DATABASE() "
            "AND `TABLE_NAME` = 'affix_template' AND `COLUMN_NAME` = '%s'), "
            "'ALTER TABLE `affix_template` ADD COLUMN `%s` %s', 'SELECT 1');" % (col, col, definition.replace("'", "''")),
            'PREPARE `stmt` FROM @sql;',
            'EXECUTE `stmt`;',
            'DEALLOCATE PREPARE `stmt`;',
        ]
    lines += [
        '',
        'CREATE TABLE IF NOT EXISTS `affix_skill_target` (',
        '  `affix_id` INT UNSIGNED NOT NULL,',
        '  `spell_id` INT UNSIGNED NOT NULL,',
        '  PRIMARY KEY (`affix_id`, `spell_id`)',
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod-item-affixes: spells a skill affix affects';",
        '',
        'DELETE FROM `spell_dbc` WHERE `ID` = %d;' % CARRIER_SPELL,
        'INSERT INTO `spell_dbc` (%s) VALUES' % ', '.join('`%s`' % c for c, _ in CARRIER_ROW),
        '(%s);' % ', '.join(sql_str(v) if isinstance(v, str) else str(v) for _, v in CARRIER_ROW),
        '',
        'DELETE FROM `affix_template` WHERE `affix_type` = 0;',
        'INSERT INTO `affix_template` (%s) VALUES' % ', '.join('`%s`' % c for c in INSERT_COLUMNS),
    ]
    rows, trows = [], []
    for class_id, a in sorted(affixes, key=lambda x: ids[x[1]['key']]):
        aid = ids[a['key']]
        mask = a.get('mask') or [0, 0, 0]
        if a['targeting'] != 'mask':
            mask = [0, 0, 0]
        fx = []
        for e in a['effects'] + [None] * (3 - len(a['effects'])):
            fx += [0, 0, 0] if e is None else [OP_IDS[e['op']], TYPE_IDS[e['type']], e['value']]
        vals = [aid, sql_str(a['_fmt']), a['weight'], 0, 0, 0, a['min_quality'], 0, sql_str(a['key']), 0, class_id,
                a['spec_id'], a['target_spell'], a['_known'], TARGETING_IDS[a['targeting']]] + mask + fx + \
               [1 if a['scales_with_crit'] else 0]
        rows.append('(%s)' % ', '.join(str(x) for x in vals))
        trows += ['(%d, %d)' % (aid, s) for s in targets[a['key']]]
    lines += [',\n'.join(rows) + ';', '', 'DELETE FROM `affix_skill_target`;',
              'INSERT INTO `affix_skill_target` (`affix_id`, `spell_id`) VALUES', ',\n'.join(trows) + ';', '']

    os.makedirs(os.path.dirname(OUT_SQL), exist_ok=True)
    with open(OUT_SQL, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines))
    with open(ID_FILE, 'w', encoding='utf-8', newline='\n') as f:
        json.dump({'_comment': 'Stable skill affix ids (merge_reviews.py). Never renumber or reuse an id.',
                   'ids': dict(sorted(ids.items(), key=lambda kv: kv[1]))}, f, indent=2)
        f.write('\n')
    print('wrote %s: %d affixes, %d target rows, carrier %d' % (OUT_SQL, len(rows), len(trows), CARRIER_SPELL))

    if args.check_db:
        bad = 0
        have = mysql_columns(args.defaults, 'affix_template')
        for c in OLD_COLUMNS:
            if c not in have:
                print('affix_template lacks existing column %s' % c); bad += 1
        new = [c for c, _ in NEW_COLUMNS]
        for c in INSERT_COLUMNS:
            if c not in have and c not in new:
                print('affix_template: unknown column %s' % c); bad += 1
        print('affix_template: %d insert columns, %d already present, %d added by this file' % (
            len(INSERT_COLUMNS), sum(c in have for c in INSERT_COLUMNS), sum(c not in have for c in INSERT_COLUMNS)))
        sd = mysql_columns(args.defaults, 'spell_dbc')
        for c, _ in CARRIER_ROW:
            if c not in sd:
                print('spell_dbc: unknown column %s' % c); bad += 1
        print('spell_dbc: %d/%d carrier columns exist' % (sum(c in sd for c, _ in CARRIER_ROW), len(CARRIER_ROW)))
        if bad:
            sys.exit('%d column error(s)' % bad)
    return 0


if __name__ == '__main__':
    sys.exit(main())
