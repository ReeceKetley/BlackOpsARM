"""Reverse only exact lookup selectors to prove unchanged default source bytes."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import sys

sys.dont_write_bytecode = True
from verify_string_pool_source import functions

ROOT = Path(__file__).resolve().parents[2]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def lf(data):
    data = data.replace(b'\r\n', b'\n')
    if b'\r' in data:
        raise RuntimeError('Unexpected bare CR in source')
    return data


def canonical(data):
    return lf(data).replace(b'\n', b'\r\n')


def once(data, before, after, label):
    if data.count(before) != 1:
        raise RuntimeError('Expected unique exact selector: ' + label)
    return data.replace(before, after, 1)


def leaf(data, signature):
    if data.count(signature + b'\n{') != 1:
        raise RuntimeError('Selected leaf is not uniquely defined: ' + signature.decode())
    start = data.index(signature + b'\n{')
    position = start + len(signature) + 2
    depth = 1
    while depth and position < len(data):
        if data[position:position + 1] == b'{':
            depth += 1
        elif data[position:position + 1] == b'}':
            depth -= 1
        position += 1
    if depth:
        raise RuntimeError('Unterminated selected function')
    return data[start:position]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / 'build-android-audit') or output.exists():
        raise RuntimeError('Use a fresh ignored proof output inside BlackOpsARM')
    baseline = json.loads(args.baseline.read_text(encoding='utf-8'))
    checkpoint = baseline['BlackOpsARM']['head']
    result = {'schema': 'BlackOpsARM.TASK-0023.source-preservation.v1', 'checkpoint': checkpoint,
              'baseline_sha256': sha(args.baseline.read_bytes()), 'verifier_sha256': sha(Path(__file__).read_bytes()),
              'normalization': 'Exact default CRLF bytes after reversing only documented selectors',
              'files': {}, 'functions': {}}

    def original(name):
        data = subprocess.check_output(['git', '--no-optional-locks', 'show', checkpoint + ':' + name], cwd=ROOT)
        normalized = canonical(data)
        if baseline['BlackOpsARM']['files'][name].upper() not in {sha(data).upper(), sha(normalized).upper(), sha(lf(data)).upper()}:
            raise RuntimeError('Captured source baseline differs from Git: ' + name)
        return normalized

    footer = b'\n#endif // !BO1_STRINGTABLE_LOOKUP_GATE\n'
    name = 'src/universal/q_shared.cpp'
    old = original(name)
    working = (ROOT / name).read_bytes()
    default = once(lf(working),
        b'#if defined(BO1_STRINGTABLE_LOOKUP_GATE)\n#include <platform/bo1_native_width.h>\n#include "assertive.h"\n#else\n',
        b'', 'ASCII comparison-only includes')
    signatures = [b'bool __cdecl I_isupper(int c)',
                  b'int __cdecl I_strnicmp(const char *s0, const char *s1, int n)',
                  b'int __cdecl I_stricmp(const char *s0, const char *s1)']
    for signature in signatures:
        body = leaf(default, signature)
        parent_body = leaf(lf(old), signature)
        if canonical(body) != canonical(parent_body) or functions(canonical(body)) != functions(canonical(parent_body)):
            raise RuntimeError('Actual ASCII comparison leaf changed')
        default = once(default, b'#endif\n' + signature + b'\n', signature + b'\n', 'ASCII leaf opening')
        default = once(default, body + b'\n#if !defined(BO1_STRINGTABLE_LOOKUP_GATE)', body, 'ASCII leaf closing')
        function_name = signature.split(b'(')[0].split()[-1].decode()
        result['functions'][function_name] = {'sha256': sha(canonical(body)), 'unique_body_byte_unchanged': True}
    if not default.endswith(footer):
        raise RuntimeError('Missing exact comparison selector footer')
    default = default[:-len(footer)]
    if canonical(default) != old:
        raise RuntimeError('Default q_shared.cpp bytes/globals/order changed beyond selectors')
    result['files'][name] = {'current_sha256': sha(working), 'parent_crlf_sha256': sha(old),
                            'restored_default_crlf_sha256': sha(canonical(default)),
                            'whole_default_source_global_and_function_order_exact': True}
    name = 'src/universal/com_stringtable.cpp'
    old = original(name)
    working = (ROOT / name).read_bytes()
    default = once(lf(working),
        b'#if defined(BO1_STRINGTABLE_LOOKUP_GATE)\n#include "com_stringtable.h"\n#include "com_stringtable_obj.h"\n'
        b'int __cdecl I_stricmp(const char*, const char*);\nvoid Com_PrintError(int, const char*, ...);\n#else\n',
        b'', 'light lookup includes and declarations')
    row = b'int __cdecl StringTable_LookupRowNumForValue(const StringTable *table, int comparisonColumn, const char *value)'
    default = once(default, b'#endif\n' + row + b'\n', row + b'\n', 'simple lookup begins')
    default = once(default,
        b'#if !defined(BO1_STRINGTABLE_LOOKUP_GATE)\nint __cdecl StringTable_LookupRowNumForValue(\n',
        b'int __cdecl StringTable_LookupRowNumForValue(\n', 'later overloaded lookups excluded only in gate')
    if not default.endswith(footer):
        raise RuntimeError('Missing exact lookup selector footer')
    default = default[:-len(footer)]
    if canonical(default) != old:
        raise RuntimeError('Default StringTable source changed beyond selectors')
    for signature in [row, b'int __cdecl StringTable_FirstHashIndex(const StringTable *table, int hash)',
                      b'int __cdecl StringTable_LookupColumnNumForValue(const StringTable *table, int comparisonRow, const char *value)']:
        body = leaf(default, signature)
        if canonical(body) != canonical(leaf(lf(old), signature)):
            raise RuntimeError('Actual selected lookup body changed')
        function_name = signature.split(b'(')[0].split()[-1].decode()
        result['functions'][function_name] = {'sha256': sha(canonical(body)), 'unique_body_byte_unchanged': True}
    result['files'][name] = {'current_sha256': sha(working), 'parent_crlf_sha256': sha(old),
                            'restored_default_crlf_sha256': sha(canonical(default)),
                            'whole_default_source_global_and_function_order_exact': True,
                            'selected_branch_uses_only_actual_comparison_and_diagnostic_declarations': True}
    for name in ['src/universal/com_stringtable.h', 'src/universal/com_stringtable_obj.h', 'src/universal/q_shared.h',
                 'src/universal/com_hash.cpp', 'src/universal/com_hash.h', 'src/universal/com_stringtable_obj.cpp']:
        old = original(name)
        working = (ROOT / name).read_bytes()
        if sha(working).upper() != baseline['BlackOpsARM']['files'][name].upper() or canonical(working) != old:
            raise RuntimeError('Previously certified type/hash source changed: ' + name)
        result['files'][name] = {'current_sha256': sha(working), 'full_source_byte_unchanged': True}
    result['status'] = 'passed'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('PASS exact default lookup/comparison source bytes and six unique unchanged production leaves; type/hash sources unchanged')


if __name__ == '__main__':
    main()
