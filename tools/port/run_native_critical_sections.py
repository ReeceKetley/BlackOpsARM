"""Bounded native lock validation: Windows host probes and exact HONOR O0/O3."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host-build', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--serial', required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    host = (ROOT / args.host_build).resolve()
    if not output.is_relative_to(ROOT) or not host.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use worktree host and fresh output')
    output.mkdir(parents=True)
    commands = []

    def run(arguments, expected=0):
        completed = subprocess.run([str(a) for a in arguments], capture_output=True, text=True, timeout=30)
        commands.append({'args': [str(a) for a in arguments], 'exit': completed.returncode,
                         'stdout': completed.stdout, 'stderr': completed.stderr})
        (output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
        if expected is None:
            if not completed.returncode or 'Native critical sections: invalid identifier' not in completed.stdout + completed.stderr:
                raise RuntimeError('Invalid identifier did not fail explicitly')
        elif completed.returncode != expected:
            raise RuntimeError('Operation failed; command evidence retained')
        return completed.stdout

    def validate(stdout, width):
        actual = json.loads(stdout)
        expected = {'native_critical_sections': {'pointer_bytes': width, 'sections': 75,
                    'initializers': 12, 'protected_increments': 30000, 'shared_pool_locks': 3, 'passed': True}}
        if actual != expected or set(actual) != set(expected) or any(type(actual['native_critical_sections'][k]) is not type(v) for k, v in expected['native_critical_sections'].items()):
            raise RuntimeError('Runtime evidence differs')

    def controls(prefix, abort_exit):
        for operation in ('enter', 'try', 'leave'):
            for value in ('-1', '75'):
                run(prefix + ['--invalid-' + operation, value], expected=None)
                if commands[-1]['exit'] != abort_exit:
                    raise RuntimeError('Invalid identifier did not terminate through expected abort')

    rows = []
    for arch, width in (('x86', 4), ('x64', 8)):
        binary = host / arch / 'native_critical_sections.exe'
        validate(run([binary]), width)
        controls([binary], 3)
        manifest = json.loads((host / arch / 'source-manifest.json').read_text(encoding='utf-8-sig'))
        if any(hashlib.sha256((ROOT / path).read_bytes()).hexdigest() != sha.lower() for path, sha in manifest.items()):
            raise RuntimeError('Host build source changed')
        rows.append({'architecture': arch, 'passed': True, 'invalid_controls': 6,
                     'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest()})
    adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', args.serial]
    run(adb + ['get-state'])
    if run(adb + ['shell', 'getprop', 'ro.product.cpu.abi']).strip() != 'arm64-v8a':
        raise RuntimeError('Expected ARM64 device')
    units = ['src/platform/android/script_pool_bridge.cpp', 'src/platform/android/script_string_services.cpp',
             'src/platform/android/script_string_services.h', 'src/platform/critical_section_services.h',
             'src/universal/critical_sections.h', 'src/platform/win32_compat/msvc_compat.h',
             'tools/port/tests/native_critical_sections_tests.cpp', 'android/cmake/native-critical-sections/CMakeLists.txt',
             'tools/port/run_native_critical_sections.py']
    manifest = {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest() for path in units}
    (output / 'source-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    cmake = 'D:/Android/Sdk/cmake/3.22.1/bin/cmake.exe'
    for opt in ('O0', 'O3'):
        build = output / opt
        run([cmake, '-S', ROOT / 'android/cmake/native-critical-sections', '-B', build, '-G', 'Ninja',
             '-DCMAKE_MAKE_PROGRAM=D:/Android/Sdk/cmake/3.22.1/bin/ninja.exe',
             '-DCMAKE_TOOLCHAIN_FILE=D:/Android/Sdk/ndk/30.0.16248370/build/cmake/android.toolchain.cmake',
             '-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-28', '-DCMAKE_BUILD_TYPE=Debug',
             '-DCMAKE_CXX_FLAGS_DEBUG=-' + opt + ' -g'])
        run([cmake, '--build', build, '--parallel', '2', '--verbose'])
        binary = build / 'bo1_native_critical_sections_tests'
        data = binary.read_bytes()
        if data[:5] != b'\x7fELF\x02' or int.from_bytes(data[18:20], 'little') != 183:
            raise RuntimeError('Expected native ARM64 ELF')
        remote = '/data/local/tmp/blackopsarm-critical43-' + opt
        run(adb + ['shell', 'mkdir', '-p', remote])
        run(adb + ['push', binary, remote + '/lock_tests'])
        sha = hashlib.sha256(data).hexdigest()
        if run(adb + ['shell', 'sha256sum', remote + '/lock_tests']).split()[0] != sha:
            raise RuntimeError('Pushed binary differs')
        run(adb + ['shell', 'chmod', '700', remote + '/lock_tests'])
        validate(run(adb + ['shell', remote + '/lock_tests']), 8)
        controls(adb + ['shell', remote + '/lock_tests'], 134)
        rows.append({'architecture': 'arm64', 'optimization': opt, 'serial': args.serial,
                     'passed': True, 'invalid_controls': 6, 'binary_sha256': sha})
    if any(hashlib.sha256((ROOT / path).read_bytes()).hexdigest() != sha for path, sha in manifest.items()):
        raise RuntimeError('Source changed during validation')
    (output / 'result.json').write_text(json.dumps({'passed': True, 'runs': rows,
            'scope': 'Native mutex services, not engine TLS/FastCriticalSection/Com_Error/Com_Init.'}, indent=2) + '\n', encoding='utf-8')
    print('All75 recursive sections, preserved pool-lock identity, contention and24 invalid controls pass on Windows/HONOR O0/O3')


if __name__ == '__main__':
    main()
