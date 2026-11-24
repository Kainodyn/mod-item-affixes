#!/usr/bin/env python3
"""Validate reviewer files out/reviews/<class_id>_<TOKEN>.json (COA_SKILL_AFFIX_PLAN.md section 6.2).

Checks schema and rules against the generator output in out/candidates/. Prints a readable report and exits 1
when any file has errors. Usage: validate_reviews.py [FILE ...]  (default: every out/reviews/*.json)
"""
import argparse, glob, json, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
OPS = {'DAMAGE', 'DOT', 'EFFECT1', 'EFFECT2', 'EFFECT3', 'ALL_EFFECTS', 'CRITICAL_CHANCE', 'CASTING_TIME',
       'DURATION', 'RADIUS', 'JUMP_TARGETS', 'COOLDOWN', 'COST', 'RANGE'}
TYPES = {'FLAT', 'PCT'}
# Legacy affix_template.targeting values (still written; the module ignores targeting and mask since
# 2026-11-24: every affix is a targetSpellId spellmod, cooldown via SPELLMOD_COOLDOWN).
TARGETING = {'mask', 'spell_id', 'hook_cd'}
PCT_ONLY = {'DAMAGE', 'DOT', 'EFFECT1', 'EFFECT2', 'EFFECT3', 'ALL_EFFECTS', 'COST'}
MUST_BE_NEGATIVE = {'COOLDOWN', 'CASTING_TIME', 'COST'}
MAX_EFFECTS = 3          # affix_template effects op1..op3 (plan section 5)
MAX_PER_TARGET = 8
CONFIDENCE = {'high', 'medium', 'low'}
AFFIX_FIELDS = {  # name: (types, required)
    'key': (str, True), 'name': (str, True), 'spec_id': (int, True), 'target_spell': (int, True),
    'affected_spells': (list, True), 'targeting': (str, True), 'mask': ((list, type(None)), False),
    'effects': (list, True), 'scales_with_crit': (bool, True), 'min_quality': (int, True),
    'weight': (int, True), 'evidence': (str, True), 'client_sensitive': (bool, True),
    'confidence': (str, True), 'notes': (str, False)}


def is_int(v):
    return isinstance(v, int) and not isinstance(v, bool)


def type_ok(v, t):
    if t is int:
        return is_int(v)
    return isinstance(v, t)


def load_candidates(cand_dir, class_id):
    files = glob.glob(os.path.join(cand_dir, '%d_*.json' % class_id))
    if not files:
        return None
    doc = json.load(open(files[0], encoding='utf-8'))
    spells = {}
    for c in doc['candidates']:
        for r in c['ranks']:
            spells[r] = dict(c, _kind='candidate', _spec=c['spec'])
        for k in c.get('children', []):
            spells.setdefault(k['id'], dict(k, _kind='child', _spec=c['spec'], _parent=c['id'],
                                            is_summon=k.get('is_summon', False)))
    return doc, spells


def stat(spells, key):
    return max((s.get(key, 0) for s in spells if isinstance(s.get(key, 0), (int, float))), default=0)


