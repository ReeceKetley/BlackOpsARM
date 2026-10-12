"""Compile historical thread declarations on Windows and native ARM independently."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
from thread_data_layout_oracle import EXPECTED

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--oracle-directory', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--serial', required=True)
    args = parser.parse_args()
    oracle = (ROOT / args.oracle_directory).resolve()
    output = (ROOT / args.output).resolve()
    if not oracle.is_relative_to(ROOT) or not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use worktree oracle and fresh output')
    source = oracle / 'thread_data_original.cpp'
    provenance = json.loads((oracle / 'oracle.json').read_text())
    source_sha = hashlib.sha256(source.read_bytes()).hexdigest()
    if source_sha != provenance['generated_cpp_sha256']:
        raise RuntimeError('Historical declaration oracle changed')
    output.mkdir(parents=True)
    commands = []

    def run(arguments, cwd=ROOT):
        invocation = arguments if isinstance(arguments, str) else [str(a) for a in arguments]
        completed = subprocess.run(invocation, cwd=cwd, capture_output=True, text=True, timeout=30)
        commands.append({'args': invocation, 'cwd': str(cwd),
                         'exit': completed.returncode, 'stdout': completed.stdout, 'stderr': completed.stderr})
        (output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
        if completed.returncode:
            raise RuntimeError('Historical layout compilation/runtime failed')
        return completed.stdout

    rows = []

    def validate(stdout, width, architecture, binary):
        row = json.loads(stdout)
        expected = dict(EXPECTED[width], pointer_bytes=width)
        if row != expected or any(type(value) is not int for value in row.values()):
            raise RuntimeError('Historical layout differs from independent offset model')
        rows.append({'architecture': architecture, 'layout': row,
                     'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest()})

    for arch, width in (('x86', 4), ('x64', 8)):
        directory = output / arch
        directory.mkdir()
        binary = directory / 'thread_data_original.exe'
        command = '"C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat" ' + arch + ' >nul && cl /nologo /W4 /std:c++20 /EHsc /MT /O2 "' + str(source) + '" /Fe"' + str(binary) + '" /Fo"' + str(directory) + '/"'
        # cmd.exe uses its own quoting rules; Python list2cmdline's backslash
        # quoting is inappropriate for the /c payload. All paths are owned here.
        run('cmd /d /s /c "' + command + '"', directory)
        validate(run([binary]), width, arch, binary)
    ndk = Path('D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64')
    binary = output / 'thread_data_original_arm64'
    run([ndk / 'bin/clang++.exe', '--target=aarch64-none-linux-android28',
         '--sysroot=' + str(ndk / 'sysroot'), '-std=c++20', '-O2', '-fms-extensions',
         '-fsigned-char', '-static-libstdc++', source, '-o', binary])
    data = binary.read_bytes()
    if data[:5] != b'\x7fELF\x02' or int.from_bytes(data[18:20], 'little') != 183:
        raise RuntimeError('Expected native ARM64 oracle binary')
    adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', args.serial]
    run(adb + ['get-state'])
    remote = '/data/local/tmp/blackopsarm-thread44-layout'
    run(adb + ['shell', 'mkdir', '-p', remote])
    run(adb + ['push', binary, remote + '/oracle'])
    if run(adb + ['shell', 'sha256sum', remote + '/oracle']).split()[0] != hashlib.sha256(data).hexdigest():
        raise RuntimeError('Pushed oracle binary differs')
    run(adb + ['shell', 'chmod', '700', remote + '/oracle'])
    validate(run(adb + ['shell', remote + '/oracle']), 8, 'arm64', binary)
    (output / 'result.json').write_text(json.dumps({'passed': True, 'baseline': provenance['baseline'],
            'historical_cpp_sha256': source_sha, 'runs': rows,
            'scope': 'Historical canonical declaration layout oracle, not thread-service execution.'}, indent=2) + '\n', encoding='utf-8')
    print('Independent historical T5 thread-data layouts pass Windowsx86/x64 and HONOR ARM64')


if __name__ == '__main__':
    main()
