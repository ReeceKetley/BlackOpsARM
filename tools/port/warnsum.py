import re, collections, sys
from pathlib import Path
log = sys.argv[1]
codes = ['C4311', 'C4312', 'C4302']
pat = re.compile(r'BlackOps64\\(?:src|tl)\\(.*?)\((\d+),\d+\): warning (C43(?:11|12|02))')
seen = {}
raw = Path(log).read_bytes()
encoding = 'utf-16' if raw.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig'
for line in raw.decode(encoding, errors='replace').splitlines():
    m = pat.search(line)
    if m:
        f = m.group(1).replace('\\', '/')
        key = (f, m.group(2))
        seen[key] = line.strip()
by_file = collections.Counter()
by_dir = collections.Counter()
for (f, ln) in seen:
    by_file[f] += 1
    by_dir[f.split('/')[0]] += 1
print('unique warnings', len(seen))
print('--- by dir')
for d, n in by_dir.most_common(40):
    print(n, d)
print('--- top files')
for f, n in by_file.most_common(int(sys.argv[2]) if len(sys.argv) > 2 else 60):
    print(n, f)