def check_file(path, cand_dir, seen_keys):
    errs = []
    try:
        doc = json.load(open(path, encoding='utf-8'))
    except (OSError, ValueError) as e:
        return ['cannot read JSON: %s' % e]
    if not isinstance(doc, dict):
        return ['top level must be an object']
    for k, t in (('class_id', int), ('class', str), ('reviewed_at', str), ('affixes', list), ('rejected', list)):
        if not type_ok(doc.get(k), t):
            errs.append('top-level "%s" missing or not %s' % (k, t.__name__))
    if errs:
        return errs
    if not re.fullmatch(r'\d{4}-\d{2}-\d{2}', doc['reviewed_at']):
        errs.append('reviewed_at must be YYYY-MM-DD')
    loaded = load_candidates(cand_dir, doc['class_id'])
    if not loaded:
        return errs + ['no candidate file for class_id %d in %s' % (doc['class_id'], cand_dir)]
    cdoc, spells = loaded
    if doc['class'] != cdoc['class']:
        errs.append('class "%s" does not match candidates "%s"' % (doc['class'], cdoc['class']))
    base = os.path.basename(path)
    if not base.startswith('_') and base != '%d_%s.json' % (cdoc['class_id'], cdoc['class']):
        errs.append('file name should be %d_%s.json' % (cdoc['class_id'], cdoc['class']))

    per_target = {}
    for n, a in enumerate(doc['affixes']):
        tag = 'affix[%d]' % n
        if not isinstance(a, dict):
            errs.append('%s: not an object' % tag)
            continue
        tag = 'affix[%d] "%s"' % (n, a.get('key', '?'))
        e = []
        for f, (t, req) in AFFIX_FIELDS.items():
            if f not in a:
                if req:
                    e.append('missing field "%s"' % f)
            elif not type_ok(a[f], t):
                e.append('field "%s" has wrong type' % f)
        for f in a:
            if f not in AFFIX_FIELDS:
                e.append('unknown field "%s"' % f)
        if e:
            errs += ['%s: %s' % (tag, x) for x in e]
            continue
        key = a['key']
        if not re.fullmatch(r'[a-z0-9_]+', key):
            e.append('key must be lower_snake_case')
        if key in seen_keys:
            e.append('duplicate key (also in %s)' % seen_keys[key])
        seen_keys.setdefault(key, base)
        if a['min_quality'] < 3:
            e.append('min_quality %d < 3 (blue+ only)' % a['min_quality'])
        if a['weight'] <= 0:
            e.append('weight must be > 0')
        if a['confidence'] not in CONFIDENCE:
            e.append('confidence must be one of %s' % sorted(CONFIDENCE))
        if not a['evidence'].strip():
            e.append('evidence is empty')

        # spells
        tgt = a['target_spell']
        ids = a['affected_spells']
        if not ids or not all(is_int(x) for x in ids):
            e.append('affected_spells must be a non-empty list of spell ids')
            ids = []
        if len(set(ids)) != len(ids):
            e.append('affected_spells has duplicates')
        unknown = [x for x in [tgt] + ids if x not in spells]
        if unknown:
            e.append('spell(s) %s not among class candidates/trigger children' % sorted(set(unknown)))
        known = [spells[x] for x in dict.fromkeys([tgt] + ids) if x in spells]
        for s in known:
            if s.get('is_summon'):
                e.append('spell %d %s is a summon (is_summon=%s)' % (s['id'], s['name'], s['is_summon']))
            if s.get('ignore_caster_mods'):
                e.append('spell %d %s has SPELL_ATTR3_IGNORE_CASTER_MODIFIERS' % (s['id'], s['name']))
        if tgt in spells and a['spec_id'] != spells[tgt]['_spec']:
            e.append('spec_id %d != spec %d of target_spell' % (a['spec_id'], spells[tgt]['_spec']))
        if tgt in spells:
            per_target[tgt] = per_target.get(tgt, 0) + 1
        affected = [spells[x] for x in ids if x in spells] or ([spells[tgt]] if tgt in spells else [])
        tspell = spells.get(tgt, {})

        # targeting (legacy column, any known value; mask is not checked)
        mode = a['targeting']
        mask = a.get('mask')
        if mode not in TARGETING:
            e.append('targeting "%s" not in %s' % (mode, sorted(TARGETING)))
        if mask is not None and (not isinstance(mask, list) or len(mask) != 3 or not all(is_int(x) for x in mask)):
            e.append('mask must be null or three integers')

        if not isinstance(a.get('name'), str) or '|' in a['name'] or len(a['name']) > 56:
            e.append('name must be a string without "|" and <= 56 chars (addon payload)')
        # effects
        effs = a['effects']
        if not 1 <= len(effs) <= MAX_EFFECTS:
            e.append('effects must have 1..%d entries' % MAX_EFFECTS)
        for m, fx in enumerate(effs):
            ft = 'effect[%d]' % m
            if not isinstance(fx, dict) or set(fx) != {'op', 'type', 'value'}:
                e.append('%s must be {"op","type","value"}' % ft)
                continue
            op, ty, v = fx['op'], fx['type'], fx['value']
            if op not in OPS:
                e.append('%s: op "%s" not allowed' % (ft, op)); continue
            if ty not in TYPES:
                e.append('%s: type "%s" not in FLAT/PCT' % (ft, ty)); continue
            if not is_int(v) or v == 0:
                e.append('%s: value must be a non-zero integer' % ft); continue
            ft = '%s %s %s %d' % (ft, op, ty, v)
            if op in PCT_ONLY and ty != 'PCT':
                e.append('%s: %s only as PCT' % (ft, op))
            if op in MUST_BE_NEGATIVE and v > 0:
                e.append('%s: %s must be negative (a bonus)' % (ft, op))
            if op == 'COOLDOWN':
                base_cd = max(tspell.get('cd', 0), tspell.get('catcd', 0))
                if base_cd <= 0:
                    e.append('%s: target_spell has no cooldown' % ft)
                elif ty == 'FLAT' and abs(v) > 0.15 * base_cd:
                    e.append('%s: |value| > 15%% of base cooldown %d ms (max %d)' % (ft, base_cd, int(0.15 * base_cd)))
                elif ty == 'PCT' and abs(v) > 15:
                    e.append('%s: |PCT| > 15' % ft)
            elif op == 'CASTING_TIME':
                if stat(affected + [tspell], 'cast') <= 0:
                    e.append('%s: no affected spell has a cast time' % ft)
                elif (ty == 'FLAT' and abs(v) > max(500, 0.17 * stat(affected + [tspell], 'cast'))) or (ty == 'PCT' and abs(v) > 10):
                    e.append('%s: limit is max(500 ms, 17%% of cast) FLAT / 10 PCT' % ft)
            elif op == 'DURATION':
                if stat(affected + [tspell], 'dur') <= 0:
                    e.append('%s: no affected spell has a finite duration > 0' % ft)
                elif v < 0:
                    e.append('%s: DURATION must be positive (a bonus)' % ft)
                elif ty == 'FLAT' and v > 0.25 * stat(affected + [tspell], 'dur'):
                    e.append('%s: FLAT > 25%% of base duration %d ms' % (ft, stat(affected + [tspell], 'dur')))
                elif ty == 'PCT' and v > 25:
                    e.append('%s: PCT > 25 (stack cap per spell is 100%%, enforced at apply time)' % ft)
            elif op in ('DAMAGE', 'DOT', 'EFFECT1', 'EFFECT2', 'EFFECT3', 'ALL_EFFECTS'):
                if abs(v) > 10:
                    e.append('%s: |PCT| > 10' % ft)
                if op == 'DOT' and not any(s.get('periodic') for s in affected + [tspell]):
                    e.append('%s: no affected spell has a periodic damage/heal aura' % ft)
                if op.startswith('EFFECT') and op != 'ALL_EFFECTS':
                    idx = int(op[-1]) - 1
                    if not any(x.get('i') == idx for s in affected for x in s.get('eff', [])):
                        e.append('%s: no affected spell has effect index %d' % (ft, idx))
            elif op == 'CRITICAL_CHANCE':
                if abs(v) > 5:
                    e.append('%s: |value| > 5' % ft)
            elif op == 'COST':
                if not (tspell.get('cost', 0) > 0 or tspell.get('cost_pct', 0) > 0):
                    e.append('%s: target_spell has no cost' % ft)
                elif abs(v) > 15:
                    e.append('%s: |PCT| > 15' % ft)
            elif op == 'RADIUS':
                if not any(s.get('radius') for s in affected + [tspell]):
                    e.append('%s: no affected spell has a radius' % ft)
                elif v < 0 or (ty == 'PCT' and v > 15) or (ty == 'FLAT' and v > 2):
                    e.append('%s: RADIUS must be positive, max +15 PCT / +2 yd FLAT' % ft)
            elif op == 'JUMP_TARGETS':
                if not any(x.get('chain', 0) > 1 for s in affected + [tspell] for x in s.get('eff', [])):
                    e.append('%s: no affected spell has chain targets' % ft)
                elif ty != 'FLAT' or not 0 < v <= 1:
                    e.append('%s: JUMP_TARGETS only FLAT +1' % ft)
            elif op == 'RANGE':
                if stat([tspell], 'range') <= 0:
                    e.append('%s: target_spell has no range' % ft)
                elif v < 0 or (ty == 'PCT' and v > 15) or (ty == 'FLAT' and v > 5):
                    e.append('%s: RANGE must be positive, max +15 PCT / +5 yd FLAT' % ft)
        errs += ['%s: %s' % (tag, x) for x in e]

    for tgt, n in per_target.items():
        if n > MAX_PER_TARGET:
            errs.append('target_spell %d has %d affixes (max %d)' % (tgt, n, MAX_PER_TARGET))
    for n, r in enumerate(doc['rejected']):
        if not isinstance(r, dict) or not is_int(r.get('spell')) or not isinstance(r.get('reason'), str):
            errs.append('rejected[%d] must be {"spell": int, "reason": str}' % n)
        elif r['spell'] not in spells:
            errs.append('rejected[%d]: spell %d not among class candidates' % (n, r['spell']))
    return errs


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('files', nargs='*')
    ap.add_argument('--candidates', default=os.path.join(HERE, 'out', 'candidates'))
    a = ap.parse_args()
    files = a.files or sorted(glob.glob(os.path.join(HERE, 'out', 'reviews', '*.json')))
    if not files:
        print('no review files found')
        return 1
    seen, bad = {}, 0
    for f in files:
        errs = check_file(f, a.candidates, seen)
        try:
            n = len(json.load(open(f, encoding='utf-8')).get('affixes', []))
        except Exception:  # noqa: BLE001 - report only
            n = '?'
        print('%s %s (%s affixes, %d errors)' % ('FAIL' if errs else 'OK  ', f, n, len(errs)))
        for x in errs:
            print('    - ' + x)
        bad += bool(errs)
    print('%d/%d file(s) failed' % (bad, len(files)))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
