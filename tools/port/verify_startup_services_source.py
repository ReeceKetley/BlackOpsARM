"""Verify selected startup functions retain original normal Windows bodies."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import verify_bootstrap_default_source as extraction

ROOT = Path(__file__).resolve().parents[2]
BASELINE = '8f81f9c'


def windows_body(source):
    lines, stack, active = [], [], True
    for line in source.splitlines(keepends=True):
        directive = line.strip()
        if directive.startswith(('#if ', '#ifdef ', '#ifndef ')):
            native = directive in ('#if defined(__ANDROID__)',
                '#if defined(__ANDROID__) || defined(BO1_STARTUP_SERVICES_NATIVE_HOST_TEST)')
            stack.append((native, False if native else True, active))
            if not native and active:
                lines.append(line)
            if native:
                active = False
        elif directive == '#else':
            native, branch, parent = stack[-1]
            if native:
                stack[-1] = (native, not branch, parent)
                active = parent and not branch
            elif active:
                lines.append(line)
        elif directive.startswith('#endif'):
            native, branch, parent = stack.pop()
            if not native and active:
                lines.append(line)
            active = parent
        elif directive.startswith('#elif'):
            if stack[-1][0]:
                raise RuntimeError('Unreviewed native elif')
            if active:
                lines.append(line)
        elif active:
            lines.append(line)
    if stack:
        raise RuntimeError('Unbalanced function condition')
    return ''.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use fresh worktree proof output')
    groups = {
        'src/universal/dvar.cpp': ['Dvar_Init', 'Dvar_Shutdown', 'Dvar_UpdateValue',
            'Dvar_RegisterNew', 'Dvar_RegisterVariant', 'Dvar_CopyString', 'Dvar_WeakCopyString',
            'Dvar_ShouldFreeCurrentString', 'Dvar_ShouldFreeLatchedString',
            'Dvar_ShouldFreeResetString', 'Dvar_FreeString', 'Dvar_SetVariant', 'Dvar_ValueToString'],
        'src/qcommon/cmd.cpp': ['Cbuf_Init', 'Cmd_Init', 'Cmd_Shutdown', 'Cmd_AddCommandInternal',
            'Cmd_ExecuteSingleCommand', 'Cmd_ResetArgs'],
        'src/qcommon/dvar_cmds.cpp': ['Dvar_AddCommands', 'Com_DvarDumpSingle'],
        'src/qcommon/common.cpp': ['Com_LogFileOpen'],
        'src/universal/q_parse.cpp': ['Com_InitParse', 'Com_InitParseInfo',
            'Com_ScriptError', 'Com_ScriptErrorDrop', 'Com_ScriptWarning'],
        'src/universal/q_shared.cpp': ['CanKeepStringPointer', 'Com_sprintf', 'Com_sprintfPos', 'va'],
        'src/universal/com_memory.cpp': ['CopyString', 'FreeString'],
        'src/universal/com_shared.cpp': ['Com_Filter', 'Com_Memset', '_copyDWord'],
        'src/universal/com_math.cpp': ['Vec2Copy', 'Vec3Copy', 'Vec4Copy', 'Vec4Set'],
        'src/client/cl_keys.cpp': ['Key_IsValidGamePadChar'],
        'src/clientscript/cscr_debugger.cpp': ['Scr_MonitorCommand']}
    proofs = {}
    for path, names in groups.items():
        before_bytes = subprocess.check_output(['git', 'show', BASELINE + ':' + path], cwd=ROOT)
        after_bytes = (ROOT / path).read_bytes()
        # Lossless byte view also preserves legacy-encoded comments. Function
        # tokens selected here are ASCII; no transcoding is performed.
        before = before_bytes.decode('latin1').replace('\r\n', '\n')
        after = after_bytes.decode('latin1').replace('\r\n', '\n')
        for name in names:
            old = extraction.body(before, name)
            current = windows_body(extraction.body(after, name))
            if old != current:
                raise RuntimeError('Original normal Windows body changed: ' + name)
            proofs[name] = dict(path=path, original_body_sha256=hashlib.sha256(old.encode('latin1')).hexdigest())
        if sorted(line for line in before.splitlines() if not line.isascii()) != sorted(line for line in after.splitlines() if not line.isascii()):
            raise RuntimeError('Legacy source comment bytes changed: ' + path)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(dict(passed=True, baseline=BASELINE, original_windows_bodies=proofs), indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
