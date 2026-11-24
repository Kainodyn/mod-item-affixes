#!/usr/bin/env python3
"""Free SpellFamilyFlags bits per CoA family vs. the spells that need a unique bit (COA_SKILL_TOOLTIP_PLAN.md).

Read-only: server Spell.dbc, SELECTs on acore_world (spell_dbc, spell_proc, spell_proc_event) and a grep of the
core C++ for family-flag literals. Prints a table per family (18..38) and optionally writes JSON.

A bit is "used" in family F when any of these hold:
  - some spell with SpellFamilyName F (Spell.dbc, or its spell_dbc row, which replaces the whole row) has it in
    SpellFamilyFlags;
  - some spell with SpellFamilyName F has it in an EffectSpellClassMask (IsAffected() only matches the mask's own
    family), unless the mask is a wildcard (>= WILDCARD_BITS bits set; listed separately);
  - spell_proc / spell_proc_event with SpellFamilyName F masks it;
  - a C++ literal next to SpellFamilyFlags/SpellClassMask/flag96/HasFlag names it, in family F if the line or the
    preceding lines say `SpellFamilyName == F` (or the family is otherwise unknown: then it counts for all CoA
    families, conservatively).
A target set (affected_spells of one spell_id/hook_cd affix) "has a unique bit" when some bit is on every member
and on no other spell of the family. Otherwise it needs one new bit (sets with identical members share one).
"""
import argparse, collections, glob, json, os, re, struct, subprocess, sys

COA_DBC_APP = '/opt/coa/azerothcore/apps/coa-dbc'
CORE_SRC = '/opt/coa/azerothcore/src/server'
sys.path.insert(0, COA_DBC_APP)
import schemas  # noqa: E402
from client_dbc import Table  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
FAMILIES = range(18, 39)
WILDCARD_BITS = 48
NAMED_FAMILIES = {'SPELLFAMILY_GENERIC': 0, 'SPELLFAMILY_MAGE': 3, 'SPELLFAMILY_WARRIOR': 4,
                  'SPELLFAMILY_WARLOCK': 5, 'SPELLFAMILY_PRIEST': 6, 'SPELLFAMILY_DRUID': 7,
                  'SPELLFAMILY_ROGUE': 8, 'SPELLFAMILY_HUNTER': 9, 'SPELLFAMILY_PALADIN': 10,
                  'SPELLFAMILY_SHAMAN': 11, 'SPELLFAMILY_POTION': 13, 'SPELLFAMILY_DEATHKNIGHT': 15,
                  'SPELLFAMILY_PET': 17}


def bitset(words):
    return {w * 32 + b for w in range(3) for b in range(32) if (words[w] & 0xFFFFFFFF) >> b & 1}


def bitname(bit):
    w, b = divmod(bit, 32)
    return 'w%d:0x%X' % (w, 1 << b)


def mysql(defaults, query):
    p = subprocess.run(['mysql', '--defaults-extra-file=' + defaults, '--batch', '--skip-column-names',
                        'acore_world', '-e', query], capture_output=True, text=True, check=False)
    if p.returncode:
        sys.exit('mysql failed: ' + p.stderr.strip()[:300])
    return [line.split('\t') for line in p.stdout.split('\n') if line]


def read_spells(dbc_dir, defaults):
    """id -> (family, flag bits, [effect mask word triples])"""
    cols = {c.name: c for c in schemas.registry()['Spell'].columns}
    t = Table.read(os.path.join(dbc_dir, 'Spell.dbc'))

    def u(rec, n):
        return struct.unpack_from('<I', rec, cols[n].offset)[0]
    spells = {}
    for r in range(t.rows):
        rec = t.record(r)
        masks = [[u(rec, 'EffectSpellClassMask[%d]' % (e * 3 + w)) for w in range(3)] for e in range(3)]
        spells[u(rec, 'Id')] = (u(rec, 'SpellFamilyName'),
                                [u(rec, 'SpellFamilyFlags[%d]' % i) for i in range(3)], masks)
    sql_cols = ['ID', 'SpellClassSet', 'SpellClassMask_1', 'SpellClassMask_2', 'SpellClassMask_3'] + \
        ['EffectSpellClassMask%s_%d' % (e, w) for e in 'ABC' for w in (1, 2, 3)]
    overrides = 0
    for row in mysql(defaults, 'SELECT %s FROM spell_dbc' % ','.join(sql_cols)):
        v = [int(x) & 0xFFFFFFFF for x in row]
        overrides += v[0] in spells
        spells[v[0]] = (v[1], v[2:5], [v[5:8], v[8:11], v[11:14]])
    return spells, overrides


