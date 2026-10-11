"""Archive this bounded gate without overwriting previous compilation evidence."""
from pathlib import Path
import argparse
import collections
import hashlib
import json
import re
import shutil
import struct
import subprocess

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build', required=True)
parser.add_argument('--host', required=True)
parser.add_argument('--narrow', required=True)
parser.add_argument('--rng-host', help='Optional standalone Windows CRT oracle directory')
parser.add_argument('--parsetree-host', help='Optional actual parse-tree/HunkUser Windows probe directory')
parser.add_argument('--debugger-host', help='Optional debugger/evaluator and first-fit Windows probe directory')
parser.add_argument('--pass-prefix', default='script_headers_', help='Build directories to archive')
parser.add_argument('--output', default='docs/port/evidence/arm64_script_headers')
args = parser.parse_args()

def local(name):
    path = (root / name).resolve()
    if not path.is_relative_to(root): raise RuntimeError('Path outside Android worktree')
    return path

output = local(args.output)
if output.exists(): raise RuntimeError('Evidence exists; choose a fresh output directory')
output.mkdir(parents=True)
final, host, narrow = [local(name) for name in [args.build, args.host, args.narrow]]
summary = {'passes': {}, 'artifacts': [], 'sourceSHA256': {}, 'copiedLogs': {}}

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def copy(source, name):
    destination = output / name
    if destination.exists(): raise RuntimeError('Evidence name collision')
    shutil.copyfile(source, destination)
    summary['copiedLogs'][name] = {'source': str(source.relative_to(root)).replace('\\', '/'), 'sha256': sha(source)}

for build in sorted(final.parent.glob(args.pass_prefix + '*')):
    if not (build / 'build.log').is_file(): continue
    for leaf in ['configure.log', 'build.log', 'compile_commands.json']:
        copy(build / leaf, build.name + '.' + leaf)
    text = (build / 'build.log').read_text(encoding='utf-8-sig')
    summary['passes'][build.name] = {
        'generatorFailure': 'Traceback (most recent call last)' in text,
        'errors': len(re.findall(r'^.+?:\d+:\d+: (?:fatal )?error:', text, re.M)),
        'warnings': len(re.findall(r'^.+?:\d+:\d+: warning:', text, re.M)),
        'warningClasses': dict(collections.Counter(re.findall(r'\[(-W[^\]]+)\]', text))),
    }
new_gate = (final / 'bo1_script_lookup_tests').is_file()
host_tests = ['script_headers', 'profile', 'math_axis', 'rng']
if new_gate: host_tests += ['engine_rng', 'script_lookup']
for arch in ['x86', 'x64']:
    for name in host_tests:
        for suffix in ['compile.log', 'test.log', 'command.txt']:
            copy(host / arch / (name + '-' + suffix), 'host_' + arch + '.' + name + '.' + suffix)
    for leaf in ['compile.log', 'test.log', 'slot-compile.log', 'slot-test.log', 'guard-compile.log']:
        copy(narrow / arch / leaf, 'narrow_' + arch + '.' + leaf)
copy(narrow / 'x64/guard-off-compile.log', 'narrow_x64.guard-off-compile.log')
if args.rng_host:
    rng_host = local(args.rng_host)
    for arch in ['x86', 'x64']:
        for leaf in ['command.txt', 'compile.log', 'test.log', 'windows_crt_rng_vectors.h']:
            copy(rng_host / arch / leaf, 'rng_oracle_' + arch + '.' + leaf)
if args.parsetree_host:
    parse_host = local(args.parsetree_host)
    for arch in ['x86', 'x64']:
        for name in ['parsetree', 'hunk']:
            for suffix in ['compile.log', 'test.log', 'command.txt']:
                copy(parse_host / arch / (name + '-' + suffix), 'parse_host_' + arch + '.' + name + '.' + suffix)
if args.debugger_host:
    debugger_host = local(args.debugger_host)
    for arch in ['x86', 'x64']:
        for name in ['debugger', 'firstfit']:
            for suffix in ['compile.log', 'test.log', 'command.txt']:
                copy(debugger_host / arch / (name + '-' + suffix), 'debugger_host_' + arch + '.' + name + '.' + suffix)
    copy(debugger_host / 'evaluator_test_services.cpp', 'host_evaluator_test_services.cpp')
    copy(final / 'evaluator_test_services.cpp', 'android_evaluator_test_services.cpp')
