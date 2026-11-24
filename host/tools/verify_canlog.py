#!/usr/bin/env python3
"""tools/verify_canlog.py <candump -L log> <scenario>

Checks a SocketCAN capture of a two-process run: every frame is a Classical
CAN 2.0B extended frame, every known identifier has its specified DLC, payload
spot checks, and scenario-specific frames. Expected IDs and bytes are the same
derived/PROVISIONAL values used by the unit tests (docs/PROVISIONAL_PARAMETERS.md).
Exit 0 = all checks passed.
"""
import re
import sys

LEN = {  # identifier -> (name, DLC) ; GB/T Tables 3-7, TP frames 8
    0x1826F456: ("CHM", 3), 0x182756F4: ("BHM", 2), 0x1801F456: ("CRM", 8),
    0x1808F456: ("CML", 8), 0x100956F4: ("BRO", 1), 0x100AF456: ("CRO", 1),
    0x181056F4: ("BCL", 5), 0x1812F456: ("CCS", 7), 0x181356F4: ("BSM", 7),
    0x101956F4: ("BST", 4), 0x101AF456: ("CST", 4), 0x181C56F4: ("BSD", 7),
    0x181DF456: ("CSD", 8), 0x081E56F4: ("BEM", 4), 0x081FF456: ("CEM", 4),
    0x1CEC56F4: ("TP.CM V->C", 8), 0x1CECF456: ("TP.CM C->V", 8), 0x1CEB56F4: ("TP.DT V->C", 8),
}
LINE = re.compile(r"^\((\d+\.\d+)\)\s+(\S+)\s+([0-9A-Fa-f]+)#([0-9A-Fa-f]*)$")


def main():
    path, scenario = sys.argv[1], sys.argv[2]
    frames, errors = [], []
    with open(path, encoding="ascii") as fh:
        for raw in fh:
            raw = raw.strip()
            if not raw:
                continue
            m = LINE.match(raw)
            if not m:
                errors.append("unparsable line: " + raw)
                continue
            ident, data = m.group(3), bytes.fromhex(m.group(4))
            if len(ident) != 8:
                errors.append("not an extended frame: " + raw)
                continue
            frames.append((float(m.group(1)), int(ident, 16), data))
    if not frames:
        errors.append("no frames captured")
    by = {}
    for t, i, d in frames:
        by.setdefault(i, []).append((t, d))
        if i not in LEN:
            errors.append("unexpected identifier 0x%08X" % i)
        elif len(d) != LEN[i][1]:
            errors.append("%s DLC %d != %d" % (LEN[i][0], len(d), LEN[i][1]))

    def need(i):
        if i not in by:
            errors.append("missing %s (0x%08X)" % (LEN[i][0], i))
            return []
        return by[i]

    for i in (0x1826F456, 0x182756F4, 0x1801F456, 0x1CEC56F4, 0x1CECF456, 0x1CEB56F4,
              0x1808F456, 0x100956F4, 0x100AF456, 0x181056F4, 0x1812F456):
        need(i)
    if by.get(0x1826F456) and by[0x1826F456][0][1] != bytes([1, 1, 0]):
        errors.append("CHM version bytes")
    if not any(d[0] == 0xAA for _, d in by.get(0x1801F456, [])):
        errors.append("no CRM 0xAA")
    for nm, val in (("BRO", 0x100956F4), ("CRO", 0x100AF456)):
        seq = [d[0] for _, d in by.get(val, [])]
        if not (0x00 in seq and 0xAA in seq and seq.index(0x00) < seq.index(0xAA)):
            errors.append("%s 0x00 before 0xAA not seen" % nm)
    rts = {(d[5] | d[6] << 8 | d[7] << 16, d[1] | d[2] << 8) for _, d in by.get(0x1CEC56F4, []) if d[0] == 16}
    for pgn, size in ((0x000200, 49), (0x000600, 13), (0x001100, 9)):
        if (pgn, size) not in rts:
            errors.append("no RTS for PGN 0x%06X size %d" % (pgn, size))
    eom = {d[5] | d[6] << 8 | d[7] << 16 for _, d in by.get(0x1CECF456, []) if d[0] == 19}
    for pgn in (0x000200, 0x000600, 0x001100):
        if pgn not in eom:
            errors.append("no EOM ACK for PGN 0x%06X" % pgn)
    if by.get(0x181056F4) and by[0x181056F4][0][1] != bytes.fromhex("A00FB80B02"):
        errors.append("BCL payload (400.0 V, -100.0 A, CC) mismatch")

    if scenario in ("normal", "charger-stop"):
        for i in (0x181356F4, 0x101956F4, 0x101AF456, 0x181C56F4, 0x181DF456):
            need(i)
        if 0x081E56F4 in by or 0x081FF456 in by:
            errors.append("unexpected BEM/CEM")
        cst = by.get(0x101AF456)
        if cst:
            exp = 0x40 if scenario == "normal" else 0x04
            if cst[0][1][0] != exp:
                errors.append("CST SPN3521 byte 0x%02X != 0x%02X" % (cst[0][1][0], exp))
        if scenario == "charger-stop" and len(by.get(0x101956F4, [])) != 5:
            errors.append("BST frames %d != 5" % len(by.get(0x101956F4, [])))
    elif scenario == "bcl-timeout":
        cem = need(0x081FF456)
        if cem and cem[0][1][2] != 0xC4:
            errors.append("CEM SPN3925 not set (byte3 0x%02X)" % cem[0][1][2])
        if cem and len(cem) != 4:
            errors.append("CEM frames %d != 4" % len(cem))
    elif scenario == "ccs-timeout":
        bem = need(0x081E56F4)
        if bem and bem[0][1][2] != 0xF1:
            errors.append("BEM SPN3905 not set (byte3 0x%02X)" % bem[0][1][2])
    else:
        errors.append("unknown scenario " + scenario)

    names = sorted("%s:%d" % (LEN.get(i, ("?", 0))[0], len(v)) for i, v in by.items())
    print("%s: %d frames, %d identifiers [%s]" % (scenario, len(frames), len(by), ", ".join(names)))
    for e in errors:
        print("VERIFY FAIL: " + e)
    print("VERIFY " + ("PASS" if not errors else "FAIL"))
    return 0 if not errors else 1


if __name__ == "__main__":
    sys.exit(main())
