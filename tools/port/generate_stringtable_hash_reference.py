"""Generate an ignored MSVC-only control from the exact pre-port hash body.

The original signed-overflow/ctype behavior is intentionally confined to the
Windows reference control. Production and ARM64 use the canonical safe leaf.
No generated duplicate engine source is tracked in Git.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCE = 'src/server_mp/sv_bot_mp.cpp'
REVISION = '1874245786ef55145c710d0016f693e123d66d1f'


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('output', type=Path)
    args = ap.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / 'build-android-audit') or output.exists():
        raise RuntimeError('Use a fresh ignored build-android-audit output file')
    commit = subprocess.check_output(['git', '--no-optional-locks', 'rev-parse', '--verify', REVISION + '^{commit}'], cwd=ROOT).decode().strip()
    source = subprocess.check_output(['git', '--no-optional-locks', 'show', commit + ':' + SOURCE], cwd=ROOT)
    matches = list(re.finditer(rb'^int __cdecl Com_HashString\(const char \*fname, int len\)\r?\n\{.*?^\}', source, re.M | re.S))
    if len(matches) != 1:
        raise RuntimeError('Cannot identify exactly one original production Com_HashString body')
    original = matches[0].group()
    renamed = original.replace(b'Com_HashString(', b'WindowsOriginalHash(', 1)
    if renamed.replace(b'WindowsOriginalHash(', b'Com_HashString(', 1) != original:
        raise RuntimeError('Reference arithmetic/control changed during generation')
    generated = b'#include <ctype.h>\n#include <stdint.h>\n\n' + renamed + b'\n'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(generated)
    proof = {'schema': 'BlackOpsARM.TASK-0022.original-windows-hash-reference.v1',
             'original_commit': commit, 'original_source': SOURCE,
             'original_git_blob_sha256': hashlib.sha256(source).hexdigest(),
             'original_function_sha256': hashlib.sha256(original).hexdigest(),
             'generated_source_sha256': hashlib.sha256(generated).hexdigest(),
             'original_function_bytes': len(original),
             'only_function_name_changed': True,
             'compiler_contract': 'MSVC Windows x86/x64 /O2; exact historical signed arithmetic and ctype body',
             'scope': 'Standalone ignored original-behavior control only; not ARM64 production code or a tracked engine fork'}
    output.with_suffix('.sources.json').write_text(json.dumps(proof, indent=2) + '\n', encoding='utf-8')
    print('PASS generated exact historical Windows hash control:', output)


if __name__ == '__main__':
    main()
