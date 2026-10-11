"""Generate named, aborting test endpoints from current production declarations."""
from pathlib import Path
import json
import re
import sys

root = Path(__file__).resolve().parents[2]
out = Path(sys.argv[1]).resolve()
if not out.is_relative_to(root):
    raise RuntimeError('Output must be inside the Android worktree')
headers = ['clientscript/cscr_variable.h', 'clientscript/cscr_compiler.h',
           'clientscript/cscr_evaluate.h',
           'clientscript/cscr_debugger.h',
           'clientscript/cscr_stringlist.h', 'clientscript/cscr_vm.h',
           'clientscript/cscr_parser.h', 'clientscript/cscr_animtree.h',
           'clientscript/cscr_tempmemory.h', 'clientscript/cscr_yacc.h',
           'universal/q_shared.h', 'qcommon/common.h', 'xanim/xanim_debug.h']
sources = [(h, (root / 'src' / h).read_bytes().decode('latin1')) for h in headers]
names = json.loads((root / 'tools/port/tests/evaluator_unavailable_services.json').read_text())['functions']
code = '// Generated test-only endpoints. Never link into an engine or APK.\n'
code += ''.join(f'#include <{h}>\n' for h in headers if h != 'qcommon/common.h')
code += '#include <cstdio>\n#include <cstdlib>\n'
code += '[[noreturn]] static void unavailable(const char *name) { std::fprintf(stderr, "Unavailable evaluator test service: %s\\n", name); std::abort(); }\n'
for name in names:
    declarations = []
    for header, text in sources:
        pattern = r'^([A-Za-z_][^\r\n;{}]*?\b' + re.escape(name) + r'\s*\([^;{}]*);'
        for m in re.finditer(pattern, text, re.M):
            decl = re.sub(r'//[^\r\n]*', '', m.group(1)).strip()
            decl = re.sub(r'\s*=\s*(?:nullptr|false|true|0)\b', '', decl)
            declarations.append(decl)
    if not declarations:
        raise RuntimeError('Declaration not found: ' + name)
    if name == 'Scr_Error':
        declarations = [d for d in declarations if 'scriptInstance_t' in d]
        if not declarations:
            raise RuntimeError('Instance-aware Scr_Error declaration not found')
    # Repeated compatible prototypes in legacy headers are compiler-checked.
    code += declarations[0] + ' { unavailable("' + name + '"); }\n'
code += '''// Test-owned zeroed context, not a VM implementation.
scrVarPub_t gScrVarPub[2]{};
scrCompilePub_t gScrCompilePub[2]{};
scrVmPub_t gScrVmPub[2]{};
unsigned int g_scrVarChildBegin[2]{};
thread_local int g_breakonExpr{}, g_breakonHit{};
thread_local unsigned int g_breakonString{}, g_breakonObject{};
const dvar_t *com_sv_running = nullptr;
'''
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(code, encoding='utf-8')
print(f'Generated {len(names)} explicitly unavailable test endpoints: {out}')
