"""Extract canonical historical T5 thread-data declarations for independent probes.

Generated C++ is validation-only in an ignored worktree build directory. No
alternate declarations are installed in production and no game assets are read.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
BASELINE = 'f117f5d'


def declaration(source, name, kind='struct'):
    match = re.search(r'\b' + kind + r'\s+' + re.escape(name) + r'\b[^{}]*\{', source)
    if not match:
        raise RuntimeError('Missing canonical declaration: ' + name)
    end = source.find('\n};', match.end())
    if end < 0:
        raise RuntimeError('Missing declaration terminator')
    return source[match.start():end + 3]


def original(path):
    return subprocess.check_output(['git', 'show', BASELINE + ':' + path], cwd=ROOT).decode('utf-8-sig').replace('\r\n', '\n')


EXPECTED = {
    4: {'cmd_args_bytes': 10476, 'item_offset': 68, 'argv_offset': 164,
        'text_offset': 196, 'argv_pool_offset': 8388, 'used_text_offset': 10436,
        'total_argv_offset': 10468, 'total_text_offset': 10472,
        'trace_count_bytes': 8, 'trace_info_bytes': 20, 'partition_offset': 4,
        'brush_offset': 8, 'model_offset': 12, 'geoms_offset': 16},
    8: {'cmd_args_bytes': 12592, 'item_offset': 72, 'argv_offset': 200,
        'text_offset': 264, 'argv_pool_offset': 8456, 'used_text_offset': 12552,
        'total_argv_offset': 12584, 'total_text_offset': 12588,
        'trace_count_bytes': 16, 'trace_info_bytes': 40, 'partition_offset': 8,
        'brush_offset': 16, 'model_offset': 24, 'geoms_offset': 32},
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-directory', required=True)
    args = parser.parse_args()
    output = (ROOT / args.output_directory).resolve()
    if not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use fresh worktree oracle directory')
    output.mkdir(parents=True)
    groups = [('src/qcommon/cmd.h', 'CmdArgs', 'struct'),
              ('src/qcommon/cm_trace.h', 'TraceCheckCount', 'struct'),
              ('src/qcommon/cm_trace.h', 'TraceThreadInfo', 'struct'),
              ('src/universal/q_shared.cpp', 'va_info_t', 'struct'),
              ('src/qcommon/threads.h', 'ThreadContext_t', 'enum')]
    declarations = []
    provenance = []
    for path, name, kind in groups:
        text = declaration(original(path), name, kind)
        provenance.append({'path': path, 'name': name, 'kind': kind,
                           'declaration_sha256': hashlib.sha256(text.encode()).hexdigest()})
        if name == 'ThreadContext_t':
            # Original MSVC accepts this all-ones literal as signed32 -1.
            # Clang diagnoses narrowing; preserve the original Windows control
            # and use its explicit signed value only for the Clang oracle.
            old = '    THREAD_CONTEXT_INVALID = 0xFFFFFFFF,'
            if text.count(old) != 1:
                raise RuntimeError('Original invalid-role literal differs')
            text = text.replace(old, '#if defined(__clang__)\n    THREAD_CONTEXT_INVALID = -1,\n#else\n' + old + '\n#endif')
        declarations.append(text)
    assertions = '''
static_assert(sizeof(va_info_t) == 4100 && offsetof(va_info_t,index) == 4096);
static_assert(sizeof(ThreadContext_t) == 4);
static_assert(THREAD_CONTEXT_MAIN == 0 && THREAD_CONTEXT_BACKEND == 1 && THREAD_CONTEXT_DATABASE == 13 && THREAD_CONTEXT_COUNT == 15);
static_assert(THREAD_CONTEXT_INVALID == -1);
'''
    fields = {'cmd_args_bytes': 'sizeof(CmdArgs)', 'item_offset': 'offsetof(CmdArgs,itemDef)',
              'argv_offset': 'offsetof(CmdArgs,argv)', 'text_offset': 'offsetof(CmdArgs,textPool)',
              'argv_pool_offset': 'offsetof(CmdArgs,argvPool)', 'used_text_offset': 'offsetof(CmdArgs,usedTextPool)',
              'total_argv_offset': 'offsetof(CmdArgs,totalUsedArgvPool)', 'total_text_offset': 'offsetof(CmdArgs,totalUsedTextPool)',
              'trace_count_bytes': 'sizeof(TraceCheckCount)', 'trace_info_bytes': 'sizeof(TraceThreadInfo)',
              'partition_offset': 'offsetof(TraceCheckCount,partitions)', 'brush_offset': 'offsetof(TraceThreadInfo,box_brush)',
              'model_offset': 'offsetof(TraceThreadInfo,box_model)', 'geoms_offset': 'offsetof(TraceThreadInfo,geoms)'}
    for width, values in EXPECTED.items():
        for key, expected in values.items():
            assertions += f'static_assert(sizeof(void*) != {width} || {fields[key]} == {expected});\n'
    format_parts = ['"pointer_bytes":%zu'] + ['"' + key + '":%zu' for key in fields]
    fmt = ('{' + ','.join(format_parts) + '}\n').replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
    arguments = ','.join(['sizeof(void*)'] + list(fields.values()))
    source = '#include <cstddef>\n#include <cstdio>\n' + '\n\n'.join(declarations) + assertions + f'\nint main() {{ std::printf("{fmt}",{arguments}); }}\n'
    (output / 'thread_data_original.cpp').write_text(source, encoding='utf-8', newline='\n')
    (output / 'oracle.json').write_text(json.dumps({'baseline': BASELINE, 'canonical_declarations': provenance,
            'expected': EXPECTED, 'reviewed_clang_enum_delta': 'signed32 invalid allones literal explicitly -1; original MSVC literal retained and statically verifies -1',
            'generated_cpp_sha256': hashlib.sha256(source.encode()).hexdigest()}, indent=2) + '\n', encoding='utf-8')
    print('Generated historical declaration oracle; static expectations independent of port headers')


if __name__ == '__main__':
    main()
