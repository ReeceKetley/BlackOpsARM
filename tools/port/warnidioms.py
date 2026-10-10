import re, collections, sys
log, filt = sys.argv[1], sys.argv[2]
root = r'D:\Desktop\Black Ops 2 Android\BlackOps64'
pat = re.compile(r'BlackOps64\\((?:src|tl)\\.*?)\((\d+),\d+\): warning (C43(?:11|12|02)): (.*?) \[')
seen = {}
for line in open(log, encoding='utf-8', errors='replace'):
    m = pat.search(line)
    if m and filt in m.group(1).replace('\\', '/'):
        seen[(m.group(1), int(m.group(2)))] = (m.group(3), m.group(4))
cache = {}
idioms = collections.Counter()
examples = {}
for (f, ln), (code, msg) in sorted(seen.items()):
    if f not in cache:
        cache[f] = open(root + '\\' + f, encoding='latin-1').read().split('\n')
    src = cache[f][ln - 1].strip()
    norm = re.sub(r'\b[A-Za-z_]\w*\b', 'X', src)
    norm = re.sub(r'\d+', 'N', norm)
    key = (code, msg[:60], norm[:110])
    idioms[key] += 1
    examples.setdefault(key, (f, ln, src[:140]))
print('unique sites', len(seen), 'distinct idioms', len(idioms))
for key, n in idioms.most_common(int(sys.argv[3]) if len(sys.argv) > 3 else 25):
    f, ln, src = examples[key]
    print(f'{n:4d} {key[0]} {key[1]}\n       e.g. {f.split(chr(92))[-1]}:{ln}: {src}')
