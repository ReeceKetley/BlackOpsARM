"""Probe current production declarations, following the existing extraction tests.

Only build-directory test code is generated; no second engine tree is maintained.
This isolates layout checks from the unported Windows profiler in q_shared.h.
It does not certify that the complete script headers/VM compile on Android.
"""
from pathlib import Path
import hashlib
import json
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
output = Path(sys.argv[1])
sources = {}
parts = ['#include <universal/narrow32.h>\n#include <database/db_expand.hpp>\n'
         '#include <cstddef>\n#include <cstdio>\n#include <cstdlib>\n'
         'enum ParseToken_t : int;']
for file, names in [
    ('src/clientscript/cscr_variable.h', ['ObjectInfo_u', 'ObjectInfo', 'VariableStackBuffer',
       'VariableUnion', 'VariableValueInternal_u', 'VariableValueInternal_w',
       'VariableValueInternal_v', 'Variable_u', 'Variable', 'VariableValueInternal', 'VariableValue']),
    ('src/clientscript/cscr_debugger.h', ['scr_localVar_t', 'scr_block_s', 'sval_u']),
]:
    path = ROOT / file
    text = path.read_text(encoding='utf-8')
    sources[file] = hashlib.sha256(path.read_bytes()).hexdigest()
    for name in names:
        match = re.search(r'^(?:union|struct) (?:__declspec\(align\(4\)\) )?' + name +
                          r'\b[^\n]*\n\{.*?^\};', text, re.M | re.S)
        if not match:
            raise RuntimeError('Cannot locate production declaration: ' + name)
        parts.append(match.group())

# Extract the actual platform-selected macro including its surrounding guards.
text = (ROOT / 'src/clientscript/cscr_debugger.h').read_text(encoding='utf-8')
start = text.index('#if BO1_NATIVE_64BIT')
end = text.index('\nunion sval_u', start)
parts.append(text[start:end])
parts.append('''
static_assert(sizeof(VariableUnion) == 4);
static_assert(sizeof(sval_u) == 4);
static_assert(sizeof(VariableValue) == 8);
static_assert(sizeof(VariableValueInternal) == 0x1C);
static_assert(offsetof(VariableStackBuffer, buf) == 13);
static_assert(alignof(VariableUnion) == 4 && alignof(sval_u) == 4);
static_assert(sizeof(unsigned int) == 4);
int main()
{
    char code[] = "code";
    VariableUnion value{};
    value.codePosValue = code;
    VariableUnion copy{};
    unsigned int raw = Bo1_NarrowBits(value.codePosValue);
    Bo1_NarrowSetBits(copy.codePosValue, raw);
    if (static_cast<const char *>(copy.codePosValue) != code) std::abort();
    sval_u tree[2];
    tree[0].node = &tree[1];
    if (static_cast<sval_u *>(tree[0].node) != &tree[1]) std::abort();
#if defined(__ANDROID__)
    if (SVAL_PTR_AT(sval_u, BO1_NARROW_ENCODE(tree)) != &tree[1]) std::abort();
#endif
#if BO1_NATIVE_64BIT
    if (DB_ExpandPtr32(0) != 0 || DB_ExpandPtr32(0x12345678u) != uintptr_t(0x12345678u)
        || DB_ExpandPtr32(0xFFFFFFFFu) != ~uintptr_t(0)
        || DB_ExpandPtr32(0xFFFFFFFEu) != ~uintptr_t(1)) std::abort();
#endif
    std::printf("PASS production declarations: VariableUnion=%zu sval_u=%zu VariableValue=%zu internal=%zu stack.buf=%zu; handles and FF sentinels roundtrip\\n",
        sizeof(VariableUnion), sizeof(sval_u), sizeof(VariableValue), sizeof(VariableValueInternal), offsetof(VariableStackBuffer, buf));
}
''')
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text('\n\n'.join(parts), encoding='utf-8')
output.with_suffix('.sources.json').write_text(json.dumps(sources, indent=2) + '\n')
