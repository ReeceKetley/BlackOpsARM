"""Verify/freeze bounded native protocol and integrated startup regression evidence."""
from pathlib import Path
import argparse
import gzip
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    parser.add_argument('--archive', action='store_true')
    args = parser.parse_args()
    scratch = ROOT / 'build-android-audit'
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use fresh worktree verification output')
    if not read(scratch / 'native_fcs_source1.json')['passed'] or not read(scratch / 'native_fcs_root_run1/result.json')['passed']:
        raise RuntimeError('Final source/runtime evidence required')
    manifest_dirs = []
    for name in ('native_fcs_impl_host4', 'native_fcs_normal1', 'native_thread_data_fcs_host1', 'native_thread_zone_fcs_host1'):
        manifest_dirs.extend(scratch / name / arch for arch in ('x86', 'x64'))
    manifest_dirs.extend(scratch / name for name in ('native_fcs_impl_O0_3', 'native_fcs_impl_O3_3',
                                                    'native_thread_zone_fcs_O0_1', 'native_thread_zone_fcs_O3_1'))
    for directory in manifest_dirs:
        if not read(directory / 'build-result.json')['completed']:
            raise RuntimeError('Final build failed')
        if any(digest(ROOT / path) != sha.lower() for path, sha in read(directory / 'source-manifest.json').items()):
            raise RuntimeError('Build input no longer current: ' + str(directory))
    for arch in ('x86', 'x64'):
        directory = scratch / 'native_thread_zone_fcs_host1' / arch
        if read(directory / 'root-run-result.json')['exit'] != 0 or not read(directory / 'root-collector-result.json')['actual_thread_data_integrated']:
            raise RuntimeError('Integrated Windows TLS/MP71 regression failed')
    for opt in ('O0', 'O3'):
        if not read(scratch / f'native_thread_zone_fcs_{opt}_run1/result.json')['actual_thread_data_integrated']:
            raise RuntimeError('Integrated HONOR TLS/MP71 regression failed')
    protected = read(scratch / 'default_resolution_baseline/hashes.json')
    if any(digest(path) != sha.lower() for path, sha in protected.items()):
        raise RuntimeError('Protected file changed')
    reference_git = read(scratch / 'default_resolution_baseline/git.json')
    for name, expected in reference_git.items():
        actual = {key: subprocess.check_output(['git', '-C', str(ROOT.parent / name)] + command, text=True)
                  for key, command in [('head', ['rev-parse', 'HEAD']), ('branch', ['branch', '--show-current']), ('status', ['status', '--short'])]}
        if actual != expected:
            raise RuntimeError('Protected reference Git changed')
    archives = {}
    for name in ('arm64_default_resolution', 'arm64_startup_reader', 'arm64_native_startup_zone',
                 'arm64_pmem_owner_stack', 'arm64_native_critical_sections', 'arm64_native_thread_data'):
        directory = ROOT / 'docs/port/evidence' / name
        rows = read(directory / 'inventory.json')['files']
        if any(digest(directory / row['archive']) != row['sha256'] for row in rows):
            raise RuntimeError('Frozen archive changed')
        archives[name] = len(rows)
    tsan = read(scratch / 'native_fcs_tsan_run1/result.json')
    control = read(scratch / 'native_fcs_tsan_control1/result.json')
    # Preserve this sanitizer limitation; it is deliberately NOT passing race evidence.
    diagnostic = 'tsan_rtl.cpp:1043'
    if tsan['passed'] or tsan['exit'] != 139 or diagnostic not in tsan['diagnostic']:
        raise RuntimeError('Recorded sanitizer limitation differs')
    if tsan['runtime_sha256'] != control['runtime_sha256'] or any(row['exit'] != 139 or diagnostic not in row['stderr']
                or 'SANITIZER_CONTROL_MAIN_ENTER' not in row['stdout'] for row in control['control_runs']):
        raise RuntimeError('Independent no-engine sanitizer controls differ')
    result = {'passed': True, 'baseline': 'cfbd591', 'protected_paths': len(protected),
              'reference_git_unchanged': True, 'frozen_archive_records': archives,
              'current_build_manifests': len(manifest_dirs), 'original_windows_protocol_oracle': True,
              'tsan_passed': False, 'no_engine_controls_reproduce_tsan_runtime_check': True,
              'scope': 'Actual writer/releases and fixed32 admission contract; not Dvar startup, fairness or Com_Error recovery.'}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    if args.archive:
        archive = ROOT / 'docs/port/evidence/arm64_fast_critical_section'
        if archive.exists():
            raise RuntimeError('Never overwrite frozen evidence')
        sources = {output}
        for pattern in ('fast_critical_section_original*', 'native_fcs_impl_*', 'native_fcs_normal*',
                        'native_fcs_source*.json', 'native_fcs_root_*', 'native_fcs_tsan_*',
                        'native_thread_data_fcs_*', 'native_thread_zone_fcs_*'):
            for path in scratch.glob(pattern):
                if path.is_dir():
                    sources.update(row for row in path.rglob('*') if row.is_file() and row.suffix in ('.json', '.txt', '.log'))
                elif path.suffix == '.json':
                    sources.add(path)
        archive.mkdir(parents=True)
        records = []
        for source in sorted(sources):
            original = source.read_bytes()
            name = '__'.join(source.relative_to(scratch).parts)
            compressed = source.suffix == '.log' or len(original) > 65536
            stored = gzip.compress(original, mtime=0) if compressed else original
            if compressed:
                name += '.gz'
            (archive / name).write_bytes(stored)
            records.append({'source': source.relative_to(ROOT).as_posix(), 'archive': name,
                            'sha256': hashlib.sha256(stored).hexdigest(), 'bytes': len(stored),
                            'original_sha256': hashlib.sha256(original).hexdigest(),
                            'original_bytes': len(original), 'gzip': compressed})
        (archive / 'inventory.json').write_text(json.dumps({'schema': 'BlackOpsARM.TASK45.archive.v1', 'files': records}, indent=2) + '\n', encoding='utf-8')
        print(f'Frozen {len(records)} protocol/oracle/integration/sanitizer metadata records')
    print('Current native protocol/integration/source/protected evidence pass; TSan limitation retained')


if __name__ == '__main__':
    main()