for leaf in ['device-tests.log', 'apk-recheck.log', 'diff-check.log']:
    copy(final / leaf, leaf)
llvm = Path('D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin')
headers = []
binaries = ['bo1_win32_compat_tests', 'bo1_arm64_narrow_tests', 'bo1_script_slot_tests',
            'bo1_script_headers_tests', 'bo1_profile_tests', 'bo1_math_axis_tests', 'bo1_rng_tests']
if new_gate: binaries += ['bo1_engine_rng_tests', 'bo1_script_lookup_tests']
if args.parsetree_host: binaries += ['bo1_parsetree_tests', 'bo1_hunk_tests']
if args.debugger_host: binaries += ['bo1_debugger_tests', 'bo1_firstfit_tests']
for path in sorted(final.rglob('*.o')) + [final / name for name in binaries]:
    data = path.read_bytes()
    if data[:6] != b'\x7fELF\x02\x01' or struct.unpack_from('<H', data, 18)[0] != 183:
        raise RuntimeError('Not ELF64 little-endian AArch64: ' + str(path))
    summary['artifacts'].append({'path': str(path.relative_to(root)).replace('\\', '/'), 'sha256': sha(path), 'machine': 'AArch64'})
    headers.extend([str(path.relative_to(root)), subprocess.check_output([str(llvm / 'llvm-readelf.exe'), '-h', str(path)], text=True)])
commands = json.loads((final / 'compile_commands.json').read_text())
for command in commands:
    if '-DBO1_PORT_X64' in command['command']: raise RuntimeError('Legacy flag defined on Android')
    path = Path(command['file'])
    summary['sourceSHA256'][str(path.relative_to(root)).replace('\\', '/')] = sha(path)
for name in ['clientscript/cscr_main.h', 'clientscript/cscr_variable.h', 'clientscript/cscr_debugger.h',
             'clientscript/cscr_instance.h', 'clientscript/cscr_stringlist.h', 'universal/mem_userhunk.h',
             'universal/q_shared.h', 'universal/profile.h', 'universal/com_math.h', 'universal/com_math.cpp',
             'platform/profile_services.h', 'platform/android/profile_services.h', 'universal/narrow32.h']:
    summary['sourceSHA256']['src/' + name] = sha(root / 'src' / name)
if new_gate:
    for name in ['platform/engine_random.h', 'platform/android/engine_random.h', 'clientscript/cscr_readwrite.h']:
        summary['sourceSHA256']['src/' + name] = sha(root / 'src' / name)
    summary['sourceSHA256']['tools/port/tests/windows_crt_rng_vectors.h'] = sha(root / 'tools/port/tests/windows_crt_rng_vectors.h')
if args.parsetree_host:
    for name in ['clientscript/cscr_parsetree.h', 'clientscript/cscr_vm.h', 'universal/com_errors.h',
                 'universal/memory_services.h', 'universal/com_memory.h', 'qcommon/common.h',
                 'platform/android/virtual_memory.h']:
        summary['sourceSHA256']['src/' + name] = sha(root / 'src' / name)
    summary['parsetreeHostBuild'] = str(parse_host.relative_to(root)).replace('\\', '/')
if args.debugger_host:
    for name in ['clientscript/cscr_evaluate.h', 'clientscript/cscr_yacc.h', 'universal/mem_firstfit.h',
                 'platform/android/hunk_services.h', 'platform/android/zone_heap.h', 'xanim/xanim_debug.h']:
        summary['sourceSHA256']['src/' + name] = sha(root / 'src' / name)
    summary['debuggerHostBuild'] = str(debugger_host.relative_to(root)).replace('\\', '/')
    summary['executables'] = binaries
summary['compiler'] = subprocess.check_output([str(llvm / 'clang++.exe'), '--version'], text=True)
summary['finalBuild'] = str(final.relative_to(root)).replace('\\', '/')
summary['hostBuild'] = str(host.relative_to(root)).replace('\\', '/')
summary['narrowBuild'] = str(narrow.relative_to(root)).replace('\\', '/')
(output / 'aarch64_elf_headers.txt').write_text('\n'.join(headers), encoding='utf-8')
(output / 'compiler_summary.json').write_text(json.dumps(summary, indent=2) + '\n', encoding='utf-8')
print('Retained', len(summary['passes']), 'passes and verified', len(summary['artifacts']), 'AArch64 artifacts')