HEX = re.compile(r'0x[0-9A-Fa-f]+')
FAM_EQ = re.compile(r'SpellFamilyName\s*(?:==|!=)\s*(\w+)')
TRIPLE = re.compile(r'(?:flag96|HasFlag)\s*\(\s*([^,()]+),\s*([^,()]+),\s*([^,()]+)\)')
INDEXED = re.compile(r'(?:SpellFamilyFlags|SpellClassMask)\s*\[\s*(\d)\s*\]\s*[&|^]?=?\s*[&|]?\s*\(?\s*(0x[0-9A-Fa-f]+)')


def lit(s):
    s = s.strip().rstrip('uUlL')
    try:
        return int(s, 0) & 0xFFFFFFFF
    except ValueError:
        return None


def cpp_bits():
    """family (or None = unknown) -> {bit: [file:line]}"""
    found = collections.defaultdict(lambda: collections.defaultdict(list))
    files = [f for ext in ('cpp', 'h') for f in glob.glob(CORE_SRC + '/**/*.' + ext, recursive=True)]
    for path in files:
        try:
            lines = open(path, encoding='utf-8', errors='replace').read().split('\n')
        except OSError:
            continue
        for i, line in enumerate(lines):
            if not re.search(r'SpellFamilyFlags|SpellClassMask|flag96\s*\(|HasFlag\s*\(', line) or '0x' not in line:
                continue
            if line.lstrip().startswith('//'):
                continue
            fam = None
            for ctx in reversed(lines[max(0, i - 15):i + 1]):
                m = FAM_EQ.search(ctx)
                if m:
                    tok = m.group(1)
                    fam = int(tok) if tok.isdigit() else NAMED_FAMILIES.get(tok)
                    break
            words = []
            for m in TRIPLE.finditer(line):
                vals = [lit(x) for x in m.groups()]
                if all(v is not None for v in vals):
                    words.append(vals)
            for m in INDEXED.finditer(line):
                w = [0, 0, 0]
                w[int(m.group(1))] = lit(m.group(2))
                words.append(w)
            ref = '%s:%d' % (os.path.relpath(path, CORE_SRC), i + 1)
            for w in words:
                bs = bitset(w)
                if len(bs) >= WILDCARD_BITS:
                    continue
                for b in bs:
                    found[fam][b].append(ref)
    return found


