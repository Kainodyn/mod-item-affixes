import json, sys, collections
d = json.load(open(sys.argv[1]))
S = {int(k): v for k, v in d['spells'].items()}
CT = {int(k): v for k, v in d['casttime'].items()}
specs = {int(k): v for k, v in d['specs'].items()}
DMG = {2, 58, 17, 31, 121, 9}  # school dmg, weapon dmg variants, health leech
HEAL = {10, 136, 67}
AURA_DMG = {3, 53, 89}
AURA_HEAL = {8, 20}
fam_index = collections.defaultdict(list)
for sp in S.values():
    if sp['fam'] and any(sp['flags']):
        fam_index[sp['fam']].append(sp)

def kind(sp):
    k = set()
    for e, a in zip(sp['eff'], sp['aura']):
        if e in DMG: k.add('dmg')
        if e in HEAL: k.add('heal')
        if e == 6 and a in AURA_DMG: k.add('dot')
        if e == 6 and a in AURA_HEAL: k.add('hot')
        if e == 6 and a == 69: k.add('absorb')
        if e in (64, 142) or (e == 6 and a in (23, 226)): k.add('trigger')
        if e in (28, 41, 42, 56): k.add('summon')
    return k

by_class = collections.defaultdict(list)
for e in d['entries']:
    by_class[e['cls']].append(e)

summary = {}
for cls in sorted(by_class):
    es = by_class[cls]
    tok = d['classtok'][str(cls)]
    fams = collections.Counter()
    active = []
    for e in es:
        if not e['ranks']:
            continue
        sp = S.get(e['ranks'][0])
        if not sp:
            continue
        fams[sp['fam']] += 1
        passive = bool(sp['attr'] & 0x40)
        if not passive:
            active.append((e, sp))
    ab = [x for x in es if x['type'] == 'Ability']
    # collision analysis on active abilities+talents with flags
    zero = 0; unique = 0; overl = []
    for e, sp in active:
        f = sp['flags']
        if not any(f):
            zero += 1; continue
        same = [o for o in fam_index[sp['fam']] if o['id'] != sp['id'] and any(a & b for a, b in zip(f, o['flags']))]
        names = {o['name'] for o in same} - {sp['name']}
        if not names:
            unique += 1
        overl.append(len(names))
    dmgheal = sum(1 for e, sp in active if kind(sp) & {'dmg', 'heal', 'dot', 'hot', 'absorb'})
    cds = sum(1 for e, sp in active if sp['cd'] or sp['catcd'])
    casts = sum(1 for e, sp in active if CT.get(sp['cast_idx'], 0) > 0)
    nspec = len({e['spec'] for e in es if e['spec']})
    overl.sort()
    summary[cls] = dict(tok=tok, entries=len(es), abilities=len(ab), talents=len(es) - len(ab), specs=nspec,
                        active=len(active), dmgheal=dmgheal, with_cd=cds, with_cast=casts,
                        fams=dict(fams.most_common(4)), zeroflag=zero, uniqueflag=unique,
                        median_overlap=overl[len(overl) // 2] if overl else None, max_overlap=overl[-1] if overl else None)
    print(cls, json.dumps(summary[cls]))
json.dump(summary, open(sys.argv[2], 'w'))
