"""Measure original canonical Dvar declarations independently on three ABIs."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]
BASELINE = '8f81f9c'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    parser.add_argument('--serial', required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use fresh Android worktree output')
    output.mkdir(parents=True)
    raw = subprocess.check_output(['git', 'show', BASELINE + ':src/universal/dvar.h'], cwd=ROOT)
    text = raw.decode('utf-8-sig').replace('\r\n', '\n')
    declarations = text[text.index('enum DvarSetSource'):text.index('using dvar_t')]
    fields = {'value_size': 'sizeof(DvarValue)', 'limits_size': 'sizeof(DvarLimits)',
              'dvar_size': 'sizeof(dvar_s)', 'dvar_align': 'alignof(dvar_s)',
              **{field: 'offsetof(dvar_s,' + field + ')' for field in
                 ('current', 'latched', 'reset', 'saved', 'domain', 'hashNext')}}
    expected = {
        4: dict(value_size=16, limits_size=16, dvar_size=112, dvar_align=8,
                current=24, latched=40, reset=56, saved=72, domain=88, hashNext=104),
        8: dict(value_size=16, limits_size=16, dvar_size=120, dvar_align=8,
                current=32, latched=48, reset=64, saved=80, domain=96, hashNext=112)}
    assertions = '\n'.join(f'static_assert(sizeof(void*) != {width} || {fields[key]} == {value});'
                           for width, values in expected.items() for key, value in values.items())
    fmt = '{"pointer_bytes":%zu,' + ','.join('"' + key + '":%zu' for key in fields) + '}\n'
    source = '#include <cstddef>\n#include <cstdio>\n' + declarations + '\n' + assertions
    source += '\nint main(){std::printf(' + json.dumps(fmt) + ',sizeof(void*),' + ','.join(fields.values()) + ');}\n'
    cpp = output / 'original_dvar_layout.cpp'
    cpp.write_text(source, encoding='utf-8', newline='\n')
    commands, results = [], []

    def run(argv, cwd=ROOT):
        result = subprocess.run(argv if isinstance(argv, str) else [str(x) for x in argv],
                                cwd=cwd, capture_output=True, text=True, timeout=30)
        commands.append(dict(argv=argv if isinstance(argv, str) else [str(x) for x in argv],
                             exit=result.returncode, stdout=result.stdout, stderr=result.stderr))
        (output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
        if result.returncode:
            raise RuntimeError('Historical Dvar oracle failed; evidence retained')
        return result.stdout

    def validate(stdout, width, arch, binary):
        row = json.loads(stdout)
        if row != dict(expected[width], pointer_bytes=width) or any(type(v) is not int for v in row.values()):
            raise RuntimeError('Original Dvar layout differs from independent model')
        results.append(dict(architecture=arch, layout=row, binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest()))

    for arch, width in (('x86', 4), ('x64', 8)):
        directory = output / arch
        directory.mkdir()
        binary = directory / 'oracle.exe'
        command = '"C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat" ' + arch + ' >nul && cl /nologo /W4 /std:c++20 /EHsc /MT /O2 "' + str(cpp) + '" /Fe"' + str(binary) + '" /Fo"' + str(directory) + '/"'
        run('cmd /d /s /c "' + command + '"', directory)
        validate(run([binary]), width, arch, binary)
    binary = output / 'oracle_arm64'
    ndk = Path('D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64')
    run([ndk / 'bin/clang++.exe', '--target=aarch64-linux-android26', '-std=c++20',
         '-fms-extensions', '-static-libstdc++', '-O2', cpp, '-o', binary])
    adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', args.serial]
    if run(adb + ['get-state']).strip() != 'device':
        raise RuntimeError('Device unavailable')
    remote = '/data/local/tmp/blackopsarm-dvar-layout-' + output.name
    run(adb + ['shell', 'mkdir', '-p', remote])
    run(adb + ['push', binary, remote + '/oracle'])
    if run(adb + ['shell', 'sha256sum', remote + '/oracle']).split()[0] != hashlib.sha256(binary.read_bytes()).hexdigest():
        raise RuntimeError('Device binary checksum differs')
    run(adb + ['shell', 'chmod', '700', remote + '/oracle'])
    validate(run(adb + ['shell', remote + '/oracle']), 8, 'arm64', binary)
    (output / 'result.json').write_text(json.dumps(dict(passed=True, baseline=BASELINE,
        canonical_header_sha256=hashlib.sha256(raw).hexdigest(),
        generated_cpp_sha256=hashlib.sha256(cpp.read_bytes()).hexdigest(), runs=results), indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
