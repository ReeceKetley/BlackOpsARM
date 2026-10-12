"""Prove canonical thread declarations and actual initializer/reset body moves."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
from thread_data_layout_oracle import BASELINE, declaration, original
import verify_bootstrap_default_source as extraction

ROOT = Path(__file__).resolve().parents[2]


def tokens(text):
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
    return re.sub(r'\s+', '', text)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use fresh worktree source proof')
    types = [('src/qcommon/cmd.h', 'src/qcommon/cmd_args.h', 'CmdArgs', 'struct'),
             ('src/qcommon/cm_trace.h', 'src/qcommon/trace_thread_info.h', 'TraceCheckCount', 'struct'),
             ('src/qcommon/cm_trace.h', 'src/qcommon/trace_thread_info.h', 'TraceThreadInfo', 'struct'),
             ('src/universal/q_shared.cpp', 'src/universal/thread_data.h', 'va_info_t', 'struct'),
             ('src/qcommon/threads.h', 'src/qcommon/thread_context.h', 'ThreadContext_t', 'enum')]
    proofs = {}
    for old_path, new_path, name, kind in types:
        before = declaration(original(old_path), name, kind)
        after = declaration((ROOT / new_path).read_text(), name, kind)
        expected = before.replace('THREAD_CONTEXT_INVALID = 0xFFFFFFFF', 'THREAD_CONTEXT_INVALID = -1') if name == 'ThreadContext_t' else before
        if tokens(after) != tokens(expected):
            raise RuntimeError('Canonical declaration differs: ' + name)
        if re.search(r'\b' + kind + r'\s+' + name + r'\b[^{}]*\{', (ROOT / old_path).read_text()):
            raise RuntimeError('Duplicate original declaration: ' + name)
        proofs[name] = {'canonical_source': old_path, 'new_header': new_path,
                        'before_sha256': hashlib.sha256(before.encode()).hexdigest(),
                        'reviewed_signed32_invalid_literal': name == 'ThreadContext_t'}
    bodies = {}
    for path, name in [('src/universal/q_shared.cpp', 'Com_InitThreadData'), ('src/qcommon/cmd.cpp', 'Cmd_ResetArgs')]:
        before = extraction.body(original(path), name)
        after = extraction.body((ROOT / path).read_text(), name)
        if before != after:
            raise RuntimeError('Original production body changed: ' + name)
        bodies[name] = {'path': path, 'body_sha256': hashlib.sha256(before.encode()).hexdigest(), 'unchanged': True}
    for path, line in [('src/qcommon/cmd.cpp', 'CmdArgs g_cmd_args[2];'),
                       ('src/universal/q_shared.cpp', 'va_info_t va_info[15];'),
                       ('src/universal/q_shared.cpp', 'TraceThreadInfo g_traceThreadInfo[15];'),
                       ('src/universal/q_shared.cpp', 'jmp_buf g_com_error[15];')]:
        if original(path).count(line) != 1 or (ROOT / path).read_text().count(line) != 1:
            raise RuntimeError('Canonical storage definition changed/duplicated')
    # Pool/lock identities and serialized layouts cannot drift during this gate.
    immutable = ['src/platform/android/script_pool_bridge.cpp', 'src/platform/android/script_string_services.cpp',
                 'src/platform/android/hunk_services.cpp', 'src/database/db_zone_types.h',
                 'src/database/db_asset_entry_types.h', 'src/database/db_assetlist_types.h',
                 'src/platform/bo1_native_width.h']
    for path in immutable:
        if (ROOT / path).read_text(encoding='utf-8-sig') != original(path):
            raise RuntimeError('Protected engine layout/service changed: ' + path)
    result = {'passed': True, 'baseline': BASELINE, 'canonical_declarations': proofs,
              'actual_production_bodies': bodies, 'storage_definitions_unique': True,
              'unchanged_layout_services': immutable,
              'scope': 'Declaration/body/storage proof; full normal Windows builds and native runtime are independently required.'}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('Canonical thread types, original initializer/reset bodies and storage proof pass')


if __name__ == '__main__':
    main()
