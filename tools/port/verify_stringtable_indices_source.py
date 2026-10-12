"""Prove exact default loader preservation after bounded index selection.

Only specifically documented selector/include bytes are reversed. Whole-file
equality proves every default function/global and its original order, without
depending on parsing the decompiled loader's unrelated functions.
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


def crlf(data):
    return lf(data).replace(b'\n', b'\r\n')


def replace_once(data, before, after, label):
    if data.count(before) != 1:
        raise RuntimeError('Expected one exact documented selector: ' + label)
    return data.replace(before, after, 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    scratch = ROOT / 'build-android-audit'
    if not output.is_relative_to(scratch) or output.exists():
        raise RuntimeError('Use a fresh proof output within BlackOpsARM/build-android-audit')
    baseline = json.loads(args.baseline.read_text(encoding='utf-8'))
    checkpoint = baseline['BlackOpsARM']['head']
    result = {'schema': 'BlackOpsARM.TASK-0021.source-preservation.v1',
              'checkpoint': checkpoint, 'baseline_sha256': sha(args.baseline.read_bytes()),
              'verifier_sha256': sha(Path(__file__).read_bytes()),
              'normalization': 'LF/CRLF only; restored comparison uses canonical CRLF', 'files': {}}

    def original(name):
        data = subprocess.check_output(['git', '--no-optional-locks', 'show', checkpoint + ':' + name], cwd=ROOT)
        normalized = crlf(data)
        if sha(normalized).upper() != baseline['BlackOpsARM']['files'][name].upper():
            raise RuntimeError('Git source does not match captured baseline: ' + name)
        return normalized

    name = 'src/database/db_load.cpp'
    old = original(name)
    working = (ROOT / name).read_bytes()
    default = lf(working)
    default = replace_once(default,
        b'#if defined(BO1_STRINGTABLE_INDEX_GATE)\n#include "db_scalar_load.h"\n#include "db_stream_load.h"\n#else\n',
        b'', 'bounded includes')
    default = replace_once(default,
        b'#endif\n__int16 *varshort;\n#if !defined(BO1_STRINGTABLE_INDEX_GATE)\n',
        b'__int16 *varshort;\n', 'global at original definition position')
    default = replace_once(default, b'#endif\nvoid __cdecl Load_shortArray(',
        b'void __cdecl Load_shortArray(', 'leaf begins')
    default = replace_once(default,
        b'    Load_Stream(atStreamStart, (unsigned __int8 *)varshort, 2 * count);\n}\n#if !defined(BO1_STRINGTABLE_INDEX_GATE)\n',
        b'    Load_Stream(atStreamStart, (unsigned __int8 *)varshort, 2 * count);\n}\n', 'leaf ends')
    if not default.endswith(b'\n#endif // !BO1_STRINGTABLE_INDEX_GATE\n'):
        raise RuntimeError('Expected exact added final selector/newline')
    default = default[:-len(b'\n#endif // !BO1_STRINGTABLE_INDEX_GATE\n')]
    if crlf(default) != old:
        raise RuntimeError('Default db_load.cpp differs after reversing only documented selection')
    if len(re.findall(rb'^__int16 \*varshort;\r?$', working, re.MULTILINE)) != 1:
        raise RuntimeError('varshort definition is not unique')
    if len(re.findall(rb'^void __cdecl Load_shortArray\([^;]*\)\r?\n\{', working, re.MULTILINE)) != 1:
        raise RuntimeError('Load_shortArray definition is not unique')
    neighborhood = b'float (*varshared_vec3_t)[3];\n__int16 *varshort;\nunsigned __int16 *varushort;'
    if neighborhood not in default:
        raise RuntimeError('Default global definition order changed')
    pattern = rb'^void __cdecl Load_shortArray\(bool atStreamStart, int count\)\r?\n\{\r?\n[^}]*\}'
    old_leaf = re.search(pattern, old, re.MULTILINE)
    new_leaf = re.search(pattern, crlf(working), re.MULTILINE)
    if old_leaf is None or new_leaf is None:
        raise RuntimeError('Missing exact scalar loader leaf')
    before = functions(old_leaf.group())
    after = functions(new_leaf.group())
    if before != after or set(after) != {'Load_shortArray'}:
        raise RuntimeError('Production scalar loader body changed')
    result['files'][name] = {'current_sha256': sha(working), 'parent_crlf_sha256': sha(old),
                            'restored_default_crlf_sha256': sha(crlf(default)),
                            'whole_default_source_exact': True, 'unique_varshort_original_order': True,
                            'unique_Load_shortArray_body_exact': True,
                            'Load_shortArray_sha256': sha(after['Load_shortArray'])}
    name = 'src/database/db_load.h'
    old = original(name)
    working = (ROOT / name).read_bytes()
    prototype = b'void __cdecl Load_shortArray(bool atStreamStart, int count);'
    default = replace_once(lf(working), b'#include "db_scalar_load.h"', prototype, 'original scalar prototype')
    if crlf(default) != old:
        raise RuntimeError('Default db_load.h differs beyond one moved declaration include')
    result['files'][name] = {'current_sha256': sha(working), 'parent_crlf_sha256': sha(old),
                            'restored_default_crlf_sha256': sha(crlf(default)), 'whole_default_source_exact': True}
    light_path = ROOT / 'src/database/db_scalar_load.h'
    light = light_path.read_bytes()
    expected = b'#pragma once\n#include <platform/bo1_native_width.h>\nextern __int16 *varshort;\n' + prototype + b'\n'
    if lf(light) != expected:
        raise RuntimeError('Light scalar header has unexpected declarations/definitions')
    result['files']['src/database/db_scalar_load.h'] = {'current_sha256': sha(light),
                                                       'exact_unique_declarations_no_definition': True}
    for name in ['src/database/db_stream_load.cpp', 'src/database/db_expand.cpp']:
        old = original(name)
        working = (ROOT / name).read_bytes()
        if crlf(working) != old or sha(working).upper() != baseline['BlackOpsARM']['files'][name].upper():
            raise RuntimeError('Previously certified full source changed: ' + name)
        result['files'][name] = {'current_sha256': sha(working), 'parent_crlf_sha256': sha(old),
                                'full_source_byte_unchanged': True}
    result['status'] = 'passed'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('PASS whole default loader/global order and actual Load_shortArray preservation; exact light declarations; stream/expander unchanged')


if __name__ == '__main__':
    main()
