#!/usr/bin/env python3
"""How far CASTING_TIME / DURATION / RANGE / RADIUS affixes miss when the streamed Spell row must point at an
existing SpellCastTimes / SpellDuration / SpellRange / SpellRadius row.

For every affix and every affected spell with a base value, computes the wanted value for 1..3 stacked copies,
the nearest existing row value and the miss. Prints a summary and writes a JSON report (argv[1], optional).
"""
import collections
import glob
import json
import os
import struct
import sys

sys.path.insert(0, '/opt/coa/azerothcore/apps/coa-dbc')
from client_dbc import Table  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
DBC = '/opt/coa/server-data/dbc/'
OPS = {'CASTING_TIME': ('SpellCastTimes', 'cast', 'i', 4, 'ms'),
       'DURATION': ('SpellDuration', 'dur', 'i', 4, 'ms'),
       'RANGE': ('SpellRange', 'range', 'f', 12, 'yd'),
       'RADIUS': ('SpellRadius', 'radius', 'f', 4, 'yd')}


def table_values(name, kind, off):
    t = Table.read(DBC + name + '.dbc')
    vals = set()
    for r in range(t.rows):
        v = struct.unpack_from('<' + kind, t.record(r), off)[0]
        if v > 0:
            vals.add(round(v, 3))
    return sorted(vals)


def main():
    rows = {op: table_values(t, k, o) for op, (t, _, k, o, _) in OPS.items()}
    spells = {}
    for f in glob.glob(os.path.join(HERE, 'out', 'candidates', '*.json')):
        for c in json.load(open(f))['candidates']:
            for s in [c] + c.get('children', []):
                spells.setdefault(s['id'], s)
    report = []
    for f in sorted(glob.glob(os.path.join(HERE, 'out', 'reviews', '[0-9]*.json'))):
        for a in json.load(open(f))['affixes']:
            for fx in a['effects']:
                if fx['op'] not in OPS:
                    continue
                _, field, _, _, unit = OPS[fx['op']]
                for sid in dict.fromkeys([a['target_spell']] + a['affected_spells']):
                    base = spells.get(sid, {}).get(field)
                    if isinstance(base, list):
                        base = max(base) if base else 0
                    if not base or base <= 0:
                        continue
                    for n in (1, 2, 3):
                        delta = fx['value'] * n if fx['type'] == 'FLAT' else base * fx['value'] * n / 100.0
                        want = max(0.0, base + delta)
                        near = min(rows[fx['op']], key=lambda v: abs(v - want))
                        report.append(dict(key=a['key'], op=fx['op'], spell=sid, n=n, base=base, want=round(want, 3),
                                           nearest=near, miss=round(near - want, 3),
                                           miss_pct_of_bonus=round(100 * abs(near - want) / abs(delta), 1) if delta else 0,
                                           unit=unit))
    for op in OPS:
        print('== %s (%d existing rows)' % (op, len(rows[op])))
        for n in (1, 2, 3):
            rs = [r for r in report if r['op'] == op and r['n'] == n]
            if not rs:
                continue
            exact = sum(1 for r in rs if abs(r['miss']) < 1e-6)
            within = collections.Counter()
            for r in rs:
                p = r['miss_pct_of_bonus']
                within['<=10%' if p <= 10 else '<=25%' if p <= 25 else '<=50%' if p <= 50 else '>50%'] += 1
            worst = max(rs, key=lambda r: r['miss_pct_of_bonus'])
            print('  x%d: %d spells, exact %d, miss of bonus %s; worst %s %s base %s want %s nearest %s' % (
                n, len(rs), exact, dict(within), worst['key'], worst['spell'], worst['base'], worst['want'],
                worst['nearest']))
    if len(sys.argv) > 1:
        json.dump(report, open(sys.argv[1], 'w'), indent=1)


if __name__ == '__main__':
    main()
