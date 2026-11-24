#!/usr/bin/env python3
"""Render out/reviews/*.json as a Markdown overview (default: ../../SKILL_AFFIXES.md)."""
import collections
import glob
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', '..', 'SKILL_AFFIXES.md')
QUAL = {3: 'Blue', 4: 'Epic', 5: 'Legendary'}
MS_OPS = {'COOLDOWN', 'CASTING_TIME', 'DURATION'}


def fmt_effect(fx):
    op, ty, v = fx['op'], fx['type'], fx['value']
    if ty == 'PCT':
        val = '%+d %%' % v
    elif op in MS_OPS:
        val = '%+g s' % (v / 1000.0)
    else:
        val = '%+d' % v
    return '%s %s' % (op.replace('_', ' ').lower(), val)


def cell(s):
    return str(s).replace('|', '\\|').replace('\n', ' ')


def main():
    reviews = sorted(glob.glob(os.path.join(HERE, 'out', 'reviews', '[0-9]*.json')))
    lines = ['# CoA class skill affixes: oversikt', '',
             'Generert av `tools/skill_affixes/gen_overview.py` fra `tools/skill_affixes/out/reviews/*.json`.',
             'Status: forslag (ikke i DB/build). Plan og beslutninger: `COA_SKILL_AFFIX_PLAN.md`.', '',
             'Targeting: `mask` = unik family-bit (klient får spellmod), `spell_id` = server-side på spell-id,',
             '`hook_cd` = cooldown kortes ned etter cast. Verdier er per affix før crit roll; ingen stack-tak.', '']
    summary, body = [], []
    tot = collections.Counter()
    for path in reviews:
        doc = json.load(open(path))
        cid, cls = doc['class_id'], doc['class']
        cand = json.load(open(os.path.join(HERE, 'out', 'candidates', os.path.basename(path))))
        specs = {int(k): v.title() for k, v in cand.get('specs', {}).items()}
        specs[0] = 'Class'
        names = {c['id']: c['name'] for c in cand['candidates']}
        for c in cand['candidates']:
            for k in c.get('children', []):
                names.setdefault(k['id'], k.get('name', str(k['id'])))
        affs, rej = doc['affixes'], doc.get('rejected', [])
        modes = collections.Counter(a['targeting'] for a in affs)
        tot.update(modes)
        tot['affixes'] += len(affs)
        tot['rejected'] += len(rej)
        anchor = '%d-%s' % (cid, cls.lower())
        summary.append('| [%s](#%s) | %d | %d | %d | %d | %d |' % (
            cls.title(), anchor, len(affs), modes['mask'], modes['spell_id'], modes['hook_cd'], len(rej)))
        body += ['', '## %d %s' % (cid, cls), '']
        by_spec = collections.defaultdict(list)
        for a in affs:
            by_spec[a['spec_id']].append(a)
        for sid in sorted(by_spec, key=lambda x: (x != 0, x)):
            body += ['### %s' % specs.get(sid, 'Spec %d' % sid), '',
                     '| Spell | Affix | Effekt | Targeting | Min. kvalitet | Weight | Confidence |',
                     '|---|---|---|---|---|---|---|']
            for a in sorted(by_spec[sid], key=lambda a: (names.get(a['target_spell'], ''), a['key'])):
                body.append('| %s (%d) | %s | %s | %s | %s | %d | %s |' % (
                    cell(names.get(a['target_spell'], '?')), a['target_spell'], cell(a['name']),
                    '; '.join(fmt_effect(fx) for fx in a['effects']), a['targeting'],
                    QUAL.get(a['min_quality'], a['min_quality']), a['weight'], a.get('confidence', '')))
            body.append('')
        if rej:
            body += ['<details><summary>Avvist (%d)</summary>' % len(rej), '',
                     '| Spell | Grunn |', '|---|---|']
            for r in sorted(rej, key=lambda r: names.get(r['spell'], '')):
                body.append('| %s (%d) | %s |' % (cell(names.get(r['spell'], '?')), r['spell'], cell(r['reason'])))
            body += ['', '</details>']
    lines += ['## Sammendrag', '', '| Klasse | Affixer | mask | spell_id | hook_cd | Avvist |', '|---|---|---|---|---|---|']
    lines += summary
    lines.append('| **Totalt** | **%d** | %d | %d | %d | %d |' % (
        tot['affixes'], tot['mask'], tot['spell_id'], tot['hook_cd'], tot['rejected']))
    lines += body
    with open(OUT, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    print('wrote %s (%d affixes)' % (os.path.normpath(OUT), tot['affixes']))


if __name__ == '__main__':
    main()
