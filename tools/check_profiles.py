#!/usr/bin/env python3
"""Check that a profile's signatures match exactly once in an unpacked exe.

usage: check_profiles.py PROFILE.ini EXE
exit 1 on 0 or 2+ matches for any sig_* key.
"""
import re
import struct
import sys


def text_section(data):
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    opt = struct.unpack_from("<H", data, pe + 20)[0]
    sec = pe + 24 + opt
    for i in range(nsec):
        off = sec + i * 40
        name = data[off:off + 8].rstrip(b"\0")
        if name == b".text":
            vsize, va, rsize, raw = struct.unpack_from("<IIII", data, off + 8)
            return data[raw:raw + min(vsize, rsize)]
    raise SystemExit("no .text section")


def compile_sig(sig):
    parts = []
    for tok in sig.split():
        if tok.startswith("?"):
            parts.append(b".")
        else:
            parts.append(re.escape(bytes([int(tok, 16)])))
    return re.compile(b"".join(parts), re.DOTALL)


def read_sigs(path):
    sigs = []
    section = None
    for line in open(path, encoding="utf-8"):
        line = line.strip()
        if not line or line[0] in ";#":
            continue
        if line[0] == "[":
            section = line[1:line.index("]")].strip().lower()
            continue
        if "=" not in line or section in (None, "settings"):
            continue
        k, v = (s.strip() for s in line.split("=", 1))
        if k.lower().startswith("sig_"):
            sigs.append((section, k, v))
    return sigs


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    text = text_section(open(sys.argv[2], "rb").read())
    bad = 0
    for section, key, sig in read_sigs(sys.argv[1]):
        n = len(list(compile_sig(sig).finditer(text)))
        ok = n == 1
        bad += not ok
        print(f"{'ok ' if ok else 'BAD'} [{section}] {key}: {n} match(es)")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
