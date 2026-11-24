#!/usr/bin/env python3
"""Generate CoA class skill-affix candidates (COA_SKILL_AFFIX_PLAN.md section 6.1).

Read-only: server DBC files + SELECTs on acore_world. Writes out/candidates/<class>_<TOKEN>.json and
out/candidates/candidates.csv. See README.md for the field legend.
"""
import argparse, collections, csv, json, os, re, struct, subprocess, sys

COA_DBC_APP = '/opt/coa/azerothcore/apps/coa-dbc'
SRC = '/opt/coa/azerothcore/src/server'
sys.path.insert(0, COA_DBC_APP)
import schemas  # noqa: E402
from client_dbc import Table  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
CLASS_TAB = 87
ATTR0_PASSIVE = 0x40
ATTR3_IGNORE_CASTER_MODS = 0x20000000
MAX_TRIGGER_DEPTH = 3
# Auras whose ticks are modified by SPELLMOD_DOT (periodic damage/heal/leech).
PERIODIC_AURAS = {3, 8, 20, 53, 62, 64, 89}
# Summons that do not count as player-cast damage/heal sources.
NOT_SUMMON = {'SUMMON_PLAYER', 'SUMMON_RAF_FRIEND', 'SUMMON_CHANGE_ITEM'}
LEGEND = ('Omitted numeric = 0, omitted bool = false. Times in ms, radius/range in yards. '
          'eff: i=effect index, e=SPELL_EFFECT_*, a=SPELL_AURA_*, v=basepoints+1 (WotLK display), '
          'per=aura period ms, trig=trigger spell, misc=MiscValue, chain=chain targets. '
          'flags=SpellFamilyFlags[0..2]; mask_unique_bit=[f0,f1,f2] with one bit shared by all ranks and no '
          'other-named spell in the family, else null. cd=RecoveryTime, catcd=CategoryRecoveryTime, '
          'gcd=StartRecoveryTime. children=trigger spells (depth<=3), "via"=parent id. '
          'bonus=spell_bonus_data [direct,dot,ap,ap_dot]. src=sql: spell_dbc overrides Spell.dbc. '
          'known_by_default: free entry (AE/TE cost 0) outside a choice group.')


# --------------------------------------------------------------------------- enums from core headers
def parse_enum(path, prefix):
    names = {}
    pat = re.compile(r'^\s*' + prefix + r'([A-Z0-9_]+)\s*=\s*(\d+)')
    try:
        for line in open(path, encoding='utf-8', errors='replace'):
            m = pat.match(line)
            if m:
                names.setdefault(int(m.group(2)), m.group(1))
    except OSError:
        pass
    return names


EFFECTS = parse_enum(SRC + '/shared/SharedDefines.h', 'SPELL_EFFECT_')
AURAS = parse_enum(SRC + '/game/Spells/Auras/SpellAuraDefines.h', 'SPELL_AURA_')


# --------------------------------------------------------------------------- DBC
def load(dbc_dir, name):
    return Table.read(os.path.join(dbc_dir, name + '.dbc'))


def u32(rec, i):
    return struct.unpack_from('<I', rec, i * 4)[0]


def i32(rec, i):
    return struct.unpack_from('<i', rec, i * 4)[0]


def f32(rec, off):
    return struct.unpack_from('<f', rec, off)[0]


def tstr(t, rec, i):
    return t.string(u32(rec, i)).decode('utf-8', 'replace')


def read_spell_dbc(dbc_dir):
    cols = {c.name: c for c in schemas.registry()['Spell'].columns}
    t = load(dbc_dir, 'Spell')
    out = {}
    for r in range(t.rows):
        rec = t.record(r)

        def g(n):
            c = cols[n]
            if c.kind == 's':
                return t.string(struct.unpack_from('<I', rec, c.offset)[0]).decode('utf-8', 'replace')
            if c.kind == 'f':
                return struct.unpack_from('<f', rec, c.offset)[0]
            return struct.unpack_from('<i' if c.kind == 'i' else '<I', rec, c.offset)[0]

        def g3(n):
            return [g('%s[%d]' % (n, i)) for i in range(3)]
        sid = g('Id')
        out[sid] = dict(
            id=sid, name=g('SpellName[enUS]'), rank=g('Rank[enUS]'), desc=g('Description[enUS]'),
            fam=g('SpellFamilyName'), flags=g3('SpellFamilyFlags'),
            attr=g('Attributes'), attr3=g('AttributesEx3'),
            cast_idx=g('CastingTimeIndex'), cd=g('RecoveryTime'), catcd=g('CategoryRecoveryTime'),
            gcd=g('StartRecoveryTime'), dur_idx=g('DurationIndex'), range_idx=g('RangeIndex'),
            power=g('PowerType'), cost=g('ManaCost'), cost_pct=g('ManaCostPercentage'),
            proc=g('ProcChance'), dmgclass=g('DmgClass'), school=g('SchoolMask'),
            eff=g3('Effect'), aura=g3('EffectApplyAuraName'), bp=g3('EffectBasePoints'),
            die=g3('EffectDieSides'), radius=g3('EffectRadiusIndex'), per=g3('EffectAmplitude'),
            misc=g3('EffectMiscValue'), trig=g3('EffectTriggerSpell'), chain=g3('EffectChainTarget'),
            src='dbc')
    return out


