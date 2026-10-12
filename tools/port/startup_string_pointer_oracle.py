"""Check exact historical two-buffer string-pointer policy against native body.

TLS lookup is an explicitly controlled oracle input; actual thread integration
is validated by the separate production startup-services driver.
"""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
from thread_data_layout_oracle import declaration
from verify_bootstrap_default_source import body

ROOT = Path(__file__).resolve().parents[2]
BASELINE = '8f81f9c'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    parser.add_argument('--serial', required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use fresh worktree output')
    output.mkdir(parents=True)
    historical = subprocess.check_output(['git', 'show', BASELINE + ':src/universal/q_shared.cpp'], cwd=ROOT).decode('utf-8-sig').replace('\r\n', '\n')
    header = subprocess.check_output(['git', 'show', BASELINE + ':src/universal/thread_data.h'], cwd=ROOT).decode('utf-8-sig')
    old = body(historical, 'CanKeepStringPointer')
    current = body((ROOT / 'src/universal/q_shared.cpp').read_text(encoding='utf-8-sig'), 'CanKeepStringPointer')
    code = '#include <cstdio>\n#include <cstdint>\n#define BO1_STARTUP_SERVICES_NATIVE_HOST_TEST 1\n'
    code += declaration(header, 'va_info_t') + '\nstatic va_info_t input;\n'
    code += 'static void* Sys_GetValue(int slot){if(slot!=1)std::abort();return &input;}\n'
    code = '#include <cstdlib>\n' + code
    code += old.replace('CanKeepStringPointer(', 'OriginalCanKeepStringPointer(', 1) + '\n'
    code += current + '\n'
    code += '''int main(){
 unsigned count=0;
 for(unsigned offset=0;offset<sizeof(input);++offset){
  char *pointer=reinterpret_cast<char*>(&input)+offset;
  const bool expected=offset>2047;
  if(OriginalCanKeepStringPointer(pointer)!=expected || CanKeepStringPointer(pointer)!=expected)return 1;
  ++count;
 }
 std::printf("{\\"pointer_bytes\\":%zu,\\"checked_offsets\\":%u,\\"excluded_last_offset\\":2047,\\"passed\\":true}\\n",sizeof(void*),count);
}
'''
    cpp = output / 'string_pointer_original_and_native.cpp'
    cpp.write_text(code, encoding='utf-8', newline='\n')
    commands, results = [], []

    def run(argv, cwd=ROOT):
        result = subprocess.run(argv if isinstance(argv, str) else [str(x) for x in argv], cwd=cwd,
                                capture_output=True, text=True, timeout=30)
        commands.append(dict(argv=argv if isinstance(argv, str) else [str(x) for x in argv],
                             exit=result.returncode, stdout=result.stdout, stderr=result.stderr))
        (output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
        if result.returncode or 'runtime error:' in result.stdout + result.stderr:
            raise RuntimeError('Pointer policy oracle failed; evidence retained')
        return result.stdout

    def validate(stdout, width, architecture):
        row = json.loads(stdout)
        if row != dict(pointer_bytes=width, checked_offsets=4100, excluded_last_offset=2047, passed=True):
            raise RuntimeError('Pointer policy result differs')
        results.append(dict(architecture=architecture, result=row))

    for arch, width in (('x86', 4), ('x64', 8)):
        directory = output / arch
        directory.mkdir()
        binary = directory / 'oracle.exe'
        command = '"C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat" ' + arch + ' >nul && cl /nologo /W4 /std:c++20 /EHsc /MT /O2 "' + str(cpp) + '" /Fe"' + str(binary) + '" /Fo"' + str(directory) + '/"'
        run('cmd /d /s /c "' + command + '"', directory)
        validate(run([binary]), width, arch)
    binary = output / 'oracle_arm64'
    ndk = Path('D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64')
    run([ndk / 'bin/clang++.exe', '--target=aarch64-linux-android26', '-std=c++20',
         '-fms-extensions', '-static-libstdc++', '-fsanitize=undefined', '-fno-sanitize-recover=all',
         '-O2', cpp, '-o', binary])
    adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', args.serial]
    if run(adb + ['get-state']).strip() != 'device':
        raise RuntimeError('Device unavailable')
    remote = '/data/local/tmp/blackopsarm-string-pointer-' + output.name
    run(adb + ['shell', 'mkdir', '-p', remote])
    run(adb + ['push', binary, remote + '/oracle'])
    binary_sha = hashlib.sha256(binary.read_bytes()).hexdigest()
    if run(adb + ['shell', 'sha256sum', remote + '/oracle']).split()[0] != binary_sha:
        raise RuntimeError('Device binary differs')
    run(adb + ['shell', 'chmod', '700', remote + '/oracle'])
    validate(run(adb + ['shell', remote + '/oracle']), 8, 'arm64')
    (output / 'result.json').write_text(json.dumps(dict(passed=True, baseline=BASELINE,
        original_body_sha256=hashlib.sha256(old.encode()).hexdigest(),
        native_body_sha256=hashlib.sha256(current.encode()).hexdigest(),
        generated_cpp_sha256=hashlib.sha256(cpp.read_bytes()).hexdigest(),
        arm_binary_sha256=binary_sha, runs=results), indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
