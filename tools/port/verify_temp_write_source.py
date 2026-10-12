"""Independent TASK-0028 source preservation proof; no runtime certificate.

Pins the preceding checkpoint, preserves Win32 leaves, and distinguishes the
changed native scratch reader from the otherwise unchanged expander functions.
Only BO1_NATIVE_64BIT is selected false; unrelated guards remain in the proof.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys

sys.dont_write_bytecode = True
from verify_temp_cursor_source import function, lf, sha, win32_source
from verify_string_pool_source import functions

ROOT = Path(__file__).resolve().parents[2]
PARENT = '3f7dc567e7b2d82977d38cfa820bd5591ea6cace'


def original(path):
    return lf(subprocess.check_output(['git', '--no-optional-locks', 'show', PARENT + ':' + path], cwd=ROOT))


def once(data, old, new, label):
    if data.count(old) != 1:
        raise RuntimeError('Expected one ' + label)
    return data.replace(old, new, 1)


def current(path):
    return lf((ROOT / path).read_bytes())


def verify():
    resolved = subprocess.check_output(
        ['git', '--no-optional-locks', 'rev-parse', '--verify', PARENT + '^{commit}'], cwd=ROOT).decode().strip()
    if resolved != PARENT:
        raise RuntimeError('Parent identity differs')
    report = {'schema': 'BlackOpsARM.TASK-0028.temp-write-source-preservation.v1',
              'parent': PARENT, 'verifier_sha256': sha(Path(__file__).read_bytes()),
              'helper_sha256': {p: sha((ROOT / 'tools/port' / p).read_bytes()) for p in
                               ('verify_temp_cursor_source.py', 'verify_string_pool_source.py')},
              'comparison': 'LF-normalized bytes; native guards false only for Win32 comparisons',
              'win32_functions': {}, 'files': {}}
    for path, names in (
        ('src/database/db_stream.cpp', ('DB_InitStreams', 'DB_PushStreamPos', 'DB_SetStreamIndex',
                                       'DB_PopStreamPos', 'DB_AllocStreamPos', 'DB_IncStreamPos')),
        ('src/database/db_stream_load.cpp', ('Load_Stream', 'Load_XStringCustom',
                                            'DB_ConvertOffsetToAlias', 'DB_ConvertOffsetToPointer'))):
        old32, new32 = win32_source(original(path)), win32_source(current(path))
        for name in names:
            before, after = function(old32, name), function(new32, name)
            if before != after:
                raise RuntimeError('Win32 function changed: ' + name)
            report['win32_functions'][name] = {'body_unchanged': True, 'lf_sha256': sha(after)}
        # The full stream-load TU has only native-guarded changes.
        if path.endswith('db_stream_load.cpp') and old32 != new32:
            raise RuntimeError('Win32 stream-load declarations/order changed')
        report['files'][path] = {'current_sha256': sha((ROOT / path).read_bytes()),
                                'win32_selected_functions_unchanged': len(names)}
    header = 'src/database/db_stream.h'
    if win32_source(original(header)) != win32_source(current(header)):
        raise RuntimeError('Win32 stream declarations changed')
    report['files'][header] = {'win32_native_guards_removed_bytes_unchanged': True,
                              'current_sha256': sha((ROOT / header).read_bytes())}

    path = 'src/database/db_expand.cpp'
    old, new = original(path), current(path)
    before, after = functions(old), functions(new)
    pattern = rb'^(?:[\w:*]+\s+)+(?:__cdecl\s+)?([A-Za-z_]\w*)\([^;]*?\)\n\{'
    for data in (old, new):
        names = [m.group(1) for m in re.finditer(pattern, data, re.M)]
        if len(names) != 163 or len(set(names)) != 163:
            raise RuntimeError('Expander function count or uniqueness changed')
    if list(before) != list(after):
        raise RuntimeError('Expander function order changed')
    cancel = b'    DB_StreamCancelWrite(); // External envelope stores must not arm a TEMP commit.\n'
    expected_envelope = once(before['DB_ExpandLoad_XAssetList'],
                             b'    g_expandReadAccum = 0; // side buffer, not an XBlock stream commit\n',
                             b'    g_expandReadAccum = 0; // side buffer, not an XBlock stream commit\n' + cancel,
                             'envelope accumulator reset')
    for name, body in before.items():
        if name == 'DB_Expand_ReadFile':
            continue
        expected = expected_envelope if name == 'DB_ExpandLoad_XAssetList' else body
        if after[name] != expected:
            raise RuntimeError('Unexpected expander body change: ' + name)
    restored = once(new, after['DB_Expand_ReadFile'], before['DB_Expand_ReadFile'], 'native scratch reader')
    restored = once(restored, cancel, b'', 'sole envelope cancellation')
    restored = once(restored,
        b'#if defined(BO1_RAWFILE_HEADER_GATE)\n#include "db_rawfile_types.h"\n#endif\n', b'', 'light RawFile include')
    restored = once(restored, b'#include <stdexcept>\n', b'', 'native exception include')
    disk = re.search(rb'(?m)^struct RawFile_32\n\{.*?^\};', old, re.S).group()
    if new.count(disk) != 1:
        raise RuntimeError('Packed RawFile definition not uniquely moved')
    restored = once(restored, disk + b'\n', b'', 'moved packed definition')
    anchor = b'struct LocalizeEntry_32\n{\n    uint32_t value;\n    uint32_t name;\n};\n\n#pragma pack(pop)'
    restored = once(restored, anchor, anchor.replace(b'\n\n#pragma', b'\n' + disk + b'\n#pragma'), 'original packed location')
    assertion = b'static_assert(sizeof(RawFile_32) == 12, "Win32 RawFile");\n'
    restored = once(restored, assertion, b'', 'moved packed assertion')
    anchor = b'static_assert(sizeof(LocalizeEntry_32) == 8, "Win32 LocalizeEntry");\n\n'
    restored = once(restored, anchor, anchor[:-1] + assertion, 'original assertion location')
    restored = once(restored,
        b'#endif // normal engine preceding RawFile body\n#if !defined(BO1_ASSETLIST_GATE) || defined(BO1_RAWFILE_HEADER_GATE)\n',
        b'', 'bounded RawFile opening selector')
    restored = once(restored,
        b'#endif // normal engine or bounded RawFile numeric header\n#if !defined(BO1_ASSETLIST_GATE)\n\n',
        b'', 'bounded RawFile closing selector')
    if restored != old:
        raise RuntimeError('Expander changed beyond explicit reader/cancellation/moves/selectors')
    report['files'][path] = {
        'current_sha256': sha((ROOT / path).read_bytes()), 'functions_total': 163,
        'functions_byte_unchanged': 161, 'function_order_unchanged': True,
        'intentional_body_changes': {
            'DB_Expand_ReadFile': {'before_lf_sha256': sha(before['DB_Expand_ReadFile']),
                                  'after_lf_sha256': sha(after['DB_Expand_ReadFile']),
                                  'scope': 'Native scratch-reader staging, explicit interleaved-read rejection and exception cleanup'},
            'DB_ExpandLoad_XAssetList': {'only_exact_cancel_line_added': True}},
        'all_other_source_restored_exact': True, 'rawfile_body_byte_unchanged': True,
        'rawfile_packed_definition_uniquely_moved': True}

    registry = 'src/database/db_registry.h'
    old_registry = original(registry)
    raw = re.search(rb'(?m)^struct RawFile // sizeof=0xC\n\{.*?^\};', old_registry, re.S).group()
    expected = once(old_registry, raw, b'#include "db_rawfile_types.h"', 'original RawFile type')
    if current(registry) != expected:
        raise RuntimeError('Registry declarations changed beyond canonical RawFile include')
    raw_path = 'src/database/db_rawfile_types.h'
    expected_raw = (b'#pragma once\n#include <cstddef>\n\n' + raw + b'\n\n'
        b'static_assert(sizeof(int) == 4, "RawFile length retains four bytes");\n'
        b'static_assert(offsetof(RawFile, name) == 0);\n'
        b'static_assert(offsetof(RawFile, len) == sizeof(void *));\n'
        b'static_assert(offsetof(RawFile, buffer) == (sizeof(void *) == 8 ? 16 : 8));\n'
        b'static_assert(sizeof(RawFile) == (sizeof(void *) == 8 ? 24 : 12));\n')
    if current(raw_path) != expected_raw:
        raise RuntimeError('Canonical RawFile definition/assertions differ')
    report['files'][registry] = {'only_canonical_rawfile_include_move': True}
    report['files'][raw_path] = {'canonical_definition_byte_unchanged': True,
                               'exact_native_width_offset_assertions': True,
                               'current_sha256': sha((ROOT / raw_path).read_bytes())}
    plan_files = {'src/platform/fastfile_rawfile_header.cpp', 'src/platform/fastfile_rawfile_header.h'}
    allowed = {'src/database/db_stream.cpp', 'src/database/db_stream.h',
               'src/database/db_stream_load.cpp', 'src/database/db_expand.cpp', registry}
    changed = subprocess.check_output(['git', '--no-optional-locks', 'diff', '--name-only', PARENT, '--', 'src'], cwd=ROOT).decode().splitlines()
    if set(changed) - allowed - plan_files:
        raise RuntimeError('Other tracked engine source changed: ' + repr(set(changed) - allowed - plan_files))
    untracked = subprocess.check_output(
        ['git', '--no-optional-locks', 'ls-files', '--others', '--exclude-standard', '--', 'src'],
        cwd=ROOT).decode().splitlines()
    if set(untracked) - {raw_path} - plan_files:
        raise RuntimeError('Other untracked engine source present: ' + repr(set(untracked) - {raw_path} - plan_files))
    for path in sorted(plan_files):
        if (ROOT / path).is_file():
            report['files'][path] = {'new_checked_plan_source': True,
                                    'semantic_behavior_certified': False,
                                    'current_sha256': sha((ROOT / path).read_bytes())}
    report.update(status='passed', tracked_engine_change_paths=sorted(changed),
                  untracked_engine_change_paths=sorted(untracked),
                  new_engine_file_scope=sorted({raw_path} | plan_files), native_behavior_certified=False,
                  compiler_preprocessing_certified=False, registry_execution_certified=False)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / 'build-android-audit').resolve()) or output.exists():
        raise RuntimeError('Use fresh ignored Android source-proof output')
    try:
        report = verify()
    except Exception as error:
        report = {'schema': 'BlackOpsARM.TASK-0028.temp-write-source-preservation.v1',
                  'parent': PARENT, 'status': 'failed', 'error': str(error),
                  'verifier_sha256': sha(Path(__file__).read_bytes())}
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
        raise
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('PASS ten Win32 bodies/header; 161/163 native expander functions unchanged; two explicit exceptions and canonical RawFile move')


if __name__ == '__main__':
    main()
