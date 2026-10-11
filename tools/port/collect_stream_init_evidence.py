"""Freeze genuine stream-initialization tests and source/build provenance."""
from pathlib import Path
import argparse
import collections
import hashlib
import json
import re
import shutil
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
LLVM = Path('D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin')

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for name in ['build', 'host', 'run', 'zone-regression', 'narrow-regression', 'output']:
        ap.add_argument('--'+name, type=Path, required=True)
    args = ap.parse_args()
    for p in vars(args).values():
        if not p.resolve().is_relative_to(ROOT):
            raise RuntimeError('Only Android worktree paths allowed')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    manifest = {'schema': 'BlackOpsARM.TASK-0012.evidence.v1', 'files': {}, 'builds': {}, 'artifacts': [], 'source_sha256': {}}
    def copy(p, name):
        shutil.copyfile(p, out/name)
        manifest['files'][name] = {'source': p.resolve().relative_to(ROOT).as_posix(), 'sha256': sha(p)}
    for build in sorted(args.build.parent.glob('stream_init_pass*')):
        for leaf in ['configure.log', 'build.log', 'compile_commands.json', 'device-tests.log']:
            if (build/leaf).is_file():
                copy(build/leaf, build.name+'.'+leaf)
        text = (build/'build.log').read_text(encoding='utf-8-sig')
        manifest['builds'][build.name] = {'compiler_errors': len(re.findall(r'^.+?:\d+:\d+: (?:fatal )?error:', text, re.M)),
            'linker_errors': len(re.findall(r'^ld.lld: error:', text, re.M)),
            'warnings': len(re.findall(r'^.+?:\d+:\d+: warning:', text, re.M)),
            'warning_classes': dict(collections.Counter(re.findall(r'\[(-W[^\]]+)\]', text)))}
    for host in sorted(args.host.parent.glob('stream_init_host_pass*')):
        for p in sorted(host.rglob('*')):
            if p.is_file() and p.name in ['compile.log', 'command.txt']:
                copy(p, host.name+'.'+p.relative_to(host).as_posix().replace('/', '.'))
    for run in sorted(args.run.parent.glob('stream_init_run*')):
        for p in sorted(run.iterdir()):
            if p.is_file() and p.suffix in ['.log', '.json']:
                copy(p, run.name+'.'+p.name)
    for label, directory in [('zone', args.zone_regression), ('narrow', args.narrow_regression)]:
        for p in sorted(directory.rglob('*')):
            if p.is_file() and (p.suffix=='.log' or p.name=='command.txt'):
                copy(p, label+'.'+p.relative_to(directory).as_posix().replace('/', '.'))
    binaries = sorted(p for p in args.build.glob('bo1_*tests') if p.is_file())
    for p in sorted(args.build.rglob('*.o'))+binaries:
        data = p.read_bytes()
        if data[:6] != b'\x7fELF\x02\x01' or struct.unpack_from('<H', data, 18)[0] != 183:
            raise RuntimeError('Unexpected ELF architecture: '+str(p))
        manifest['artifacts'].append({'path': p.resolve().relative_to(ROOT).as_posix(), 'sha256': sha(p), 'machine': 'AArch64'})
    symbols = subprocess.check_output([LLVM/'llvm-nm.exe', '-C', args.build/'bo1_stream_init_tests'], text=True)
    required = ['DB_InitStreams(', 'DB_FreeXBlocks(', 'DB_MemAlloc(', 'DB_IncStreamPos(']
    forbidden = ['Load_XAssetListCustom(', 'DB_InitZone(', 'DB_InsertPointer(', 'DB_LoadXFileData(', 'Load_ScriptStringList(']
    if any(s not in symbols for s in required) or any(s in symbols for s in forbidden):
        raise RuntimeError('Incorrect linked stream boundary')
    (out/'stream_init_symbols.txt').write_text(symbols, encoding='utf-8')
    manifest['production_symbols_present'] = required
    manifest['later_loading_symbols_absent'] = forbidden
    for c in json.loads((args.build/'compile_commands.json').read_text(encoding='utf-8')):
        if '-DBO1_PORT_X64' in c['command']:
            raise RuntimeError('Android legacy flag enabled')
        p = Path(c['file']).resolve()
        if p.is_relative_to(ROOT) and p.is_file():
            manifest['source_sha256'][p.relative_to(ROOT).as_posix()] = sha(p)
    for name in ['src/universal/physicalmemory.cpp', 'src/platform/fastfile_zone.h',
                 'src/database/db_stream.h', 'src/database/db_expand.cpp', 'src/database/db_file_load.cpp',
                 'tools/port/run_stream_init_gate.py', 'tools/port/build_stream_init_host.ps1',
                 'tools/port/collect_stream_init_evidence.py']:
        manifest['source_sha256'][name] = sha(ROOT/name)
    for arch, machine in [('x86',0x14c), ('x64',0x8664)]:
        p = args.host/arch/'stream_init.exe'
        data = p.read_bytes(); pe = struct.unpack_from('<I', data, 0x3c)[0]
        if data[pe:pe+4]!=b'PE\0\0' or struct.unpack_from('<H',data,pe+4)[0]!=machine:
            raise RuntimeError('Incorrect Windows architecture')
        manifest['artifacts'].append({'path': p.resolve().relative_to(ROOT).as_posix(), 'sha256': sha(p), 'machine': arch})
    manifest['aarch64_artifact_count'] = len(manifest['artifacts'])-2
    manifest['native_test_executables'] = [p.name for p in binaries]
    manifest['final_build'] = args.build.as_posix(); manifest['host_build'] = args.host.as_posix(); manifest['final_run'] = args.run.as_posix()
    manifest['initial_run_status'] = 'Passed; superseded by independently observable unequal-size stride reset and nonzero drift assertions, not a compiler/runtime failure'
    manifest['compiler'] = subprocess.check_output([LLVM/'clang++.exe', '--version'], text=True)
    (out/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    print('Frozen stream evidence:', out, manifest['aarch64_artifact_count'], 'AArch64 artifacts')

if __name__ == '__main__':
    main()
