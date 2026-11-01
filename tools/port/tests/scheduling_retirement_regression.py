"""Build real ARM64 scheduling services with isolated cpuset-boundary coverage."""
from pathlib import Path
import argparse, hashlib, json, subprocess

root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser()
p.add_argument('--out', type=Path, required=True)
p.add_argument('--scheduling-source', type=Path)
a = p.parse_args()
a.out.mkdir(parents=True, exist_ok=False)
source = a.scheduling_source or root/'src/platform/android/scheduling_services.cpp'
compiler = 'D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
flags = [compiler, '--target=aarch64-none-linux-android28',
         '--sysroot=D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/sysroot',
         '-std=c++20', '-g', '-fms-extensions', '-fsigned-char', '-ffunction-sections',
         '-fdata-sections', '-fsanitize=undefined', '-fno-sanitize-recover=all',
         '-D_DEBUG=1', '-DBO1_NATIVE_SCHEDULING_SERVICES=1', '-Isrc', '-I.',
         '-Isrc/platform/android', '-I../DXVK-Native/include/native/directx',
         '-I../DXVK-Native/include/native/windows', '-include',
         'src/platform/win32_compat/msvc_compat.h']
services = [str(source), 'src/platform/android/thread_services.cpp',
            'src/platform/android/hunk_services.cpp', 'src/platform/android/fatal_error.cpp',
            'build-android-audit/native_system_core/selected_objects/q_shared.cpp.o',
            'build-android-audit/native_system_core/selected_objects/cmd.cpp.o']
inputs = [Path(x) for x in services] + [Path(__file__),
          root/'src/platform/android/scheduling_services.h',
          root/'tools/port/tests/native_scheduling_retirement_tests.cpp',
          root/'tools/port/tests/native_scheduling_tests.cpp']
manifest = {str(x): hashlib.sha256(x.read_bytes()).hexdigest() for x in inputs}
(a.out/'source-manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
for test in ['native_scheduling_retirement_tests', 'native_scheduling_tests']:
    command = flags + ['tools/port/tests/'+test+'.cpp'] + services
    if test == 'native_scheduling_retirement_tests':
        command += ['-Wl,--wrap=sched_setaffinity']
    command += ['-Wl,--gc-sections', '-static-libstdc++', '-o', str(a.out/test)]
    (a.out/(test+'-command.json')).write_text(json.dumps(command, indent=2), encoding='utf-8')
    result = subprocess.run(command, cwd=root, capture_output=True, text=True)
    (a.out/(test+'-compile.log')).write_text(result.stdout+result.stderr, encoding='utf-8')
    print(test, 'compile exit', result.returncode, flush=True)
    if result.returncode:
        print((result.stdout+result.stderr)[-3000:])
        raise SystemExit(result.returncode)
assert all(hashlib.sha256(x.read_bytes()).hexdigest() == manifest[str(x)] for x in inputs)
