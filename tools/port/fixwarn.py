"""Apply idiom rewrites to lines flagged by C4311/C4312/C4302 in a build log.
usage: fixwarn.py <log> <rule-set> <file-substring> [--apply]
Only touches flagged lines; never changes line counts. Prints lines it could not fix."""
import re, sys, collections

log, ruleset, filt = sys.argv[1], sys.argv[2], sys.argv[3]
apply = '--apply' in sys.argv
root = r'D:\Desktop\Black Ops 2 Android\BlackOps64'
pat = re.compile(r'BlackOps64\\((?:src|tl)\\.*?)\((\d+),\d+\): warning (C43(?:11|12|02)): (.*?) \[')
flag = collections.defaultdict(dict)
for line in open(log, encoding='utf-8', errors='replace'):
    m = pat.search(line)
    if m and filt in m.group(1).replace('\\', '/'):
        flag[m.group(1)][int(m.group(2))] = (m.group(3), m.group(4))

PTR_CAST = r'\(\s*(?:const\s+)?(?:unsigned\s+)?(?:__int8|_BYTE|char|uint8|void|\w+)\s*\*\s*\)'

def rules_expr(code, msg, s):
    o = s
    if code in ('C4311', 'C4302'):
        # X.intVal = (int)EXPR;  ->  X.string = (const char *)EXPR;
        s = re.sub(r'(\b[\w\.\->\[\]\(\)]*?internals)\.intVal\s*=\s*\(int\)\s*(?=[^\s])', r'\1.string = (const char *)', s)
        s = re.sub(r'(\b[\w\.\->\[\]]+)\.intVal\s*=\s*\(int\)\s*(?=[^\s])', r'\1.string = (const char *)', s) if s == o else s
    if code == 'C4312':
        s = re.sub(r'(' + PTR_CAST + r'\s*)([\w\.\->\[\]]+)\.intVal', r'\1\2.string', s)
    return s

def rules_vec(code, msg, s):
    o = s
    X = r'([A-Za-z_][\w]*(?:\s*(?:\.|->)\s*[A-Za-z_]\w*|\[[^\]]*\])*)'
    # *(float *)(V->u.intValue + 4*k)  ->  V->u.vectorValue[k]
    s = re.sub(r'\*\(\s*float\s*\*\)\s*\(\s*' + X + r'\s*(\.|->)\s*u\.intValue\s*\+\s*(\d+)\s*\)',
               lambda m: '%s%su.vectorValue[%d]' % (m.group(1), m.group(2), int(m.group(3)) // 4), s)
    s = re.sub(r'\*\(\s*float\s*\*\)\s*' + X + r'\s*(\.|->)\s*u\.intValue(?![\w])',
               lambda m: '%s%su.vectorValue[0]' % (m.group(1), m.group(2)), s)
    # (const float *)V->u.intValue -> V->u.vectorValue
    s = re.sub(r'\(\s*(?:const\s+)?float\s*\*\)\s*' + X + r'\s*(\.|->)\s*u\.intValue(?![\w])',
               lambda m: '(const float *)%s%su.vectorValue' % (m.group(1), m.group(2)), s)
    # V->u.intValue = (int)ptr;  -> V->u.vectorValue = ptr;   (only on lines flagged as pointer truncation)
    s = re.sub(r'' + X + r'\s*(\.|->)\s*u\.intValue\s*=\s*\(int\)\s*([A-Za-z_]\w*)\s*;',
               lambda m: '%s%su.vectorValue = %s;' % (m.group(1), m.group(2), m.group(3)), s)
    s = re.sub(r'' + X + r'\s*(\.|->)\s*u\.intValue\s*=\s*\(int\)\s*(Scr_AllocVector\([^;]*\))\s*;',
               lambda m: '%s%su.vectorValue = %s;' % (m.group(1), m.group(2), m.group(3)), s)
    return s

RULES = {'expr': rules_expr, 'vec': rules_vec}
fn = RULES[ruleset]
tot = fixed = 0
unfixed = []
for f, lines in flag.items():
    path = root + '\\' + f
    src = open(path, encoding='latin-1', newline='').read()
    eol = '\r\n' if '\r\n' in src else '\n'
    L = src.replace('\r\n', '\n').split('\n')
    for ln, (code, msg) in sorted(lines.items()):
        tot += 1
        old = L[ln - 1]
        new = fn(code, msg, old)
        if new != old:
            fixed += 1
            L[ln - 1] = new
        else:
            unfixed.append((f, ln, old.strip()[:150]))
    if apply:
        open(path, 'w', encoding='latin-1', newline='').write(eol.join(L))
print(f'flagged {tot} fixed {fixed} unfixed {len(unfixed)}')
for f, ln, s in unfixed[:60]:
    print(f'  {f.split(chr(92))[-1]}:{ln}: {s}')
