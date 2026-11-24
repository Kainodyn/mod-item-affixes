import sys, struct, json, collections
sys.path.insert(0, '/opt/coa/azerothcore/apps/coa-dbc')
import schemas
from client_dbc import Table

D = '/opt/coa/server-data/dbc/'
OUT = sys.argv[1]

def load(name):
    return Table.read(D + name + '.dbc')

def u(t, rec, i):
    return struct.unpack_from('<I', rec, i * 4)[0]

def s(t, rec, i):
    return t.string(u(t, rec, i)).decode('utf-8', 'replace')

spell_cols = {c.name: (c.offset // 4, c.kind) for c in schemas.registry()['Spell'].columns}
sp = load('Spell')
spells = {}
for r in range(sp.rows):
    rec = sp.record(r)
    def g(n):
        i, k = spell_cols[n]
        if k == 's':
            return s(sp, rec, i)
        v = u(sp, rec, i)
        return struct.unpack('<i', struct.pack('<I', v))[0] if k == 'i' else v
    sid = u(sp, rec, 0)
    spells[sid] = dict(
        id=sid, name=g('SpellName[enUS]'), rank=g('Rank[enUS]'),
        fam=g('SpellFamilyName'), flags=[g('SpellFamilyFlags[%d]' % i) for i in range(3)],
        attr=g('Attributes'), attr3=g('AttributesEx3'),
        cast_idx=g('CastingTimeIndex'), cd=g('RecoveryTime'), catcd=g('CategoryRecoveryTime'), gcd=g('StartRecoveryTime'),
        dur_idx=g('DurationIndex'), power=g('PowerType'), cost=g('ManaCost'), cost_pct=g('ManaCostPercentage'),
        eff=[g('Effect[%d]' % i) for i in range(3)], aura=[g('EffectApplyAuraName[%d]' % i) for i in range(3)],
        bp=[g('EffectBasePoints[%d]' % i) for i in range(3)], radius=[g('EffectRadiusIndex[%d]' % i) for i in range(3)],
        misc=[g('EffectMiscValue[%d]' % i) for i in range(3)], dmgclass=g('DmgClass'))

ct = load('SpellCastTimes'); casttime = {u(ct, ct.record(r), 0): struct.unpack_from('<i', ct.record(r), 4)[0] for r in range(ct.rows)}
du = load('SpellDuration'); duration = {u(du, du.record(r), 0): struct.unpack_from('<i', du.record(r), 4)[0] for r in range(du.rows)}

classes = load('ChrClasses'); classtok = {u(classes, classes.record(r), 0): s(classes, classes.record(r), 55) for r in range(classes.rows)}
cty = load('CharacterAdvancementClassTypes'); ctype = {u(cty, cty.record(r), 0): (u(cty, cty.record(r), 2), u(cty, cty.record(r), 4)) for r in range(cty.rows)}
tt = load('CharacterAdvancementTabTypes'); tabtok = {u(tt, tt.record(r), 0): s(tt, tt.record(r), 1).upper() for r in range(tt.rows)}
cs = load('ChrSpecs')
spec_by = {}; specs = {}
for r in range(cs.rows):
    rec = cs.record(r)
    sid = u(cs, rec, 0)
    spec_by[(s(cs, rec, 1), s(cs, rec, 2))] = sid
    specs[sid] = dict(id=sid, cls=s(cs, rec, 1), tab=s(cs, rec, 2), sig=u(cs, rec, 24), ident=u(cs, rec, 28),
                      roles=[u(cs, rec, i) for i in range(11, 16)])
ca = load('CharacterAdvancement')
entries = []
for r in range(ca.rows):
    rec = ca.record(r)
    t = ctype.get(u(ca, rec, 32))
    if not t or not t[1] or not (12 <= t[0] <= 32):
        continue
    cls = t[0]; tab = u(ca, rec, 33)
    spec = 0
    if tab != 87:
        spec = spec_by.get((classtok.get(cls), tabtok.get(tab)))
        if spec is None:
            continue
    entries.append(dict(entry=u(ca, rec, 0), cls=cls, spec=spec, type=s(ca, rec, 1),
                        ranks=[u(ca, rec, i) for i in range(5, 10) if u(ca, rec, i)],
                        ae=u(ca, rec, 14), te=u(ca, rec, 15), level=u(ca, rec, 26)))

json.dump(dict(spells=spells, casttime=casttime, duration=duration, classtok=classtok, specs=specs, entries=entries),
          open(OUT, 'w'))
print(len(spells), len(entries), collections.Counter(e['type'] for e in entries))
