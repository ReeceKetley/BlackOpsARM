import re, sys, collections

def norm(line):
    line = line.rstrip('\r\n')
    line = re.sub(r'0x[0-9a-fA-F]+', '0xN', line)
    line = re.sub(r'-?\d+(\.\d+)?', 'N', line)
    return line.strip()

def load(p):
    c = collections.Counter()
    for l in open(p, encoding='latin-1'):
        if l.startswith('bo1_') or l.startswith('BO1_'):
            # harness chatter has per-run timing numbers; keep the prefix + first words
            n = norm(l)[:60]
        else:
            n = norm(l)
        if n:
            c[n] += 1
    return c

a = load(sys.argv[1])
b = load(sys.argv[2])
only_a = [(k, v) for k, v in a.items() if k not in b]
only_b = [(k, v) for k, v in b.items() if k not in a]
print('== only in', sys.argv[1], len(only_a))
for k, v in sorted(only_a, key=lambda x: -x[1])[:int(sys.argv[3]) if len(sys.argv) > 3 else 40]:
    print(v, k[:170])
print('== only in', sys.argv[2], len(only_b))
for k, v in sorted(only_b, key=lambda x: -x[1])[:int(sys.argv[3]) if len(sys.argv) > 3 else 40]:
    print(v, k[:170])