def load_targets():
    """family-agnostic list of (class_file, key, targeting, frozenset(affected))"""
    out = []
    for path in sorted(glob.glob(os.path.join(HERE, 'out', 'reviews', '*.json'))):
        for a in json.load(open(path))['affixes']:
            if a['targeting'] in ('spell_id', 'hook_cd'):
                ids = a.get('affected_spells') or [a['target_spell']]
                out.append((os.path.basename(path), a['key'], a['targeting'], frozenset(ids)))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--dbc', default='/opt/coa/server-data/dbc')
    ap.add_argument('--defaults', default='/opt/coa/secrets/my.cnf')
    ap.add_argument('--json', help='write the full report here')
    a = ap.parse_args()

    spells, overrides = read_spells(a.dbc, a.defaults)
    used = {f: collections.defaultdict(set) for f in FAMILIES}   # bit -> sources
    members = {f: collections.defaultdict(set) for f in FAMILIES}  # bit -> spell ids carrying it
    wildcards = {f: [] for f in FAMILIES}
    zero_flag = collections.Counter()
    for sid, (fam, flags, masks) in spells.items():
        if fam not in used:
            continue
        fb = bitset(flags)
        if not fb:
            zero_flag[fam] += 1
        for b in fb:
            used[fam][b].add('flags')
            members[fam][b].add(sid)
        for m in masks:
            mb = bitset(m)
            if len(mb) >= WILDCARD_BITS:
                wildcards[fam].append(sid)
                continue
            for b in mb:
                used[fam][b].add('effmask')
    for table in ('spell_proc', 'spell_proc_event'):
        for fam, m0, m1, m2 in mysql(a.defaults, 'SELECT SpellFamilyName, SpellFamilyMask0, SpellFamilyMask1, '
                                     'SpellFamilyMask2 FROM %s WHERE SpellFamilyName BETWEEN 18 AND 38' % table):
            mb = bitset([int(m0), int(m1), int(m2)])
            if len(mb) >= WILDCARD_BITS:
                wildcards[int(fam)].append(table)
                continue
            for b in mb:
                used[int(fam)][b].add(table)
    for path in sorted(glob.glob(os.path.join(HERE, 'out', 'reviews', '*.json'))):
        for af in json.load(open(path))['affixes']:
            if af['targeting'] == 'mask' and af.get('mask'):
                fam = spells.get(af['target_spell'], (None,))[0]
                if fam in used:
                    for b in bitset(af['mask']):
                        used[fam][b].add('affix_mask')
    cpp = cpp_bits()
    for fam in FAMILIES:
        for src in (cpp.get(fam, {}), cpp.get(None, {})):
            for b in src:
                used[fam][b].add('cpp' if src is cpp.get(fam) else 'cpp?')

    report = {}
    targets = load_targets()
    for fam in FAMILIES:
        fam_ids = {sid for sid, s in spells.items() if s[0] == fam}
        need_sets, covered, foreign = set(), 0, set()
        for cls, key, mode, ids in targets:
            in_fam = ids & fam_ids
            if not in_fam:
                continue
            foreign |= ids - fam_ids
            common = set.intersection(*(bitset(spells[i][1]) for i in in_fam))
            if any(members[fam][b] <= in_fam for b in common):
                covered += 1
            else:
                need_sets.add(frozenset(in_fam))
        free = [b for b in range(96) if b not in used[fam]]
        free_strict = [b for b in free]  # already excludes 'cpp?' (unknown-family literals)
        free_loose = [b for b in range(96) if not (used[fam][b] - {'cpp?'})]
        reclaim = [b for b in range(96) if used[fam][b] == {'flags'}]
        reclaim_loose = [b for b in range(96) if used[fam][b] and not (used[fam][b] - {'flags', 'cpp?'})]
        need_spells = set().union(*need_sets) if need_sets else set()
        report[fam] = dict(
            spells=len(fam_ids), zero_flag_spells=zero_flag[fam], used_bits=len(used[fam]),
            free_bits=len(free_strict), free_bits_ignoring_unknown_cpp=len(free_loose),
            free=[bitname(b) for b in free_strict],
            affixes_already_unique=covered, sets_needing_bit=len(need_sets), spells_needing_bit=len(need_spells),
            fits=len(need_sets) <= len(free_strict),
            reclaimable_flag_only=len(reclaim), reclaimable_ignoring_unknown_cpp=len(reclaim_loose),
            reclaimable=[bitname(b) for b in reclaim],
            fits_with_reclaim=len(need_sets) <= len(free_strict) + len(reclaim), wildcard_masks=sorted(map(str, wildcards[fam])),
            targets_outside_family=sorted(foreign),
            need_sets=[sorted(s) for s in sorted(need_sets, key=sorted)])

    print('spell_dbc rows overriding Spell.dbc: %d; targets from %d spell_id/hook_cd affixes' %
          (overrides, len(targets)))
    print('%-4s %6s %5s %5s %5s %6s %5s %6s %6s %6s %5s %4s %4s %s' % (
        'fam', 'spells', '0flag', 'used', 'free', 'free*', 'recl', 'recl*', 'uniq', 'sets', 'spls', 'fits', 'fitR',
        'wildcard masks'))
    tot = collections.Counter()
    for fam, r in report.items():
        print('%-4d %6d %5d %5d %5d %6d %5d %6d %6d %6d %5d %4s %4s %d' % (
            fam, r['spells'], r['zero_flag_spells'], r['used_bits'], r['free_bits'],
            r['free_bits_ignoring_unknown_cpp'], r['reclaimable_flag_only'], r['reclaimable_ignoring_unknown_cpp'],
            r['affixes_already_unique'], r['sets_needing_bit'], r['spells_needing_bit'],
            'yes' if r['fits'] else 'NO', 'yes' if r['fits_with_reclaim'] else 'NO', len(r['wildcard_masks'])))
        for k in ('sets_needing_bit', 'spells_needing_bit', 'affixes_already_unique'):
            tot[k] += r[k]
        tot['fits'] += r['fits']
        tot['fitsR'] += r['fits_with_reclaim']
    print('total: %d sets / %d spells need a new bit; %d affixes already have a unique bit; %d/21 families fit, '
          '%d/21 with reclaimed flag-only bits' % (tot['sets_needing_bit'], tot['spells_needing_bit'],
                                                  tot['affixes_already_unique'], tot['fits'], tot['fitsR']))
    print('free = no flags/effmask/proc/C++ use (unknown-family C++ literals count for every family); '
          'free* = ignoring unknown-family C++ literals')
    print('recl = bits only in SpellFamilyFlags (no mask/proc/C++/affix_mask reference): stripping them from every '
          'spell changes no server match, so they can be reassigned; recl* = same, ignoring unknown-family C++')
    if a.json:
        with open(a.json, 'w') as f:
            json.dump({'cpp_unknown_family_bits': {bitname(b): refs[:5] for b, refs in cpp.get(None, {}).items()},
                       'families': report}, f, indent=1)
        print('wrote', a.json)


if __name__ == '__main__':
    main()
