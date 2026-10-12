"""Independent exact source proof for TASK29 loader compile selection.

Undo only the named light includes and surrounding selectors, then compare the
entire db_load.cpp to its checkpoint. No native runtime/compiler claim follows.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys

sys.dont_write_bytecode = True
from verify_temp_cursor_source import function, lf, sha
from verify_string_pool_source import functions

ROOT = Path(__file__).resolve().parents[2]
CHECKPOINT = '7f7f005'


def once(data, old, new, label):
    if data.count(old) != 1:
        raise RuntimeError('Expected one reversible source edit: ' + label)
    return data.replace(old, new, 1)


def prove():
    parent = subprocess.check_output(
        ['git', '--no-optional-locks', 'rev-parse', '--verify', CHECKPOINT + '^{commit}'], cwd=ROOT).decode().strip()
    def original(path):
        return subprocess.check_output(['git', '--no-optional-locks', 'show', parent + ':' + path], cwd=ROOT)
    path = 'src/database/db_load.cpp'
    before, after = original(path), (ROOT / path).read_bytes()
    old, work = lf(before), lf(after)
    report = {'schema': 'BlackOpsARM.TASK-0029.rawfile-payload-source-preservation.v1',
              'parent': parent, 'verifier_sha256': sha(Path(__file__).read_bytes()),
              'helper_sha256': {p: sha((ROOT / 'tools/port' / p).read_bytes()) for p in
                               ('verify_temp_cursor_source.py', 'verify_string_pool_source.py')},
              'comparison': 'Whole source LF-normalized bytes, preserving BOM and all non-line-ending bytes',
              'files': {}, 'selected_loader_leaves': {}}
    include = (b'#if defined(BO1_RAWFILE_PAYLOAD_GATE)\n#include "db_rawfile_load.h"\n'
               b'#include "db_stream.h"\n#include "db_expand.hpp"\n#endif\n')
    work = once(work, include, b'', 'light declaration includes')
    opening = b'#endif\n#if !defined(BO1_STRINGTABLE_INDEX_GATE) || defined(BO1_RAWFILE_PAYLOAD_GATE)\n'
    closing = b'#endif\n#if !defined(BO1_STRINGTABLE_INDEX_GATE)\n'
    for global_line in (b'const char *varConstChar;\n', b'const char **varXString;\n', b'RawFile *varRawFile;\n'):
        work = once(work, opening + global_line + closing, global_line, global_line.decode().strip())
    for names in (('AllocLoad_raw_byte', 'Load_ConstCharArray'), ('Load_XString',), ('Load_RawFile',)):
        original_group = b'\n\n'.join(function(old, name) for name in names) + b'\n\n'
        selected_group = opening + original_group + closing
        work = once(work, selected_group, original_group, 'selected leaf group ' + '/'.join(names))
    if work != old:
        raise RuntimeError('db_load.cpp changed beyond exact declared selectors/light includes')
    before_functions, after_functions = functions(old), functions(lf(after))
    if list(before_functions) != list(after_functions) or before_functions != after_functions:
        raise RuntimeError('Loader function bodies/definition order changed')
    report['files'][path] = {
        'original_git_blob_sha256': sha(before), 'current_sha256': sha(after),
        'whole_source_restored_exact': True, 'bom_preserved': before.startswith(b'\xef\xbb\xbf') == after.startswith(b'\xef\xbb\xbf'),
        'bom_count_preserved': before.count(b'\xef\xbb\xbf') == after.count(b'\xef\xbb\xbf'),
        'function_count': len(before_functions), 'all_collected_function_bodies_order_unchanged': True,
        'only_documented_compile_selection_and_light_includes': True,
    }
    for name in ('AllocLoad_raw_byte', 'Load_ConstCharArray', 'Load_XString', 'Load_RawFile', 'Load_shortArray'):
        first, last = function(old, name), function(lf(after), name)
        if first != last:
            raise RuntimeError('Selected original body changed: ' + name)
        report['selected_loader_leaves'][name] = {'unique_body_byte_unchanged': True, 'lf_sha256': sha(last)}
    for global_line in (b'__int16 *varshort;', b'const char *varConstChar;', b'const char **varXString;', b'RawFile *varRawFile;'):
        pattern = rb'(?m)^' + re.escape(global_line) + rb'$'
        if len(re.findall(pattern, old)) != 1 or len(re.findall(pattern, lf(after))) != 1:
            raise RuntimeError('Original scalar/RawFile global definition is not unique')
    report['global_storage'] = {'varshort': 'one unchanged original definition',
                                'varConstChar': 'one unchanged original definition',
                                'varXString': 'one unchanged original definition',
                                'varRawFile': 'one unchanged original definition',
                                'default_definition_order_unchanged': True}
    for immutable in (
        'src/database/db_load.h', 'src/database/db_scalar_load.h',
        'src/database/db_stream.cpp', 'src/database/db_stream.h',
        'src/database/db_stream_load.cpp', 'src/database/db_stream_load.h',
        'src/database/db_expand.cpp', 'src/database/db_expand.hpp',
        'src/database/db_rawfile_types.h', 'src/database/db_registry.h'):
        first, last = original(immutable), (ROOT / immutable).read_bytes()
        if lf(first) != lf(last):
            raise RuntimeError('Preceding production source/declarations changed: ' + immutable)
        report['files'][immutable] = {'whole_source_lf_bytes_unchanged': True,
                                    'original_git_blob_sha256': sha(first), 'current_sha256': sha(last)}
    light = 'src/database/db_rawfile_load.h'
    expected = (b'#pragma once\n#include "db_rawfile_types.h"\n'
                b'extern RawFile*varRawFile;\nextern const char**varXString;\nextern const char*varConstChar;\n'
                b'unsigned char* __cdecl AllocLoad_raw_byte();\nvoid __cdecl Load_XString(bool atStreamStart);\n'
                b'void __cdecl Load_ConstCharArray(bool atStreamStart,int count);\nvoid __cdecl Load_RawFile(bool atStreamStart);\n')
    if lf((ROOT / light).read_bytes()) != expected:
        raise RuntimeError('Light declarations differ from reviewed existing signatures/extern storage')
    report['files'][light] = {'only_compatible_declarations_no_storage': True,
                            'current_sha256': sha((ROOT / light).read_bytes())}
    report.update(status='passed', compiler_preprocessing_certified=False,
                  native_loader_execution_certified=False, full_registry_executed=False,
                  shortarray_declaration_and_body_preserved=True,
                  scope='Source preservation only; checked plan and fixture semantics have independent review/runtime evidence')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    allowed = (ROOT / 'build-android-audit/rawfile_payload_review_run1').resolve()
    if not output.is_relative_to(allowed) or output.exists():
        raise RuntimeError('Use a fresh source-proof file within rawfile_payload_review_run1')
    try:
        result = prove()
    except Exception as error:
        result = {'schema': 'BlackOpsARM.TASK-0029.rawfile-payload-source-preservation.v1',
                  'checkpoint': CHECKPOINT, 'status': 'failed', 'error': str(error),
                  'verifier_sha256': sha(Path(__file__).read_bytes())}
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        raise
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('PASS exact db_load selector reversal, unchanged five selected leaves/storage/order and preceding stream/expand/type source')


if __name__ == '__main__':
    main()
