"""Prove the unique hash move and exact default StringTable wrapper selection.

ASCII hash behavior is preserved with explicit modulo32 arithmetic. The existing
tolower argument is intentionally unchanged; non-ASCII/locale behavior is outside
this source proof. No source is rewritten by this verifier.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
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


def replace_once(data, before, after, label):
    if data.count(before) != 1:
        raise RuntimeError('Expected one exact documented change: ' + label)
    return data.replace(before, after, 1)


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
    result = {'schema': 'BlackOpsARM.TASK-0022.source-preservation.v1', 'checkpoint': checkpoint,
              'baseline_sha256': sha(args.baseline.read_bytes()), 'verifier_sha256': sha(Path(__file__).read_bytes()),
              'normalization': 'LF/CRLF only, comparison in canonical CRLF', 'files': {},
              'scope': 'Unique production hash relocation, fixed32 arithmetic/bit return, unchanged default wrapper/globals and callers; non-ASCII/locale deferred'}
    cache = {}

    def original(name, reconstructed_raw=None):
        if name not in cache:
            data = subprocess.check_output(['git', '--no-optional-locks', 'show', checkpoint + ':' + name], cwd=ROOT)
            cache[name] = canonical(data)
            observed = baseline['BlackOpsARM']['files'][name].upper()
            if observed not in {sha(data).upper(), sha(cache[name]).upper(), sha(lf(data)).upper()}:
                # The inventory contains historical mixed endings. Reversing
                # only its two exact added lines restores the captured RAW
                # baseline; canonical equality to Git remains an extra check.
                if (reconstructed_raw is None or sha(reconstructed_raw).upper() != observed
                        or canonical(reconstructed_raw) != cache[name]):
                    raise RuntimeError('Captured baseline differs from Git source: ' + name)
        return cache[name]

    name = 'src/server_mp/sv_bot_mp.cpp'
    old = lf(original(name))
    pattern = rb'^int __cdecl Com_HashString\(const char \*fname, int len\)\n\{\n[^}]*\}\n'
    matches = list(re.finditer(pattern, old, re.MULTILINE))
    if len(matches) != 1:
        raise RuntimeError('Expected unique original Com_HashString definition')
    match = matches[0]
    removed = old[match.start():match.end()] + b'\n'
    default = replace_once(old, removed, b'', 'sole old function and its separator newline')
    working = (ROOT / name).read_bytes()
    if canonical(default) != canonical(working):
        raise RuntimeError('Server source changed beyond one hash-function removal')
    result['files'][name] = {'current_sha256': sha(working), 'parent_sha256': sha(original(name)),
                            'only_single_function_removed': True, 'all_other_source_order_exact': True,
                            'removed_function_sha256': sha(canonical(match.group()))}
    name = 'src/server_mp/sv_bot_mp.h'
    old = lf(original(name))
    prototype = b'int __cdecl Com_HashString(const char *fname, int len);'
    original_line = prototype + b' // BO1TODO: move somewhere in qcommon'
    working = (ROOT / name).read_bytes()
    restored = replace_once(lf(working), b'#include <universal/com_hash.h>', original_line, 'original declaration location')
    if canonical(restored) != canonical(old):
        raise RuntimeError('Server header changed beyond moved prototype include')
    result['files'][name] = {'current_sha256': sha(working), 'parent_sha256': sha(original(name)),
                            'single_declaration_include_at_original_position': True}
    name = 'src/universal/com_hash.h'
    working = (ROOT / name).read_bytes()
    if lf(working) != b'#pragma once\n#include <platform/bo1_native_width.h>\n' + prototype + b'\n':
        raise RuntimeError('Hash header has unexpected API/namespace/definition')
    result['files'][name] = {'current_sha256': sha(working), 'exact_unique_original_API': True}
    name = 'src/universal/com_hash.cpp'
    working = (ROOT / name).read_bytes()
    expected = b'''#include "com_hash.h"
#include <cctype>
#include <cstdint>
#include <cstring>

int __cdecl Com_HashString(const char *fname, int len)
{
    static_assert(sizeof(int)==sizeof(std::uint32_t));
    if (!fname)
        return 0;
    std::uint32_t hash=5381;
    for (int i=0; fname[i] && (!len || i<len); ++i)
        hash=static_cast<std::uint32_t>(tolower(fname[i]))+33u*hash;
    // Preserve Windows signed result bits without signed-overflow arithmetic.
    int result;
    std::memcpy(&result,&hash,sizeof(result));
    return result;
}
'''
    if lf(working) != expected:
        raise RuntimeError('Moved hash differs from documented fixed32 body')
    old_body = functions(canonical(match.group()))['Com_HashString']
    old_recipe = b'''int __cdecl Com_HashString(const char *fname, int len)
{
    int hash; // [esp+0h] [ebp-Ch]
    int i; // [esp+8h] [ebp-4h]

    if ( !fname )
        return 0;
    hash = 5381;
    for ( i = 0; fname[i] && (!len || i < len); ++i )
        hash = tolower(fname[i]) + 33 * hash;
    return hash;
}'''
    if old_body != canonical(old_recipe):
        raise RuntimeError('Original hash recipe differs from audited null/seed/length/lowercase contract')
    result['files'][name] = {'current_sha256': sha(working), 'fixed32_body_exact': True,
                            'original_null_seed_length_and_tolower_argument_preserved': True,
                            'return_bits_via_memcpy_int': True, 'global_API_no_namespace_fork': True}
    name = 'src/universal/com_stringtable_obj.cpp'
    old = original(name)
    working = (ROOT / name).read_bytes()
    default = replace_once(lf(working),
        b'#if defined(BO1_STRINGTABLE_HASH_GATE)\n#include <platform/bo1_native_width.h>\n#include "com_stringtable_obj.h"\n#include "com_hash.h"\n#else\n',
        b'', 'hash-only includes')
    default = replace_once(default, b'#endif\nint __cdecl StringTable_HashString(',
        b'int __cdecl StringTable_HashString(', 'wrapper start')
    default = replace_once(default, b'    return Com_HashString(string, 0);\n}\n#if !defined(BO1_STRINGTABLE_HASH_GATE)\n',
        b'    return Com_HashString(string, 0);\n}\n', 'wrapper end')
    footer = b'\n#endif // !BO1_STRINGTABLE_HASH_GATE\n'
    if not default.endswith(footer):
        raise RuntimeError('Unexpected default object-source footer')
    default = default[:-len(footer)]
    if canonical(default) != old:
        raise RuntimeError('Default object source changed beyond exact hash-only selection')
    wrapper_pattern = rb'^int __cdecl StringTable_HashString\([^;]*\)\r?\n\{\r?\n[^}]*\}'
    before = re.findall(wrapper_pattern, old, re.MULTILINE)
    after = re.findall(wrapper_pattern, canonical(working), re.MULTILINE)
    if len(before) != 1 or before != after:
        raise RuntimeError('Actual hash wrapper body is not unique/byte-unchanged')
    result['files'][name] = {'current_sha256': sha(working), 'parent_sha256': sha(old),
                            'whole_default_source_and_global_order_exact': True,
                            'unique_wrapper_body_unchanged': True, 'wrapper_sha256': sha(after[0])}
    name = 'cmake_files.cmake'
    working = (ROOT / name).read_bytes()
    additions = b'\t"${SRC_DIR}/universal/com_hash.cpp"\n\t"${SRC_DIR}/universal/com_hash.h"\n'
    raw_additions = canonical(additions) if b'\r\n' in working else additions
    raw_restored = replace_once(working, raw_additions, b'', 'two exact inventory lines with their own endings')
    if sha(raw_restored).upper() != baseline['BlackOpsARM']['files'][name].upper():
        raise RuntimeError('Raw inventory reversal does not restore captured baseline bytes')
    old = original(name, raw_restored)
    default = replace_once(lf(working), additions, b'', 'explicit unique universal source inventory addition')
    if canonical(default) != old:
        raise RuntimeError('Source inventory changed beyond the single moved unit/header')
    result['files'][name] = {'current_sha256': sha(working), 'parent_sha256': sha(old),
                            'only_explicit_moved_unit_addition': True,
                            'raw_restored_sha256': sha(raw_restored), 'raw_baseline_exactly_restored': True}
    callers = {}
    definitions, declarations = [], []
    for path in sorted((ROOT / 'src').rglob('*')):
        if path.suffix not in ['.cpp', '.h', '.hpp'] or not path.is_file():
            continue
        data = canonical(path.read_bytes())
        relative = path.relative_to(ROOT).as_posix()
        definition_count = len(re.findall(rb'^int __cdecl Com_HashString\([^\r\n;{}]*\)\r\n\{', data, re.MULTILINE))
        declaration_count = len(re.findall(rb'^int __cdecl Com_HashString\([^\r\n;{}]*\);', data, re.MULTILINE))
        definitions.extend([relative] * definition_count)
        declarations.extend([relative] * declaration_count)
        calls = len(re.findall(rb'\bCom_HashString\s*\(', data)) - definition_count - declaration_count
        if calls:
            if relative not in baseline['BlackOpsARM']['files']:
                raise RuntimeError('Unexpected newly introduced hash caller: ' + relative)
            parent = original(relative)
            parent_definitions = len(re.findall(rb'^int __cdecl Com_HashString\([^\r\n;{}]*\)\r\n\{', parent, re.MULTILINE))
            parent_declarations = len(re.findall(rb'^int __cdecl Com_HashString\([^\r\n;{}]*\);', parent, re.MULTILINE))
            old_calls = len(re.findall(rb'\bCom_HashString\s*\(', parent)) - parent_definitions - parent_declarations
            if calls != old_calls:
                raise RuntimeError('Hash caller inventory changed: ' + relative)
            if relative not in ['src/server_mp/sv_bot_mp.cpp', 'src/universal/com_stringtable_obj.cpp'] and data != parent:
                raise RuntimeError('Hash consumer source changed unexpectedly: ' + relative)
            callers[relative] = calls
    if definitions != ['src/universal/com_hash.cpp'] or declarations != ['src/universal/com_hash.h']:
        raise RuntimeError('Production hash definition/declaration duplicated or forked')
    expected_callers = {'src/clientscript/cscr_yacc.cpp': 1, 'src/server_mp/sv_bot_mp.cpp': 1,
                        'src/universal/com_expressions.cpp': 1, 'src/universal/com_stringtable_obj.cpp': 1,
                        'src/universal/dvar.cpp': 2}
    if callers != expected_callers:
        raise RuntimeError('Expected original hash callers lost or added')
    result.update({'unique_definition': definitions[0], 'unique_declaration': declarations[0],
                   'original_callers_preserved': callers, 'status': 'passed'})
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('PASS unique production hash move/fixed32 contract, exact default wrapper/server source/inventory and original callers')


if __name__ == '__main__':
    main()
