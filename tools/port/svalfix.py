"""Rewrite raw parse-node address arithmetic on sval_u.stringValue into typed node[] access.
usage: svalfix.py [--apply] file...   (prints leftover stringValue derefs it did not convert)"""
import re, sys

apply = '--apply' in sys.argv
files = [a for a in sys.argv[1:] if not a.startswith('--')]
X = r'([A-Za-z_][\w]*(?:\s*(?:\.|->)\s*[A-Za-z_]\w*|\[[^\]]*\])*)'   # val, expr, ifStatBlock, a.b, a->b, a[1]


def idx(n):
    n = int(n)
    assert n % 4 == 0, n
    return n // 4


def sub_all(s):
    o = s
    # --- pointer slots stored in a node: SVAL_PTR_AT(sval_u, <addr>)
    s = re.sub(r'SVAL_PTR_AT\(\s*sval_u\s*,\s*\(\s*\*\(unsigned int \*\)\s*' + X + r'\.stringValue\s*\+\s*(\d+)\s*\)\s*\)',
               lambda m: '%s.node[0].node[%d].node' % (m.group(1), idx(m.group(2))), s)
    s = re.sub(r'SVAL_PTR_AT\(\s*sval_u\s*,\s*' + X + r'\.stringValue\s*\)', lambda m: '%s.node[0].node' % m.group(1), s)
    # --- field reads/writes at +N
    s = re.sub(r'\*\(\s*sval_u\s*\*\)\s*\(\s*' + X + r'\s*(\.|->)\s*stringValue\s*\+\s*(\d+)\s*\)',
               lambda m: '%s%snode[%d]' % (m.group(1), m.group(2), idx(m.group(3))), s)
    s = re.sub(r'\(\s*sval_u\s*\*\)\s*\(\s*' + X + r'\s*(\.|->)\s*stringValue\s*\+\s*(\d+)\s*\)',
               lambda m: '(&%s%snode[%d])' % (m.group(1), m.group(2), idx(m.group(3))), s)
    s = re.sub(r'\*\(\s*(?:unsigned int|_DWORD)\s*\*\)\s*\(\s*' + X + r'\s*(\.|->)\s*stringValue\s*\+\s*(\d+)\s*\)',
               lambda m: '%s%snode[%d].stringValue' % (m.group(1), m.group(2), idx(m.group(3))), s)
    s = re.sub(r'\*\(\s*(?:unsigned int|_DWORD)\s*\*\)\s*' + X + r'\s*(\.|->)\s*stringValue(?![\w])',
               lambda m: '%s%snode[0].stringValue' % (m.group(1), m.group(2)), s)
    s = re.sub(r'\*\(\s*_BYTE\s*\*\)\s*' + X + r'\s*(\.|->)\s*stringValue(?![\w])\s*=\s*([^;]+);',
               lambda m: '%s%snode[0].intValue = %s;' % (m.group(1), m.group(2), m.group(3)), s)
    s = re.sub(r'\*\(\s*_BYTE\s*\*\)\s*' + X + r'\s*(\.|->)\s*stringValue(?![\w])',
               lambda m: '(unsigned __int8)%s%snode[0].type' % (m.group(1), m.group(2)), s)
    s = re.sub(r'\(\s*scr_block_s\s*\*\)\s*' + X + r'\s*(\.|->)\s*stringValue(?![\w])',
               lambda m: '%s%sblock' % (m.group(1), m.group(2)), s)
    return s


leftovers = []
for f in files:
    src = open(f, encoding='latin-1', newline='').read()
    eol = '\r\n' if '\r\n' in src else '\n'
    L = src.replace('\r\n', '\n').split('\n')
    n = 0
    for i, line in enumerate(L):
        new = sub_all(line)
        if new != line:
            L[i] = new
            n += 1
    for i, line in enumerate(L):
        if re.search(r'stringValue\s*\+|\(\s*(?:unsigned int|int)\s*\)\s*Scr_AllocNode|\(\s*(?:sval_u|char|const char|_BYTE|_DWORD|unsigned int|float|scr_block_s)\s*\*\)\s*[\w\.\->]*stringValue|\*\(\s*[\w ]+\*\)\s*[\w\.\->]*stringValue', line):
            leftovers.append((f.split('\\')[-1], i + 1, line.strip()[:150]))
    print(f.split('\\')[-1], 'rewrote', n, 'lines')
    if apply:
        open(f, 'w', encoding='latin-1', newline='').write(eol.join(L))
print('leftovers', len(leftovers))
for f, ln, s in leftovers[:80]:
    print(f'  {f}:{ln}: {s}')
