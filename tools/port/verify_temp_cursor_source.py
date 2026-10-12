"""Prove Win32 stream leaves and preceding loaders survive TASK-0027.

This is a source proof, not a compiler or runtime certificate. It resolves only
BO1_NATIVE_64BIT conditionals to false; other preprocessor branches are retained
byte for byte. LF normalization accounts for Git checkout line endings.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
PARENT = '9a8534d56165bb86e8ac9fee4c7f499f9d68b259'
STREAM = 'src/database/db_stream.cpp'
LEAVES = ('DB_InitStreams', 'DB_PushStreamPos', 'DB_SetStreamIndex',
          'DB_PopStreamPos', 'DB_AllocStreamPos', 'DB_IncStreamPos')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def lf(data):
    return data.replace(b'\r\n', b'\n')


def win32_source(data):
    """Select only native-width guards; fail closed on unfamiliar expressions."""
    result, stack = [], []
    active = True
    for line in data.splitlines(keepends=True):
        directive = re.match(rb'^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)', line)
        if not directive:
            if active:
                result.append(line)
            continue
        command, expression = directive.groups()
        expression = re.sub(rb'//.*|/\*.*?\*/', b'', expression).strip()
        if command in (b'if', b'ifdef', b'ifndef'):
            native = b'BO1_NATIVE_64BIT' in expression
            selected = True
            if native:
                if command != b'if' or expression not in (b'BO1_NATIVE_64BIT', b'!BO1_NATIVE_64BIT'):
                    raise RuntimeError('Unrecognized native guard: ' + line.decode(errors='replace'))
                selected = expression == b'!BO1_NATIVE_64BIT'
            stack.append({'native': native, 'outer': active, 'selected': selected, 'else': False})
            if native:
                active = active and selected
            elif active:
                result.append(line)
        elif command in (b'else', b'elif'):
            if not stack:
                raise RuntimeError('Unbalanced conditional branch')
            top = stack[-1]
            if top['native']:
                if command == b'elif' or top['else']:
                    raise RuntimeError('Unsupported native conditional branch')
                top['else'] = True
                active = top['outer'] and not top['selected']
            elif active:
                if b'BO1_NATIVE_64BIT' in expression:
                    raise RuntimeError('Native elif in an unrelated conditional')
                result.append(line)
        else:
            if not stack:
                raise RuntimeError('Unbalanced conditional end')
            top = stack.pop()
            if not top['native'] and active:
                result.append(line)
            active = top['outer']
    if stack:
        raise RuntimeError('Unclosed conditional')
    return b''.join(result)


def function(data, name):
    pattern = rb'(?m)^[^\n;{}]*\b' + re.escape(name.encode()) + rb'\([^;{}\n]*\)\n\{'
    matches = list(re.finditer(pattern, data))
    if len(matches) != 1:
        raise RuntimeError('Expected one body for ' + name)
    match = matches[0]
    # Ignore braces in ordinary comments/string/character literals. These source
    # leaves contain no raw C++ strings; reject one rather than silently misparse.
    tail = data[match.end() - 1:]
    if b'R"' in tail.split(b'\n}\n', 1)[0]:
        raise RuntimeError('Raw string outside this bounded source parser')
    tokens = re.compile(rb'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', re.S)
    depth = 0
    for token in tokens.finditer(tail):
        value = token.group()
        if value == b'{':
            depth += 1
        elif value == b'}':
            depth -= 1
            if depth == 0:
                return data[match.start():match.end() - 1 + token.end()]
    raise RuntimeError('Unclosed function ' + name)


def original(path):
    return subprocess.check_output(['git', '--no-optional-locks', 'show', PARENT + ':' + path], cwd=ROOT)


def verify():
    resolved = subprocess.check_output(
        ['git', '--no-optional-locks', 'rev-parse', '--verify', PARENT + '^{commit}'], cwd=ROOT).decode().strip()
    if resolved != PARENT:
        raise RuntimeError('Parent identity differs')
    before, after = original(STREAM), (ROOT / STREAM).read_bytes()
    old32, new32 = win32_source(lf(before)), win32_source(lf(after))
    result = {'schema': 'BlackOpsARM.TASK-0027.temp-cursor-source-preservation.v1',
              'parent': PARENT, 'verifier_sha256': sha(Path(__file__).read_bytes()),
              'comparison': 'BO1_NATIVE_64BIT=false only; all other branches retained; LF-normalized bytes',
              'stream_original_git_blob_sha256': sha(before), 'stream_current_sha256': sha(after),
              'win32_leaves': {}, 'preceding_files': {}}
    for name in LEAVES:
        old_body, new_body = function(old32, name), function(new32, name)
        if old_body != new_body:
            raise RuntimeError('Win32 body changed: ' + name)
        result['win32_leaves'][name] = {'body_unchanged': True, 'lf_sha256': sha(new_body)}
    header = 'src/database/db_stream.h'
    old_header, new_header = original(header), (ROOT / header).read_bytes()
    if win32_source(lf(old_header)) != win32_source(lf(new_header)):
        raise RuntimeError('Win32 stream declaration/type bytes changed')
    result['win32_header'] = {'native_guards_removed_bytes_unchanged': True,
                              'current_sha256': sha(new_header)}
    for path in ('src/database/db_expand.cpp', 'src/database/db_stream_load.cpp'):
        old, new = original(path), (ROOT / path).read_bytes()
        if lf(old) != lf(new):
            raise RuntimeError('Preceding production loader changed: ' + path)
        result['preceding_files'][path] = {'whole_source_lf_bytes_unchanged': True,
                                         'original_git_blob_sha256': sha(old), 'current_sha256': sha(new)}
    result.update(status='passed', native_behavior_certified=False,
                  compiler_preprocessing_certified=False, registry_execution_certified=False)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / 'build-android-audit').resolve()) or output.exists():
        raise RuntimeError('Use a fresh ignored Android source-proof output')
    try:
        result = verify()
    except Exception as error:
        result = {'schema': 'BlackOpsARM.TASK-0027.temp-cursor-source-preservation.v1',
                  'parent': PARENT, 'verifier_sha256': sha(Path(__file__).read_bytes()),
                  'status': 'failed', 'error': str(error)}
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        raise
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('PASS six Win32 stream leaves/header and preceding production loaders; native runtime not certified')


if __name__ == '__main__':
    main()