def index_table(t, value):
    return {u32(t.record(r), 0): value(t.record(r)) for r in range(t.rows)}


# --------------------------------------------------------------------------- SQL (read-only)
def mysql_rows(defaults, db, query):
    p = subprocess.run(['mysql', '--defaults-extra-file=' + defaults, '--batch', '--skip-column-names', db,
                        '-e', query], capture_output=True, text=True, check=False)
    if p.returncode:
        sys.exit('mysql failed: ' + p.stderr.strip()[:300])
    unesc = {'n': '\n', 't': '\t', '\\': '\\', '0': '\0'}
    rows = []
    for line in p.stdout.split('\n'):
        if not line:
            continue
        rows.append([None if f == 'NULL' else re.sub(r'\\(.)', lambda m: unesc.get(m.group(1), m.group(1)), f)
                     for f in line.split('\t')])
    return rows


def b64(v):
    import base64
    return base64.b64decode(v.replace('\\n', '')).decode('utf-8', 'replace') if v else ''


SQL_SPELL_COLS = (
    ['ID', 'Name_Lang_enUS', 'NameSubtext_Lang_enUS', 'Description_Lang_enUS', 'SpellClassSet',
     'SpellClassMask_1', 'SpellClassMask_2', 'SpellClassMask_3', 'Attributes', 'AttributesEx3',
     'CastingTimeIndex', 'RecoveryTime', 'CategoryRecoveryTime', 'StartRecoveryTime', 'DurationIndex',
     'RangeIndex', 'PowerType', 'ManaCost', 'ManaCostPct', 'ProcChance', 'DefenseType', 'SchoolMask']
    + ['%s_%d' % (n, i) for n in ('Effect', 'EffectAura', 'EffectBasePoints', 'EffectDieSides',
                                   'EffectRadiusIndex', 'EffectAuraPeriod', 'EffectMiscValue',
                                   'EffectTriggerSpell', 'EffectChainTargets') for i in (1, 2, 3)])


def read_spell_sql(defaults):
    out = {}
    for row in mysql_rows(defaults, 'acore_world', 'SELECT %s FROM spell_dbc' % ','.join(
            'TO_BASE64(%s)' % c if c.endswith('enUS') else c for c in SQL_SPELL_COLS)):
        d = dict(zip(SQL_SPELL_COLS, row))

        def n(k):
            v = d[k]
            return int(float(v)) if v not in (None, '') else 0

        def n3(k):
            return [n('%s_%d' % (k, i)) for i in (1, 2, 3)]
        sid = n('ID')
        out[sid] = dict(
            id=sid, name=b64(d['Name_Lang_enUS']), rank=b64(d['NameSubtext_Lang_enUS']),
            desc=b64(d['Description_Lang_enUS']), fam=n('SpellClassSet'),
            flags=[n('SpellClassMask_%d' % i) & 0xFFFFFFFF for i in (1, 2, 3)],
            attr=n('Attributes') & 0xFFFFFFFF, attr3=n('AttributesEx3') & 0xFFFFFFFF,
            cast_idx=n('CastingTimeIndex'), cd=n('RecoveryTime'), catcd=n('CategoryRecoveryTime'),
            gcd=n('StartRecoveryTime'), dur_idx=n('DurationIndex'), range_idx=n('RangeIndex'),
            power=n('PowerType'), cost=n('ManaCost'), cost_pct=n('ManaCostPct'), proc=n('ProcChance'),
            dmgclass=n('DefenseType'), school=n('SchoolMask'),
            eff=n3('Effect'), aura=n3('EffectAura'), bp=n3('EffectBasePoints'), die=n3('EffectDieSides'),
            radius=n3('EffectRadiusIndex'), per=n3('EffectAuraPeriod'), misc=n3('EffectMiscValue'),
            trig=n3('EffectTriggerSpell'), chain=n3('EffectChainTargets'), src='sql')
    return out


