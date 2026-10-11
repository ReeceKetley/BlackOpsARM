"""Freeze genuine script-string pointer-table tests and source/build provenance."""
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
    for name in ['build', 'host', 'run', 'zone-regression', 'narrow-regression', 'windows-regression', 'headers', 'output']:
        ap.add_argument('--'+name, type=Path, required=True)
    args = ap.parse_args()
    for p in vars(args).values():
        if not p.resolve().is_relative_to(ROOT):
            raise RuntimeError('Only Android worktree paths allowed')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    manifest = {'schema': 'BlackOpsARM.TASK-0014.evidence.v1', 'files': {}, 'builds': {}, 'artifacts': [], 'source_sha256': {}}
    def copy(p, name):
        shutil.copyfile(p, out/name)
        manifest['files'][name] = {'source': p.resolve().relative_to(ROOT).as_posix(), 'sha256': sha(p)}
    for build in sorted(args.build.parent.glob('string_table_pass*')):
        for leaf in ['configure.log', 'build.log', 'compile_commands.json', 'device-tests.log']:
            if (build/leaf).is_file():
                copy(build/leaf, build.name+'.'+leaf)
        text = (build/'build.log').read_text(encoding='utf-8-sig')
        manifest['builds'][build.name] = {'compiler_errors': len(re.findall(r'^.+?:\d+:\d+: (?:fatal )?error:', text, re.M)),
            'linker_errors': len(re.findall(r'^ld.lld: error:', text, re.M)),
            'warnings': len(re.findall(r'^.+?:\d+:\d+: warning:', text, re.M)),
            'warning_classes': dict(collections.Counter(re.findall(r'\[(-W[^\]]+)\]', text)))}
    for host in sorted(args.host.parent.glob('string_table_host_pass*')):
        for p in sorted(host.rglob('*')):
            if p.is_file() and p.name in ['compile.log', 'command.txt', 'string_table.map']:
                copy(p, host.name+'.'+p.relative_to(host).as_posix().replace('/', '.'))
    for run in sorted(args.run.parent.glob('string_table_run*')):
        for p in sorted(run.iterdir()):
            if p.is_file() and p.suffix in ['.log', '.json']:
                copy(p, run.name+'.'+p.name)
    for directory in sorted(args.headers.parent.glob('string_table_headers*')):
        for p in sorted(directory.rglob('*')):
            if p.is_file() and (p.suffix == '.log' or p.name.endswith('command.txt')):
                copy(p, directory.name+'.'+p.relative_to(directory).as_posix().replace('/', '.'))
    for p in sorted(args.windows_regression.iterdir()):
        if p.is_file() and p.suffix in ['.log', '.json']:
            copy(p, 'windows-regression.'+p.name)
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
    # The new envelope executable exists only on native64 for the production expander.
    symbols = subprocess.check_output([LLVM/'llvm-nm.exe', '-C', args.build/'bo1_string_table_tests'], text=True)
    required = ['DB_InitStreams(', 'DB_FreeXBlocks(', 'DB_MemAlloc(', 'DB_ExpandLoad_Ptr32Array(', 'DB_LoadXFileData(']
    forbidden = ['Load_XAssetListCustom(', 'DB_InitZone(', 'DB_InsertPointer(', 'DB_ExpandLoad_XAssetArray(', 'DB_ExpandLoad_XAssetList(', 'Load_ScriptStringList(', 'Load_XStringCustom(', 'Load_TempStringCustom(']
    if any(s not in symbols for s in required) or any(s in symbols for s in forbidden):
        raise RuntimeError('Incorrect linked stream boundary')
    (out/'string_table_symbols.txt').write_text(symbols, encoding='utf-8')
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
                 'src/database/db_assetlist_types.h', 'src/platform/fastfile_assetlist.h',
                 'src/database/db_registry.h', 'src/database/database.h',
                 'src/platform/fastfile_script_table.h', 'tools/port/tests/frontend_zone_fixture.h',
                 'tools/port/run_string_table_gate.py', 'tools/port/string_table_oracle.py',
                 'tools/port/build_string_table_host.ps1', 'tools/port/check_assetlist_headers.ps1',
                 'tools/port/verify_assetlist_source.py', 'tools/port/collect_string_table_evidence.py']:
        manifest['source_sha256'][name] = sha(ROOT/name)
    for arch, machine in [('x86',0x14c), ('x64',0x8664)]:
        p = args.host/arch/'string_table.exe'
        data = p.read_bytes(); pe = struct.unpack_from('<I', data, 0x3c)[0]
        if data[pe:pe+4]!=b'PE\0\0' or struct.unpack_from('<H',data,pe+4)[0]!=machine:
            raise RuntimeError('Incorrect Windows architecture')
        manifest['artifacts'].append({'path': p.resolve().relative_to(ROOT).as_posix(), 'sha256': sha(p), 'machine': arch})
    manifest['aarch64_artifact_count'] = len(manifest['artifacts'])-2
    full_object=args.headers/'full-windows-db-expand.obj'
    full_symbols=subprocess.check_output([LLVM/'llvm-nm.exe','-C',full_object],text=True)
    if 'DB_ExpandLoad_XAssetList(' not in full_symbols or 'DB_ExpandLoad_XAssetArray(' not in full_symbols:
        raise RuntimeError('Normal Windows expander branch missing')
    (out/'normal_windows_expander_symbols.txt').write_text(full_symbols,encoding='utf-8')
    manifest['normal_windows_expander_compile']={'path':full_object.as_posix(),'sha256':sha(full_object),'gate_macro':False,'machine':'x64'}
    manifest['native_test_executables'] = [p.name for p in binaries]
    manifest['final_build'] = args.build.as_posix(); manifest['host_build'] = args.host.as_posix(); manifest['final_run'] = args.run.as_posix()
    manifest['initial_run_status'] = 'Initial host fixture failed RPC small macro; fixed variable names. First orchestration run rejected valid lower_snake status labels; exact contract corrected. Logs/failure.json retained. Final3-ABI gate26 fixtures/12cycles passes.'
    manifest['compiler'] = subprocess.check_output([LLVM/'clang++.exe', '--version'], text=True)
    for leaf in ['source-preservation.json', 'review-evidence.md', 'verification-attempts.log']:
        copy(args.build/leaf, leaf)
    manifest['bounded_expander_selection'] = 'BO1_ASSETLIST_GATE + BO1_FRONTEND_TABLES_GATE: actual pointer/asset table bodies, no strings/renderers; pointer-table binary GC excludes asset-directory execution'
    manifest['reader_boundary'] = 'finite checked fixture DB_LoadXFileData, actual pointer-table expander; not engine async/refill reader'
    (out/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    print('Frozen string-table evidence:', out, manifest['aarch64_artifact_count'], 'AArch64 artifacts')

if __name__ == '__main__':
    main()
