#!/usr/bin/env python3
"""
mod-item-affixes (classless branch) -- client patcher.

Adds the module's custom spells (Imprints and spell-swap variants) to the
client and installs the ItemAffixes addon. Run it AFTER the
mod-classless-wildcard client installer, and again every time you re-run that
installer.

Why it works this way: the Wildcard installer writes the highest-priority
patch archives (Data/patch-Z.MPQ and Data/<locale>/patch-<locale>-Z.MPQ) with
its own Spell.dbc and SkillLineAbility.dbc. A separate archive from this
module could not outrank them, so this patcher adds its rows INTO those two
DBCs, inside Wildcard's archives. Re-running the Wildcard installer rebuilds
them from the client's originals, which drops these rows again.

    python ItemAffixesPatch.py [WOW_FOLDER] [--uninstall] [--dry-run] [--no-addon]

Put the ItemAffixesPatch folder in your WoW folder (beside Wow.exe) and the
folder is found on its own. Nothing but a stock Python 3.7+ is needed.
"""
import argparse
import bz2
import json
import os
import shutil
import struct
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
ROWS_FILE = os.path.join(HERE, "affix_dbc_rows.json")
WILDCARD_MANIFEST = "ClasslessWildcard-install.json"
DBC_PATHS = {"Spell.dbc": "DBFilesClient\\Spell.dbc",
             "SkillLineAbility.dbc": "DBFilesClient\\SkillLineAbility.dbc"}
LOCALES_PER_STRING = 16


class Abort(Exception):
    pass


# --------------------------------------------------------------------- MPQ
# The subset of the MPQ format the Wildcard installer writes: a v1 archive,
# unencrypted files stored or compressed per sector (zlib / bzip2).

MPQ_MAGIC = b"MPQ\x1a"
FLAG_IMPLODE = 0x00000100
FLAG_COMPRESS = 0x00000200
FLAG_ENCRYPTED = 0x00010000
FLAG_SINGLE_UNIT = 0x01000000
FLAG_DELETE_MARKER = 0x02000000
FLAG_SECTOR_CRC = 0x04000000
FLAG_EXISTS = 0x80000000
HASH_EMPTY = 0xFFFFFFFF
HASH_DELETED = 0xFFFFFFFE


def _crypt_table():
    table = [0] * 0x500
    seed = 0x00100001
    for i in range(0x100):
        index = i
        for _ in range(5):
            seed = (seed * 125 + 3) % 0x2AAAAB
            hi = (seed & 0xFFFF) << 16
            seed = (seed * 125 + 3) % 0x2AAAAB
            table[index] = hi | (seed & 0xFFFF)
            index += 0x100
    return table


CRYPT = _crypt_table()


def mpq_hash(name, kind):
    s1, s2 = 0x7FED7FED, 0xEEEEEEEE
    for ch in name.upper().replace("/", "\\").encode("ascii"):
        s1 = (CRYPT[(kind << 8) + ch] ^ ((s1 + s2) & 0xFFFFFFFF)) & 0xFFFFFFFF
        s2 = (ch + s1 + s2 + (s2 << 5) + 3) & 0xFFFFFFFF
    return s1


