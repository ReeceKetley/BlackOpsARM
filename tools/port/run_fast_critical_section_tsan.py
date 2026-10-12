"""Attempt bounded ThreadSanitizer validation using the installed NDK runtime."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    parser.add_argument('--serial', required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use fresh worktree sanitizer output')
    output.mkdir(parents=True)
    commands = []

    def run(arguments, require_zero=True):
        completed = subprocess.run([str(a) for a in arguments], capture_output=True, text=True, timeout=30)
        commands.append({'args': [str(a) for a in arguments], 'exit': completed.returncode,
                         'stdout': completed.stdout, 'stderr': completed.stderr})
        (output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
        if require_zero and completed.returncode:
            raise RuntimeError('TSan setup failed; evidence retained')
        return completed

    units = ['src/platform/fast_critical_section.h', 'src/platform/atomic32.h',
             'src/platform/android/fast_critical_section_services.cpp', 'src/database/db_registry.cpp',
             'src/universal/dvar.cpp', 'tools/port/tests/native_fast_critical_section_tests.cpp',
             'android/cmake/native-fast-critical-section/CMakeLists.txt', 'tools/port/run_fast_critical_section_tsan.py']
    inputs = {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest() for path in units}
    (output / 'source-manifest.json').write_text(json.dumps(inputs, indent=2) + '\n', encoding='utf-8')
    sdk = Path('D:/Android/Sdk')
    ndk = sdk / 'ndk/30.0.16248370'
    cmake = sdk / 'cmake/3.22.1/bin/cmake.exe'
    build = output / 'build'
    run([cmake, '-S', ROOT / 'android/cmake/native-fast-critical-section', '-B', build, '-G', 'Ninja',
         '-DCMAKE_MAKE_PROGRAM=' + str(sdk / 'cmake/3.22.1/bin/ninja.exe'),
         '-DCMAKE_TOOLCHAIN_FILE=' + str(ndk / 'build/cmake/android.toolchain.cmake'),
         '-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-28', '-DCMAKE_BUILD_TYPE=Debug',
         '-DCMAKE_CXX_FLAGS_DEBUG=-O1 -g -fsanitize=thread'])
    run([cmake, '--build', build, '--parallel', '2', '--verbose'])
    binary = build / 'bo1_native_fast_critical_section_tests'
    runtime = ndk / 'toolchains/llvm/prebuilt/windows-x86_64/lib/clang/21/lib/linux/libclang_rt.tsan-aarch64-android.so'
    adb = [sdk / 'platform-tools/adb.exe', '-s', args.serial]
    remote = '/data/local/tmp/blackopsarm-fcs45-tsan'
    run(adb + ['get-state'])
    run(adb + ['shell', 'mkdir', '-p', remote])
    for path, name in [(binary, 'lock_tests'), (runtime, runtime.name)]:
        run(adb + ['push', path, remote + '/' + name])
        sha = hashlib.sha256(path.read_bytes()).hexdigest()
        if run(adb + ['shell', 'sha256sum', remote + '/' + name]).stdout.split()[0] != sha:
            raise RuntimeError('Transferred sanitizer binary differs')
    run(adb + ['shell', 'chmod', '700', remote + '/lock_tests'])
    completed = run(adb + ['shell', 'env', 'LD_LIBRARY_PATH=' + remote,
                    'TSAN_OPTIONS=halt_on_error=1:exitcode=66:memory_limit_mb=256', remote + '/lock_tests'], require_zero=False)
    combined = completed.stdout + completed.stderr
    for path, sha in inputs.items():
        if hashlib.sha256((ROOT / path).read_bytes()).hexdigest() != sha:
            raise RuntimeError('Source changed during sanitizer attempt')
    passed = completed.returncode == 0 and 'ThreadSanitizer' not in combined
    result = {'passed': passed, 'exit': completed.returncode, 'serial': args.serial,
              'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
              'runtime_sha256': hashlib.sha256(runtime.read_bytes()).hexdigest(),
              'memory_limit_mb': 256, 'scope': 'Positive protocol contention only; driver-owned20s watchdog; no Dvar startup.'}
    if passed:
        record = json.loads(completed.stdout)['fast_critical_section']
        if record['total_writes'] != 10000 or record['reader_checks'] != 12000 or record['passed'] is not True:
            raise RuntimeError('Sanitizer runtime record differs')
    else:
        result['diagnostic'] = combined
    (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('ThreadSanitizer protocol run passed' if passed else 'ThreadSanitizer attempt did not pass; exact diagnostic recorded')


if __name__ == '__main__':
    main()
