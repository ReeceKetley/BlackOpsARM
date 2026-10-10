#!/usr/bin/env python3
"""Compare two BO1_STREAM_TRACE files (Win32 reference vs x64) and report the first divergence.

Per-asset mode (default): compares 'ZONE' and 'S index type w0..w6' lines.
Op mode (when both traces were made with BO1_TRACE_ZONE/BO1_TRACE_ASSET): compares 'A|I block pos' lines
inside the 'B index type' ... 'S index type' span.

usage: trace_diff.py w32_trace.txt x64_trace.txt
"""
import sys


def load(path):
    zones = []          # list of (zone, [lines])
    cur = None
    with open(path, encoding="utf-8", errors="replace") as f:
        for raw in f:
            line = raw.rstrip("\n")
            if line.startswith("ZONE "):
                cur = (line[5:], [])
                zones.append(cur)
            elif cur is not None:
                if line.startswith("S "):
                    # block 0 (temp) position is not comparable: Win32 keeps it stale in g_streamPosArray[0]
                    t = line.split()
                    line = " ".join(t[:3] + t[4:])
                cur[1].append(line)
    return zones


def main():
    a = load(sys.argv[1])
    b = load(sys.argv[2])
    print(f"zones: ref={[z for z, _ in a]}")
    print(f"       x64={[z for z, _ in b]}")
    for (za, la), (zb, lb) in zip(a, b):
        if za != zb:
            print(f"zone mismatch: {za} vs {zb}")
            return 1
        n = min(len(la), len(lb))
        for i in range(n):
            if la[i] != lb[i]:
                print(f"FIRST DIVERGENCE zone={za} line={i}")
                for j in range(max(0, i - 3), min(n, i + 2)):
                    mark = ">>" if j == i else "  "
                    print(f" {mark} ref: {la[j]}")
                    print(f" {mark} x64: {lb[j]}")
                return 1
        if len(la) != len(lb):
            print(f"zone {za}: length differs ref={len(la)} x64={len(lb)} (x64 stopped early or ran long)")
            if len(lb) < len(la):
                print(f"  last x64 line: {lb[-1] if lb else '<none>'}")
                print(f"  next ref line: {la[len(lb)]}")
            return 1
        print(f"zone {za}: {len(la)} lines identical")
    if len(a) != len(b):
        print(f"x64 loaded {len(b)} of {len(a)} zones")
    return 0


if __name__ == "__main__":
    sys.exit(main())