# --------------------------------------------------------------------------- helpers
def bits(flags):
    return [(w, b) for w in range(3) for b in range(32) if flags[w] >> b & 1]


class Ctx:
    def __init__(self, a):
        self.dbc = a.dbc
        spells = read_spell_dbc(a.dbc)
        self.sql_override = set()
        for sid, sp in read_spell_sql(a.defaults).items():
            if sid in spells:
                self.sql_override.add(sid)
            spells[sid] = sp
        self.S = spells
        for sid, idx, val in mysql_rows(a.defaults, 'acore_world',
                                        'SELECT spell_id, effect_index, value FROM coa_spell_fix'):
            sp = spells.get(int(sid))
            if sp and int(idx) < 3:
                sp['bp'][int(idx)] = int(val) - 1  # value is the DBC display number (bp+1)
                sp['fixed'] = True
        self.bonus = {int(r[0]): [round(float(x), 4) for x in r[1:5]] for r in mysql_rows(
            a.defaults, 'acore_world',
            'SELECT entry, direct_bonus, dot_bonus, ap_bonus, ap_dot_bonus FROM spell_bonus_data')}
        self.client_desc = {int(r[0]): b64(r[1]) for r in mysql_rows(
            a.defaults, 'acore_world', 'SELECT ID, TO_BASE64(Description) FROM coa_client_spell_description')}
        ct = load(a.dbc, 'SpellCastTimes'); self.cast = index_table(ct, lambda r: i32(r, 1))
        du = load(a.dbc, 'SpellDuration'); self.dur = index_table(du, lambda r: i32(r, 1))
        ra = load(a.dbc, 'SpellRadius'); self.rad = index_table(ra, lambda r: round(f32(r, 12), 2))
        rg = load(a.dbc, 'SpellRange'); self.rng = index_table(rg, lambda r: round(f32(r, 12), 2))
        # family -> list of (name, flags) for uniqueness checks
        self.fam_index = collections.defaultdict(list)
        for sp in spells.values():
            if sp['fam'] and any(sp['flags']):
                self.fam_index[sp['fam']].append(sp)

    # --- description with simple $-token substitution
    def describe(self, sp, limit=None):
        text = self.client_desc.get(sp['id']) or sp['desc'] or ''

        def val(spell, tok, n):
            if not spell:
                return None
            i = (int(n) - 1) if n else 0
            if not 0 <= i < 3:
                return None
            if tok in 'sm':
                return str(abs(spell['bp'][i] + 1))
            if tok == 't':
                return str(spell['per'][i] / 1000).rstrip('0').rstrip('.')
            if tok == 'a':
                return str(self.rad.get(spell['radius'][i], 0)).rstrip('0').rstrip('.')
            if tok == 'd':
                d = self.dur.get(spell['dur_idx'], 0)
                return ('%g sec' % (d / 1000)) if d > 0 else None
            if tok == 'o':
                d, p = self.dur.get(spell['dur_idx'], 0), spell['per'][i]
                return str(abs(spell['bp'][i] + 1) * (d // p)) if d > 0 and p > 0 else None
            if tok == 'h':
                return str(spell['proc'])
            if tok == 'x':
                return str(spell['chain'][i])
            return None

        def sub(m):
            ref, tok, n = m.group(1), m.group(2).lower(), m.group(3)
            spell = self.S.get(int(ref)) if ref else sp
            v = val(spell, tok, n)
            return v if v is not None else m.group(0)
        def scaled(m):
            op, k, ref, tok, n = m.group(1), float(m.group(2)), m.group(3), m.group(4).lower(), m.group(5)
            v = val(self.S.get(int(ref)) if ref else sp, tok, n)
            if v is None or not v.lstrip('-').replace('.', '').isdigit():
                return m.group(0)
            return '%g' % (float(v) / k if op == '/' else float(v) * k)
        text = re.sub(r'\$([/*])([\d.]+);(\d*)([smto])([1-3]?)', scaled, text)
        text = re.sub(r'\$(\d*)([smtadohx])([1-3]?)', sub, text)
        text = re.sub(r'\s+', ' ', text).strip()
        if limit and len(text) > limit:
            text = text[:limit - 1].rstrip() + '…'
        return text

    def effects(self, sp):
        out = []
        for i in range(3):
            e = sp['eff'][i]
            if not e:
                continue
            d = {'i': i, 'e': EFFECTS.get(e, e)}
            if sp['aura'][i]:
                d['a'] = AURAS.get(sp['aura'][i], sp['aura'][i])
            v = sp['bp'][i] + 1
            if v or sp['die'][i] > 1:
                d['v'] = v
            for k in ('per', 'trig', 'misc', 'chain'):
                if sp[k][i]:
                    d[k] = sp[k][i]
            if sp['radius'][i]:
                d['r'] = self.rad.get(sp['radius'][i], 0)
            out.append(d)
        return out

    def is_summon_self(self, sp):
        for i in range(3):
            en = EFFECTS.get(sp['eff'][i], '')
            an = AURAS.get(sp['aura'][i], '') if sp['eff'][i] == 6 else ''
            if ('SUMMON' in en and en not in NOT_SUMMON) or 'SUMMON' in an:
                return True
        return False

    def periodic(self, sp):
        return any(sp['eff'][i] in (6, 27, 35, 65, 119, 128, 129, 143) and sp['aura'][i] in PERIODIC_AURAS
                   for i in range(3))

    def summary(self, sp):
        """Compact per-spell stats (shared by candidates and trigger children)."""
        d = {}
        def put(k, v):
            if v:
                d[k] = v
        put('cd', sp['cd']); put('catcd', sp['catcd']); put('gcd', sp['gcd'])
        if sp['cost'] or sp['cost_pct']:
            d['power'] = sp['power']
        put('cost', sp['cost']); put('cost_pct', sp['cost_pct'])
        put('cast', max(0, self.cast.get(sp['cast_idx'], 0)))
        put('dur', self.dur.get(sp['dur_idx'], 0))
        rads = sorted({self.rad.get(r, 0) for r in sp['radius'] if r} - {0})
        put('radius', rads)
        put('range', self.rng.get(sp['range_idx'], 0))
        put('dmgclass', sp['dmgclass']); put('school', sp['school'])
        d['fam'] = sp['fam']
        d['flags'] = sp['flags']
        d['eff'] = self.effects(sp)
        put('passive', bool(sp['attr'] & ATTR0_PASSIVE))
        put('periodic', self.periodic(sp))
        put('ignore_caster_mods', bool(sp['attr3'] & ATTR3_IGNORE_CASTER_MODS))
        put('bonus', self.bonus.get(sp['id']))
        if sp['src'] == 'sql':
            d['src'] = 'sql'
        put('fixed', sp.get('fixed'))
        return d

    def unique_bit(self, ids):
        """One family-flag bit set on every spell in ids and on no other-named spell of that family."""
        sps = [self.S[i] for i in ids if i in self.S]
        if not sps or len({s['fam'] for s in sps}) != 1 or not sps[0]['fam']:
            return None
        common = [sps[0]['flags'][w] for w in range(3)]
        for s in sps[1:]:
            common = [common[w] & s['flags'][w] for w in range(3)]
        if not any(common):
            return None
        names = {s['name'] for s in sps}
        idset = set(ids)
        used = [0, 0, 0]
        for o in self.fam_index[sps[0]['fam']]:
            if o['id'] in idset or o['name'] in names:
                continue
            used = [used[w] | o['flags'][w] for w in range(3)]
        for w, b in bits(common):
            if not used[w] >> b & 1:
                m = [0, 0, 0]
                m[w] = 1 << b
                return m
        return None

    def children(self, roots):
        """Trigger spells reachable from roots (max depth 3), each once."""
        out, seen = [], set(roots)
        frontier = [(r, 0) for r in roots]
        while frontier:
            sid, depth = frontier.pop(0)
            if depth >= MAX_TRIGGER_DEPTH:
                continue
            for t in self.S[sid]['trig']:
                if t and t in self.S and t not in seen:
                    seen.add(t)
                    out.append((t, sid, depth + 1))
                    frontier.append((t, depth + 1))
        return out


# --------------------------------------------------------------------------- CharacterAdvancement
def read_entries(dbc):
    classes = load(dbc, 'ChrClasses')
    classtok = {u32(classes.record(r), 0): tstr(classes, classes.record(r), 55) for r in range(classes.rows)}
    cty = load(dbc, 'CharacterAdvancementClassTypes')
    ctype = index_table(cty, lambda r: (u32(r, 2), u32(r, 4)))
    tt = load(dbc, 'CharacterAdvancementTabTypes')
    tabtok = index_table(tt, lambda r: tstr(tt, r, 1).upper())
    cs = load(dbc, 'ChrSpecs')
    spec_by, specs = {}, {}
    for r in range(cs.rows):
        rec = cs.record(r)
        sid = u32(rec, 0)
        spec_by[(tstr(cs, rec, 1), tstr(cs, rec, 2))] = sid
        specs[sid] = dict(id=sid, tab=tstr(cs, rec, 2), sig=u32(rec, 24))
    ca = load(dbc, 'CharacterAdvancement')
    entries = []
    for r in range(ca.rows):
        rec = ca.record(r)
        t = ctype.get(u32(rec, 32))
        if not t or not t[1] or not 12 <= t[0] <= 32:
            continue
        cls, tab = t[0], u32(rec, 33)
        spec = 0
        if tab != CLASS_TAB:
            spec = spec_by.get((classtok.get(cls), tabtok.get(tab)))
            if spec is None:
                continue
        entries.append(dict(entry=u32(rec, 0), cls=cls, spec=spec, type=tstr(ca, rec, 1),
                            ranks=[u32(rec, i) for i in range(5, 10) if u32(rec, i)],
                            ae=u32(rec, 14), te=u32(rec, 15), level=u32(rec, 26), group=u32(rec, 29)))
    return entries, classtok, specs


# --------------------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--dbc', default='/opt/coa/server-data/dbc')
    ap.add_argument('--defaults', default='/opt/coa/secrets/my.cnf', help='mysql --defaults-extra-file')
    ap.add_argument('--out', default=os.path.join(HERE, 'out', 'candidates'))
    a = ap.parse_args()
    ctx = Ctx(a)
    entries, classtok, specs = read_entries(a.dbc)
    sig_spells = {s['sig'] for s in specs.values() if s['sig']}
    os.makedirs(a.out, exist_ok=True)
    by_class = collections.defaultdict(list)
    for e in entries:
        by_class[e['cls']].append(e)

    csv_rows, stats = [], {}
    for cls in sorted(by_class):
        tok = classtok.get(cls, str(cls)).upper()
        cands, passive, missing = [], [], 0
        for e in sorted(by_class[cls], key=lambda x: (x['spec'], x['type'], x['entry'])):
            ranks = [r for r in e['ranks'] if r in ctx.S]
            if not ranks:
                missing += 1
                continue
            sp = ctx.S[ranks[0]]
            if sp['attr'] & ATTR0_PASSIVE:
                passive.append({'entry': e['entry'], 'spec': e['spec'], 'type': e['type'][0], 'id': sp['id'],
                                'name': sp['name'], 'desc': ctx.describe(sp, 200)})
                continue
            c = {'entry': e['entry'], 'spec': e['spec'], 'type': e['type'], 'id': sp['id'],
                 'name': sp['name'], 'ranks': ranks, 'desc': ctx.describe(sp)}
            if not e['ae'] and not e['te'] and not e['group']:
                c['known_by_default'] = True
            if any(r in sig_spells for r in ranks):
                c['signature'] = True
            c.update(ctx.summary(sp))
            if len(ranks) > 1:
                c['rank_values'] = {str(r): [x + 1 for x in ctx.S[r]['bp']] for r in ranks[1:]}
                diff = {k for r in ranks[1:] for k in ('cd', 'cost', 'cast_idx', 'dur_idx', 'flags')
                        if ctx.S[r][k] != sp[k]}
                if diff:
                    c['rank_diff'] = sorted(diff)
            c['mask_unique_bit'] = ctx.unique_bit(ranks)
            kids = []
            for kid, via, depth in ctx.children(ranks):
                ks = ctx.S[kid]
                k = {'id': kid, 'name': ks['name'], 'via': via, 'depth': depth}
                k.update(ctx.summary(ks))
                k['mask_unique_bit'] = ctx.unique_bit([kid])
                if ctx.is_summon_self(ks):
                    k['is_summon'] = True
                kids.append(k)
            if kids:
                c['children'] = kids
            if ctx.is_summon_self(sp) or any(k.get('is_summon') for k in kids):
                c['is_summon'] = 'self' if ctx.is_summon_self(sp) else 'child'
            if 'is_summon' not in c:
                c['is_summon'] = False
            cands.append(c)
            csv_rows.append([cls, tok, e['spec'], e['entry'], e['type'], sp['id'], sp['name'],
                             ' '.join(map(str, ranks)), c.get('cd', 0), c.get('catcd', 0), c.get('gcd', 0),
                             c.get('power', ''), c.get('cost', 0), c.get('cost_pct', 0), c.get('cast', 0),
                             c.get('dur', 0), ' '.join(map(str, c.get('radius', []))), c.get('range', 0),
                             c.get('dmgclass', 0), sp['fam'], ' '.join('0x%x' % f for f in sp['flags']),
                             json.dumps(c['mask_unique_bit']) if c['mask_unique_bit'] else '',
                             c['is_summon'] or '', int(bool(c.get('ignore_caster_mods'))),
                             int(bool(c.get('periodic'))), len(kids), ' '.join(str(k['id']) for k in kids),
                             int(bool(c.get('known_by_default')))])
        doc = {'class_id': cls, 'class': tok, '_legend': LEGEND,
               'specs': {str(s): specs[s]['tab'] for s in sorted({x['spec'] for x in cands + passive}) if s},
               'candidates': cands, 'passive_context': passive}
        path = os.path.join(a.out, '%d_%s.json' % (cls, tok))
        with open(path, 'w', encoding='utf-8') as f:
            json.dump(doc, f, ensure_ascii=False, separators=(',', ':'))
        per_spec = collections.Counter(c['spec'] for c in cands)
        stats[cls] = dict(tok=tok, active=len(cands), ability=sum(c['type'] == 'Ability' for c in cands),
                          talent=sum(c['type'] == 'Talent' for c in cands),
                          zero_flags=sum(not any(c['flags']) for c in cands),
                          mask_unique=sum(bool(c['mask_unique_bit']) for c in cands),
                          summons=sum(bool(c['is_summon']) for c in cands),
                          icm=sum(bool(c.get('ignore_caster_mods')) for c in cands),
                          passive=len(passive), missing=missing, kb=os.path.getsize(path) // 1024,
                          per_spec=' '.join('%d:%d' % kv for kv in sorted(per_spec.items())))
    with open(os.path.join(a.out, 'candidates.csv'), 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f)
        w.writerow(['class_id', 'class', 'spec', 'entry', 'type', 'spell', 'name', 'ranks', 'cd', 'catcd', 'gcd',
                    'power', 'cost', 'cost_pct', 'cast', 'dur', 'radius', 'range', 'dmgclass', 'fam', 'flags',
                    'mask_unique_bit', 'is_summon', 'ignore_caster_mods', 'periodic', 'n_children', 'children',
                    'known_by_default'])
        w.writerows(csv_rows)
    hdr = '%-3s %-14s %6s %5s %5s %5s %6s %6s %4s %7s %4s %4s  %s' % (
        'id', 'class', 'active', 'abil', 'tal', '0flg', 'unique', 'summon', 'icm', 'passive', 'miss', 'KB',
        'per spec')
    print(hdr)
    tot = collections.Counter()
    for cls, s in stats.items():
        print('%-3d %-14s %6d %5d %5d %5d %6d %6d %4d %7d %4d %4d  %s' % (
            cls, s['tok'], s['active'], s['ability'], s['talent'], s['zero_flags'], s['mask_unique'],
            s['summons'], s['icm'], s['passive'], s['missing'], s['kb'], s['per_spec']))
        tot.update({k: v for k, v in s.items() if isinstance(v, int)})
    print('TOTAL active=%(active)d abil=%(ability)d tal=%(talent)d 0flg=%(zero_flags)d unique=%(mask_unique)d '
          'summon=%(summons)d icm=%(icm)d passive=%(passive)d missing=%(missing)d KB=%(kb)d' % tot)
    print('spell_dbc overrides of Spell.dbc: %d; candidates from spell_dbc: %d' % (
        len(ctx.sql_override), sum(1 for r in csv_rows if ctx.S[r[5]]['src'] == 'sql')))


if __name__ == '__main__':
    main()
