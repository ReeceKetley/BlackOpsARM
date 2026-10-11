"""Retain full compiler diagnostics, commands, ELF headers and source hashes."""
from pathlib import Path
import collections
import hashlib
import json
import re
import shutil
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / 'build-android-audit'
OUTPUT = ROOT / 'docs/port/evidence/arm64_phase1'
OUTPUT.mkdir(parents=True, exist_ok=True)
passes = ['pass0_raw', 'pass1_spelling', 'pass2_foundation', 'final_validated',
          'pass4_signed_char', 'pass5_native_width', 'pass6_script_slots',
          'pass7_device', 'pass9_guard_matrix', 'pass10_final']
summary = {'passes': {}, 'artifacts': [], 'finalSourceSHA256': {}}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


for name in passes:
    for leaf in ['configure.log', 'build.log', 'compile_commands.json']:
        source = BUILD / name / leaf
        destination = OUTPUT / (name + '.' + leaf)
        if source.is_file():
            if destination.exists():
                raise RuntimeError('Evidence exists: ' + str(destination))
            shutil.copyfile(source, destination)
    text = (BUILD / name / 'build.log').read_text(encoding='utf-8-sig')
    summary['passes'][name] = {
        'errors': len(re.findall(r'^.+?:\d+:\d+: (?:fatal )?error:', text, re.M)),
        'warnings': len(re.findall(r'^.+?:\d+:\d+: warning:', text, re.M)),
        'warningClasses': dict(collections.Counter(re.findall(r'\[(-W[^\]]+)\]', text))),
    }
final = BUILD / 'pass10_final'
llvm = Path('D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin')
summary['compiler'] = subprocess.check_output([str(llvm / 'clang++.exe'), '--version'], text=True)
headers = []
artifacts = sorted(final.rglob('*.o')) + [final / name for name in [
    'bo1_win32_compat_tests', 'bo1_arm64_narrow_tests', 'bo1_script_slot_tests']]
for path in artifacts:
    data = path.read_bytes()
    if data[:4] != b'\x7fELF' or data[4:6] != bytes([2, 1]) or struct.unpack_from('<H', data, 18)[0] != 183:
        raise RuntimeError('Not ELF64 little-endian AArch64: ' + str(path))
    summary['artifacts'].append({'path': str(path.relative_to(ROOT)).replace('\\', '/'),
                                  'sha256': sha(path), 'elfClass': 64, 'machine': 'AArch64 (183)'})
    header = subprocess.check_output([str(llvm / 'llvm-readelf.exe'), '-h', str(path)], text=True)
    headers += [str(path.relative_to(ROOT)), header]
commands = json.loads((final / 'compile_commands.json').read_text())
for command in commands:
    source = Path(command['file'])
    if source.is_relative_to(ROOT) and not source.is_relative_to(BUILD):
        summary['finalSourceSHA256'][str(source.relative_to(ROOT)).replace('\\', '/')] = sha(source)
    if 'BO1_PORT_X64' in command['command']:
        raise RuntimeError('Android compiler command defines legacy Windows flag')
summary['scriptDeclarationSHA256'] = json.loads((final / 'script_slot_probe.sources.json').read_text())
(OUTPUT / 'aarch64_elf_headers.txt').write_text('\n'.join(headers))
shutil.copyfile(BUILD / 'pass4_signed_char/narrow_probe.log', OUTPUT / 'before_narrow_slot_failure.log')
shutil.copyfile(BUILD / 'apk_recheck.log', OUTPUT / 'apk_recheck.log')
shutil.copyfile(final / 'device-tests.log', OUTPUT / 'honor_device_tests.log')
host = max(BUILD.glob('host-narrow-*'))
for arch in ['x86', 'x64']:
    for name in ['compile.log', 'test.log', 'slot-compile.log', 'slot-test.log', 'guard-compile.log']:
        shutil.copyfile(host / arch / name, OUTPUT / f'host_{arch}.{name}')
for name in ['compile.log', 'test.log']:
    shutil.copyfile(BUILD / 'host-errors' / name, OUTPUT / ('host_tls.' + name))
(OUTPUT / 'compiler_summary.json').write_text(json.dumps(summary, indent=2) + '\n')
print('Retained', len(passes), 'compiler passes and', len(artifacts), 'verified AArch64 artifacts.')