def _crypt(data, key, decrypt):
    words = list(struct.unpack("<%dI" % (len(data) // 4), data))
    seed = 0xEEEEEEEE
    for i, value in enumerate(words):
        seed = (seed + CRYPT[0x400 + (key & 0xFF)]) & 0xFFFFFFFF
        out = value ^ ((key + seed) & 0xFFFFFFFF)
        plain = out if decrypt else value
        key = ((((~key) << 21) + 0x11111111) & 0xFFFFFFFF) | (key >> 11)
        seed = (plain + seed + (seed << 5) + 3) & 0xFFFFFFFF
        words[i] = out
    return struct.pack("<%dI" % len(words), *words)


def _decompress(chunk, expected):
    if len(chunk) == expected:
        return chunk
    kind, body = chunk[0], chunk[1:]
    if kind == 0x02:
        return zlib.decompress(body)
    if kind == 0x10:
        return bz2.decompress(body)
    raise Abort("unsupported MPQ compression 0x%02x" % kind)


class Archive:
    """Read an MPQ fully into memory: {name: (raw block bytes, flags, size)}."""

    def __init__(self, path):
        self.path = path
        with open(path, "rb") as fh:
            self.raw = fh.read()
        if self.raw[:4] != MPQ_MAGIC:
            raise Abort("%s is not an MPQ archive this patcher can read" % path)
        (_, self.header_size, _, self.version, self.sector_shift, hash_pos,
         block_pos, hash_count, block_count) = struct.unpack_from("<4sIIHHIIII", self.raw)
        self.sector_size = 512 << self.sector_shift
        hashes = _crypt(self.raw[hash_pos:hash_pos + hash_count * 16],
                        mpq_hash("(hash table)", 3), True)
        blocks = _crypt(self.raw[block_pos:block_pos + block_count * 16],
                        mpq_hash("(block table)", 3), True)
        self.hashes = [struct.unpack_from("<IIHHI", hashes, i * 16) for i in range(hash_count)]
        self.blocks = [struct.unpack_from("<4I", blocks, i * 16) for i in range(block_count)]

    def _block_index(self, name):
        count = len(self.hashes)
        start = mpq_hash(name, 0) & (count - 1)
        a, b = mpq_hash(name, 1), mpq_hash(name, 2)
        for probe in range(count):
            ha, hb, _loc, _plat, index = self.hashes[(start + probe) & (count - 1)]
            if index == HASH_EMPTY:
                return None
            if index != HASH_DELETED and ha == a and hb == b:
                return index
        return None

    def has(self, name):
        index = self._block_index(name)
        return index is not None and bool(self.blocks[index][3] & FLAG_EXISTS) \
            and not self.blocks[index][3] & FLAG_DELETE_MARKER

    def packed(self, name):
        pos, csize, fsize, flags = self.blocks[self._block_index(name)]
        return self.raw[pos:pos + csize], flags, fsize

    def read(self, name):
        data, flags, fsize = self.packed(name)
        if flags & (FLAG_ENCRYPTED | FLAG_IMPLODE):
            raise Abort("%s: %s is encrypted or imploded; not a Wildcard archive?"
                        % (os.path.basename(self.path), name))
        if not flags & FLAG_COMPRESS:
            return data[:fsize]
        if flags & FLAG_SINGLE_UNIT:
            return _decompress(data, fsize)
        count = (fsize + self.sector_size - 1) // self.sector_size
        offsets = struct.unpack_from("<%dI" % (count + 1), data)
        out = bytearray()
        for i in range(count):
            want = min(self.sector_size, fsize - i * self.sector_size)
            out += _decompress(data[offsets[i]:offsets[i + 1]], want)
        return bytes(out)

    def names(self):
        raw = self.read("(listfile)").decode("utf-8", "replace")
        return [n.strip() for n in raw.replace("\r", "\n").split("\n")
                if n.strip() and n.strip().lower() != "(listfile)"]


def _pack(payload, sector_size):
    count = max(1, (len(payload) + sector_size - 1) // sector_size)
    offsets, chunks = [4 * (count + 1)], []
    for i in range(count):
        raw = payload[i * sector_size:(i + 1) * sector_size]
        squeezed = b"\x02" + zlib.compress(raw, 9)
        chunk = squeezed if len(squeezed) < len(raw) else raw
        chunks.append(chunk)
        offsets.append(offsets[-1] + len(chunk))
    return struct.pack("<%dI" % len(offsets), *offsets) + b"".join(chunks)


def write_archive(src, replaced, path):
    """Rewrite `src` with the files in `replaced` ({name: bytes}) swapped in.

    Every other file is copied over packed exactly as it was. Same layout the
    Wildcard installer writes: v1 header, zlib sectors, a listfile."""
    names = src.names()
    entries = []
    for name in names + ["(listfile)"]:
        if name == "(listfile)":
            listing = "\r\n".join(names + ["(listfile)"]).encode("utf-8")
            entries.append((name, _pack(listing, src.sector_size), FLAG_EXISTS | FLAG_COMPRESS, len(listing)))
        elif name in replaced:
            body = replaced[name]
            entries.append((name, _pack(body, src.sector_size), FLAG_EXISTS | FLAG_COMPRESS, len(body)))
        else:
            data, flags, size = src.packed(name)
            entries.append((name, data, flags, size))

    hash_count = 16
    while hash_count < len(entries) * 4:
        hash_count <<= 1
    table = [(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF, 0xFFFF, HASH_EMPTY)] * hash_count
    for index, (name, _d, _f, _s) in enumerate(entries):
        start = mpq_hash(name, 0) & (hash_count - 1)
        for probe in range(hash_count):
            slot = (start + probe) & (hash_count - 1)
            if table[slot][4] == HASH_EMPTY:
                table[slot] = (mpq_hash(name, 1), mpq_hash(name, 2), 0, 0, index)
                break

    body, blocks, pos = bytearray(), bytearray(), 32
    for _name, data, flags, size in entries:
        blocks += struct.pack("<4I", pos + len(body), len(data), size, flags)
        body += data
    hashes = b"".join(struct.pack("<IIHHI", *h) for h in table)
    hash_pos = 32 + len(body)
    block_pos = hash_pos + len(hashes)
    header = struct.pack("<4sIIHHIIII", MPQ_MAGIC, 32, block_pos + len(blocks), 0,
                         src.sector_shift, hash_pos, block_pos, hash_count, len(entries))
    tmp = path + ".itemaffixes-tmp"
    with open(tmp, "wb") as fh:
        fh.write(header)
        fh.write(body)
        fh.write(_crypt(hashes, mpq_hash("(hash table)", 3), False))
        fh.write(_crypt(bytes(blocks), mpq_hash("(block table)", 3), False))
    os.replace(tmp, path)


# --------------------------------------------------------------------- DBC

class Dbc:
    def __init__(self, raw):
        magic, count, self.fields, self.size, strsize = struct.unpack_from("<4s4I", raw)
        if magic != b"WDBC" or self.size != self.fields * 4:
            raise Abort("unexpected DBC layout")
        self.records = [list(struct.unpack_from("<%dI" % self.fields, raw, 20 + i * self.size))
                        for i in range(count)]
        self.strings = bytearray(raw[20 + count * self.size:20 + count * self.size + strsize])

    def text(self, offset):
        end = self.strings.index(0, offset)
        return self.strings[offset:end].decode("utf-8", "replace")

    def add_text(self, text):
        offset = len(self.strings)
        self.strings += text.encode("utf-8") + b"\0"
        return offset

    def build(self):
        out = bytearray(struct.pack("<4s4I", b"WDBC", len(self.records), self.fields,
                                    self.size, len(self.strings)))
        for rec in self.records:
            out += struct.pack("<%dI" % self.fields, *rec)
        return bytes(out + self.strings)


def string_groups(string_fields):
    """Base field of every 16-locale string column (136, 153, ...)."""
    fields = set(string_fields)
    return [f for f in sorted(fields) if f - 1 not in fields]


def row_matches(dbc, rec, row, string_fields):
    for i, value in enumerate(row["values"]):
        if i not in string_fields and rec[i] != value:
            return False
    for field, text in row["strings"].items():
        if dbc.text(rec[int(field)]) != text:
            return False
    return True


def apply_rows(raw, table, uninstall):
    """Return (new DBC bytes or None if unchanged, added, removed, skipped)."""
    dbc = Dbc(raw)
    if dbc.fields != table["fields"]:
        raise Abort("field count %d, expected %d" % (dbc.fields, table["fields"]))
    string_fields = set(table["string_fields"])
    by_id = {rec[0]: i for i, rec in enumerate(dbc.records)}
    added = removed = skipped = 0

    if uninstall:
        ours = {row["id"]: row for row in table["rows"]}
        keep = []
        for rec in dbc.records:
            row = ours.get(rec[0])
            if row and row_matches(dbc, rec, row, string_fields):
                removed += 1
            else:
                keep.append(rec)
        dbc.records = keep
        return (dbc.build() if removed else None), 0, removed, 0

    groups = string_groups(string_fields)
    for row in table["rows"]:
        if row["id"] in by_id:
            if row_matches(dbc, dbc.records[by_id[row["id"]]], row, string_fields):
                skipped += 1
                continue
            raise Abort("id %d is already used by another row; another patch uses "
                        "the same id range" % row["id"])
        rec = list(row["values"])
        for field, text in row["strings"].items():
            rec[int(field)] = dbc.add_text(text)
        # Text is authored once (the enUS column); show it in every locale.
        for base in groups:
            for k in range(1, LOCALES_PER_STRING):
                if str(base + k) not in row["strings"] and base + k in string_fields:
                    rec[base + k] = rec[base]
        dbc.records.append(rec)
        added += 1
    return (dbc.build() if added else None), added, 0, skipped


# --------------------------------------------------------------- the client

def looks_like_client(path):
    return path and os.path.isdir(os.path.join(path, "Data")) and (
        os.path.isfile(os.path.join(path, "Wow.exe"))
        or os.path.isfile(os.path.join(path, WILDCARD_MANIFEST))
        or os.path.isdir(os.path.join(path, "World of Warcraft.app")))


def find_client(arg):
    candidates = [arg] if arg else [os.path.dirname(HERE), HERE, os.getcwd()]
    for path in candidates:
        if looks_like_client(path):
            return os.path.abspath(path)
    if arg:
        raise Abort("%s does not look like a WoW 3.3.5a folder (no Data folder / Wow.exe)" % arg)
    raise Abort("WoW folder not found. Put the ItemAffixesPatch folder in your WoW "
                "folder (beside Wow.exe), or pass the folder: ItemAffixesPatch.py \"C:\\Games\\WoW\"")


def wildcard_archives(wow):
    path = os.path.join(wow, WILDCARD_MANIFEST)
    if not os.path.isfile(path):
        raise Abort("the mod-classless-wildcard client installer has not been run on this "
                    "client (%s missing). Run it first, then this patcher." % WILDCARD_MANIFEST)
    with open(path, encoding="utf-8") as fh:
        suffix = json.load(fh).get("suffix")
    if not suffix:
        raise Abort("%s has no patch letter; re-run the Wildcard installer" % WILDCARD_MANIFEST)
    data = os.path.join(wow, "Data")
    found = []
    base = os.path.join(data, "patch-%s.MPQ" % suffix)
    for entry in sorted(os.listdir(data)):
        sub = os.path.join(data, entry)
        if os.path.isdir(sub):
            for name in os.listdir(sub):
                if name.lower() == ("patch-%s-%s.mpq" % (entry, suffix)).lower():
                    found.append(os.path.join(sub, name))
    for name in os.listdir(data):
        if name.lower() == os.path.basename(base).lower():
            found.insert(0, os.path.join(data, name))
    if not found:
        raise Abort("no Wildcard patch archives (patch-%s) in %s; re-run the Wildcard installer"
                    % (suffix, data))
    return found


def addon_sources():
    for base in (os.path.join(HERE, "AddOns"), os.path.join(HERE, "..", "addon")):
        if os.path.isdir(os.path.join(base, "ItemAffixes")):
            return [os.path.join(base, d) for d in ("ItemAffixes", "ItemAffixesToast")
                    if os.path.isdir(os.path.join(base, d))]
    return []


def main():
    ap = argparse.ArgumentParser(description="mod-item-affixes client patcher (classless branch)")
    ap.add_argument("wow", nargs="?", help="WoW 3.3.5a folder (found on its own if omitted)")
    ap.add_argument("--uninstall", action="store_true", help="take the rows and the addon out again")
    ap.add_argument("--dry-run", action="store_true", help="show what would change, write nothing")
    ap.add_argument("--no-addon", action="store_true", help="leave Interface/AddOns alone")
    args = ap.parse_args()

    with open(ROWS_FILE, encoding="utf-8") as fh:
        tables = json.load(fh)["dbc"]

    wow = find_client(args.wow)
    print("mod-item-affixes client patcher")
    print("===============================")
    print("World of Warcraft : %s" % wow)
    print("Mode              : %s%s" % ("uninstall" if args.uninstall else "install",
                                        " (dry run)" if args.dry_run else ""))
    print()

    # Every archive is checked before any is written, so a conflict in one
    # never leaves another half patched.
    pending = []
    for path in wildcard_archives(wow):
        rel = os.path.relpath(path, wow)
        archive = Archive(path)
        replaced = {}
        for name, internal in DBC_PATHS.items():
            if not archive.has(internal):
                print("  %-28s %-22s not in this archive, skipped" % (rel, name))
                continue
            try:
                new, added, removed, skipped = apply_rows(archive.read(internal), tables[name],
                                                          args.uninstall)
            except Abort as error:
                raise Abort("%s %s: %s" % (rel, name, error))
            if args.uninstall:
                print("  %-28s %-22s %d row(s) removed" % (rel, name, removed))
            else:
                print("  %-28s %-22s %d row(s) added, %d already there" % (rel, name, added, skipped))
            if new is not None:
                replaced[internal] = new
        if replaced:
            pending.append((archive, replaced, path, rel))

    if not args.dry_run:
        for archive, replaced, path, rel in pending:
            try:
                write_archive(archive, replaced, path)
            except PermissionError:
                raise Abort("cannot write %s -- close World of Warcraft and run again" % rel)
    touched = len(pending)

    if not args.no_addon:
        target = os.path.join(wow, "Interface", "AddOns")
        for src in addon_sources():
            dst = os.path.join(target, os.path.basename(src))
            if args.uninstall:
                if os.path.isdir(dst) and not args.dry_run:
                    shutil.rmtree(dst)
                print("  addon %-22s removed" % os.path.basename(src))
            else:
                if not args.dry_run:
                    if os.path.isdir(dst):
                        shutil.rmtree(dst)
                    os.makedirs(target, exist_ok=True)
                    shutil.copytree(src, dst)
                print("  addon %-22s -> Interface/AddOns/%s" % (os.path.basename(src), os.path.basename(src)))
        if not addon_sources():
            print("  addon                        not shipped with this patcher, skipped")

    print()
    if args.dry_run:
        print("Dry run: nothing was written.")
    elif args.uninstall:
        print("Done. The custom spells and the addon are gone.")
    else:
        print("Done%s. Start the game." % ("" if touched else " (spells were already in place)"))
        print("Run this again after every run of the Wildcard installer.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Abort as error:
        print("\nERROR: %s" % error)
        sys.exit(1)
