"""Freeze TASK-0011 measurements, diagnostics and artifact/source provenance."""
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


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--build', type=Path, required=True)
    ap.add_argument('--host', type=Path, required=True)
    ap.add_argument('--measurements', type=Path, required=True)
    ap.add_argument('--initial-measurements', type=Path, required=True)
    ap.add_argument('--zone-regression', type=Path, required=True)
    ap.add_argument('--narrow-regression', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    for path in vars(args).values():
        if not path.resolve().is_relative_to(ROOT):
            raise RuntimeError('Only Android worktree paths allowed')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    manifest = {'schema': 'BlackOpsARM.TASK-0011.evidence.v1', 'files': {}, 'passes': {}, 'artifacts': [], 'source_sha256': {}}
    def copy(src, name):
        shutil.copyfile(src, out/name)
        manifest['files'][name] = {'source': src.resolve().relative_to(ROOT).as_posix(), 'sha256': sha(src)}
    for build in sorted(args.build.parent.glob('checked_zone_pass*')):
        if not (build/'build.log').is_file():
            continue
        for leaf in ['configure.log', 'build.log', 'compile_commands.json']:
            copy(build/leaf, build.name+'.'+leaf)
        text = (build/'build.log').read_text(encoding='utf-8-sig')
        manifest['passes'][build.name] = {'compiler_errors': len(re.findall(r'^.+?:\d+:\d+: (?:fatal )?error:',text,re.M)),
            'linker_errors': len(re.findall(r'^ld.lld: error:',text,re.M)),
            'warnings': len(re.findall(r'^.+?:\d+:\d+: warning:',text,re.M)),
            'warning_classes': dict(collections.Counter(re.findall(r'\[(-W[^\]]+)\]', text)))}
    for build in sorted(args.host.parent.glob('checked_zone_host_pass*')):
        for arch in ['x86', 'x64']:
            for leaf in ['compile.log', 'command.txt', 'fixtures.log']:
                if (build/arch/leaf).is_file():
                    copy(build/arch/leaf, build.name+'.'+arch+'.'+leaf)
    for label, directory in [('final', args.measurements), ('initial', args.initial_measurements)]:
        for leaf in ['measurements.json', 'invocations.json']:
            copy(directory/leaf, label+'.'+leaf)
        # Full raw timing/memory samples are in measurements.json. Keep fixtures,
        # terminal failures, provenance and device context separately; original
        # per-process logs/derived 36-byte scratch headers remain local intact.
        if label == 'final':
            for path in sorted(directory.glob('*.log')):
                if any(key in path.name for key in ['fixtures', 'genuine-validation', 'backend-failure', 'device-metadata', 'device-sha', 'header-sha']) and path.stat().st_size:
                    copy(path, 'final.'+path.name)
    for prefix, directory in [('zone', args.zone_regression), ('narrow', args.narrow_regression)]:
        for path in sorted(directory.rglob('*')):
            if path.is_file() and (path.suffix == '.log' or path.name == 'command.txt'):
                copy(path, prefix+'.'+path.relative_to(directory).as_posix().replace('/', '.'))
    for leaf in ['device-tests.log', 'apk-recheck.log', 'diff-check.log']:
        copy(args.build/leaf, 'final.'+leaf)
    llvm = Path('D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin')
    binaries = sorted(p for p in args.build.glob('bo1_*tests') if p.is_file())
    headers = []
    for p in sorted(args.build.rglob('*.o'))+binaries:
        data = p.read_bytes()
        if data[:6] != b'\x7fELF\x02\x01' or struct.unpack_from('<H',data,18)[0] != 183:
            raise RuntimeError('Non-AArch64 object: '+str(p))
        manifest['artifacts'].append({'path': p.resolve().relative_to(ROOT).as_posix(), 'sha256': sha(p), 'machine': 'AArch64'})
        headers += [str(p), subprocess.check_output([llvm/'llvm-readelf.exe','-h',p],text=True)]
    (out/'aarch64_elf_headers.txt').write_text('\n'.join(headers))
    symbols = subprocess.check_output([llvm/'llvm-nm.exe','-C',args.build/'bo1_checked_zone_tests'],text=True)
    forbidden = ['DB_InitStreams(', 'Load_XAssetListCustom(', 'DB_InitZone(']
    if any(s in symbols for s in forbidden):
        raise RuntimeError('New allocation gate links a later loading boundary')
    (out/'checked_zone_symbols.txt').write_text(symbols)
    manifest['later_loading_symbols_absent'] = forbidden
    commands = json.loads((args.build/'compile_commands.json').read_text())
    for c in commands:
        if '-DBO1_PORT_X64' in c['command']:
            raise RuntimeError('Blanket legacy flag on Android')
        p = Path(c['file']).resolve()
        if p.is_relative_to(ROOT) and p.is_file():
            manifest['source_sha256'][p.relative_to(ROOT).as_posix()] = sha(p)
    for p in [ROOT/'src/platform/fastfile_zone.h',ROOT/'src/platform/android/zone_memory.h',
              ROOT/'src/universal/physicalmemory.cpp',ROOT/'tools/port/measure_checked_zone.py',
              ROOT/'tools/port/build_checked_zone_host.ps1',ROOT/'tools/port/collect_checked_zone_evidence.py']:
        manifest['source_sha256'][p.relative_to(ROOT).as_posix()] = sha(p)
    for arch, machine in [('x86',0x14c),('x64',0x8664)]:
        p=args.host/arch/'checked_zone.exe';data=p.read_bytes();pe=struct.unpack_from('<I',data,0x3c)[0]
        if data[pe:pe+4] != b'PE\0\0' or struct.unpack_from('<H',data,pe+4)[0] != machine:
            raise RuntimeError('Unexpected Windows reference ABI')
        manifest['artifacts'].append({'path': p.resolve().relative_to(ROOT).as_posix(), 'sha256': sha(p), 'machine': arch})
    manifest['aarch64_artifact_count'] = len(headers)//2
    manifest['native_test_executables'] = [p.name for p in binaries]
    manifest['compiler'] = subprocess.check_output([llvm/'clang++.exe','--version'],text=True)
    manifest['initial_measurement_status'] = 'Superseded: sampler reset race, unrelated debug arena, ambiguous Windows raw baseline; raw first attempt preserved'
    manifest['final_build'] = args.build.as_posix(); manifest['host_build'] = args.host.as_posix()
    (out/'compiler_summary.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('Frozen checked-zone evidence:',out,'AArch64 artifacts:',manifest['aarch64_artifact_count'])


if __name__ == '__main__':
    main()
