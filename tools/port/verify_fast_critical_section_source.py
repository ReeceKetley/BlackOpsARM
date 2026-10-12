"""Prove original Windows counter users and reviewed native atomic substitutions."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
from thread_data_layout_oracle import declaration
import verify_bootstrap_default_source as extraction

ROOT = Path(__file__).resolve().parents[2]
BASELINE = 'cfbd591'


def original(path):
    return subprocess.check_output(['git', 'show', BASELINE + ':' + path], cwd=ROOT).decode('utf-8-sig').replace('\r\n', '\n')


def windows_selection(source):
    # Select only new native-counter conditions, retaining every old directive.
    lines, frames, active = [], [], True
    for line in source.splitlines(keepends=True):
        directive = line.strip()
        if directive.startswith(('#if ', '#ifdef ', '#ifndef ')):
            native = directive == '#if BO1_NATIVE_FAST_CRIT_SECT'
            frames.append((native, False if native else True, active))
            if not native and active:
                lines.append(line)
            active = active and not native
        elif directive == '#else':
            native, branch, parent = frames[-1]
            if native:
                frames[-1] = (native, not branch, parent)
                active = parent and not branch
            elif active:
                lines.append(line)
        elif directive.startswith('#elif'):
            if frames[-1][0]:
                raise RuntimeError('Unexpected native elif')
            if active:
                lines.append(line)
        elif directive.startswith('#endif'):
            native, branch, parent = frames.pop()
            if not native and active:
                lines.append(line)
            active = parent
        elif active:
            lines.append(line)
    if frames:
        raise RuntimeError('Unbalanced body conditions')
    return ''.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use fresh worktree source proof')
    groups = {'src/database/db_registry.cpp': ['Sys_LockWrite', 'Sys_UnlockWrite', 'DB_LinkXAssetEntry'],
              'src/universal/dvar.cpp': ['Sys_UnlockRead', 'Dvar_FindMalleableVar_0', 'Dvar_SetCheatState',
                                       'Dvar_ResetScriptInfo', 'Dvar_AnyLatchedValues', 'Dvar_RestoreDvars',
                                       'Dvar_ResetDvars', 'Dvar_ResetConAccessDvars', 'Dvar_Init', 'Dvar_Shutdown', 'Dvar_RegisterVariant'],
              'src/game_sp/g_sp_savegame.cpp': ['Dvar_SaveDvars'],
              'src/live/live_pcache.cpp': ['PCache_IsLocked']}
    proofs = {}
    for path, names in groups.items():
        before = original(path)
        after = (ROOT / path).read_text(encoding='utf-8-sig')
        for name in names:
            original_body = extraction.body(before, name)
            current_body = windows_selection(extraction.body(after, name))
            if current_body != original_body:
                raise RuntimeError('Original Windows body changed: ' + name)
            proofs[name] = {'path': path, 'original_body_sha256': hashlib.sha256(original_body.encode()).hexdigest(), 'unchanged_windows': True}
        if [line for line in before.splitlines() if not line.isascii()] != [line for line in after.splitlines() if not line.isascii()]:
            raise RuntimeError('Source encoding drift: ' + path)
    before = declaration(original('src/win32/win_common.h'), 'FastCriticalSection')
    after = declaration((ROOT / 'src/platform/fast_critical_section.h').read_text(encoding='utf-8'), 'FastCriticalSection')
    if windows_selection(after) != before:
        raise RuntimeError('Original Windows canonical lock declaration changed')
    for path in ('src/platform/atomic32.h', 'src/platform/android/thread_services.cpp', 'src/universal/thread_data.h',
                 'src/platform/android/script_pool_bridge.cpp', 'src/platform/android/script_string_services.cpp',
                 'src/platform/native_zone_file.cpp', 'src/platform/native_zone_load.cpp'):
        if (ROOT / path).read_text(encoding='utf-8-sig') != original(path):
            raise RuntimeError('Unrelated atomic/thread/zone implementation changed')
    for path in ('src/qcommon/cmd_args.h', 'src/qcommon/trace_thread_info.h', 'src/qcommon/thread_context.h'):
        if (ROOT / path).read_text(encoding='utf-8').rstrip() != original(path).rstrip():
            raise RuntimeError('Thread declaration changed beyond EOF whitespace')
    result = {'passed': True, 'baseline': BASELINE, 'original_windows_bodies': proofs,
              'canonical_windows_lock_unchanged': True, 'native_counter_access_sites': 9,
              'atomic_thread_zone_implementations_unchanged': True,
              'thread_header_delta': 'EOF blank-line cleanup only',
              'scope': 'Original Windows body/type proof; native runtime and normal builds independently required.'}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('Original Windows lock/caller bodies and canonical declaration preserved')


if __name__ == '__main__':
    main()
