import re, sys
logs = sys.argv[1].split(',')
filt = sys.argv[2]
limit = int(sys.argv[3]) if len(sys.argv) > 3 else 100000
root = r'D:\Desktop\Black Ops 2 Android\BlackOps64'
pat = re.compile(r'BlackOps64\\((?:src|tl)\\.*?)\((\d+),\d+\): warning (C43(?:11|12|02)): (.*?) \[')
seen = {}
for lg in logs:
    for line in open(lg, encoding='utf-8', errors='replace'):
        m = pat.search(line)
        if m and filt in m.group(1).replace('\\', '/'):
            seen[(m.group(1), int(m.group(2)))] = m.group(3)
cache = {}
n = 0
for (f, ln), c in sorted(seen.items()):
    if f not in cache:
        cache[f] = open(root + '\\' + f, encoding='latin-1').read().split('\n')
    print('%s:%d: %s' % (f.split('\\')[-1], ln, cache[f][ln - 1].strip()[:170]))
    n += 1
    if n >= limit:
        break
print(len(seen), 'total')
