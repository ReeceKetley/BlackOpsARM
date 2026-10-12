"""Generate an ignored MSVC control containing the exact historical free body.

Only its name changes. The historical typed-alias accesses remain confined to
this Windows reference; they are never compiled as Android production services.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCE = 'src/database/db_registry.cpp'
REVISION = 'fa27a73'
OLD_NAME = b'DB_FreeXAssetHeader_EmblemSet_'
NEW_NAME = b'WindowsOriginalFreeReference'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    proof_path = output.with_suffix('.sources.json')
    if (not output.is_relative_to((ROOT / 'build-android-audit').resolve())
            or output.exists() or proof_path.exists()):
        raise RuntimeError('Use a fresh ignored build-android-audit output file')
    commit = subprocess.check_output(
        ['git', '--no-optional-locks', 'rev-parse', '--verify', REVISION + '^{commit}'],
        cwd=ROOT).decode().strip()
    source = subprocess.check_output(
        ['git', '--no-optional-locks', 'show', commit + ':' + SOURCE], cwd=ROOT)
    pattern = (rb'^void __cdecl DB_FreeXAssetHeader_EmblemSet_'
               rb'\(XAssetPoolEntry<EmblemSet> \*\*pool, XAssetHeader header\)\r?\n'
               rb'\{.*?^\}')
    matches = list(re.finditer(pattern, source, re.M | re.S))
    if len(matches) != 1:
        raise RuntimeError('Cannot identify exactly one historical generic-free definition')
    original = matches[0].group()
    renamed = original.replace(OLD_NAME + b'(', NEW_NAME + b'(', 1)
    if renamed.replace(NEW_NAME + b'(', OLD_NAME + b'(', 1) != original:
        raise RuntimeError('Historical control changed beyond its function name')
    generated = (b'#if !defined(_WIN32)\n#error "Historical Windows reference only"\n#endif\n'
                 b'#include <database/db_registry.h>\n#include <xanim/xmodel.h>\n\n'
                 + renamed + b'\n')
    proof = dict(schema='BlackOpsARM.TASK-0026.original-windows-free-reference.v1',
                 original_commit=commit, original_source=SOURCE,
                 original_git_blob_sha256=hashlib.sha256(source).hexdigest(),
                 original_function_sha256=hashlib.sha256(original).hexdigest(),
                 generated_source_sha256=hashlib.sha256(generated).hexdigest(),
                 original_function_bytes=len(original), only_function_name_changed=True,
                 compiler_contract='MSVC Windows x86/x64 /O2; exact historical typed-alias free body',
                 scope='Ignored standalone control only; no tracked engine fork or Android implementation')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(generated)
    proof_path.write_text(json.dumps(proof, indent=2) + '\n', encoding='utf-8')
    print('PASS generated exact historical Windows free control:', output)


if __name__ == '__main__':
    main()
