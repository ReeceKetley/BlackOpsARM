"""Compare selected native CRT services with independently executed MSVC CRT."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    parser.add_argument('--original', required=True)
    parser.add_argument('--serial', required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    original = (ROOT / args.original).resolve()
    if not output.is_relative_to(ROOT) or output.exists() or not original.is_relative_to(ROOT):
        raise RuntimeError('Use fresh worktree output and worktree oracle')
    output.mkdir(parents=True)
    commands = []

    def run(argv):
        result = subprocess.run([str(x) for x in argv], capture_output=True, text=True, timeout=40)
        commands.append({'argv': [str(x) for x in argv], 'exit': result.returncode,
                         'stdout': result.stdout, 'stderr': result.stderr})
        (output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
        if result.returncode or 'runtime error:' in result.stdout + result.stderr:
            raise RuntimeError('CRT comparison command failed; evidence retained')
        return result.stdout

    expected = None
    for arch in ('x86', 'x64'):
        result = json.loads((original / arch / 'result.json').read_text(encoding='utf-8-sig'))
        if result['exit_code'] != 0:
            raise RuntimeError('Original MSVC oracle failed')
        rows = [json.loads(line) for line in (original / arch / 'run.log').read_text(encoding='utf-8-sig').splitlines()]
        if len(rows) != 230 or (expected is not None and expected != rows):
            raise RuntimeError('Independent Windows oracles differ')
        expected = rows
    sources = ['tools/port/startup_crt_oracle.cpp', 'src/platform/android/legacy_crt.cpp',
               'src/platform/android/legacy_crt.h']
    manifest = {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in sources}
    (output / 'source-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', args.serial]
    if run(adb + ['get-state']).strip() != 'device':
        raise RuntimeError('Requested device unavailable')
    device = '/data/local/tmp/blackopsarm-startup-crt-' + output.name
    run(adb + ['shell', 'mkdir', '-p', device])
    compiler = 'D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
    results = []
    for optimization in ('O0', 'O3'):
        binary = output / ('startup_crt_' + optimization)
        run([compiler, '--target=aarch64-linux-android26', '-std=c++20', '-fms-extensions',
             '-DBO1_STARTUP_CRT_NATIVE_ORACLE=1', '-I' + str(ROOT / 'src'), '-' + optimization,
             '-fsanitize=undefined', '-fno-sanitize-recover=all', '-static-libstdc++',
             ROOT / sources[0], ROOT / sources[1], '-o', binary])
        remote = device + '/' + binary.name
        run(adb + ['push', binary, remote])
        if run(adb + ['shell', 'sha256sum', remote]).split()[0] != hashlib.sha256(binary.read_bytes()).hexdigest():
            raise RuntimeError('Device CRT binary checksum differs')
        run(adb + ['shell', 'chmod', '700', remote])
        actual = [json.loads(line) for line in run(adb + ['shell', remote]).splitlines()]
        if actual != expected:
            raise RuntimeError('Native output differs from genuine MSVC oracle')
        results.append({'optimization': optimization, 'records': len(actual),
                        'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest()})
    if manifest != {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in sources}:
        raise RuntimeError('Sources changed during validation')
    (output / 'result.json').write_text(json.dumps({'passed': True, 'results': results}, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
