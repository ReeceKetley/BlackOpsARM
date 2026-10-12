"""Verify final authentic thread/zone evidence and freeze metadata only."""
from pathlib import Path
import argparse
import gzip
import hashlib
import json
import subprocess
from check_startup_zone_runtime import validate
from startup_zone_oracle import inspect

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    parser.add_argument('--archive', action='store_true')
    args = parser.parse_args()
    scratch = ROOT / 'build-android-audit'
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use fresh worktree verification output')
    if not json.loads((scratch / 'native_thread_data_source_final1.json').read_text())['passed']:
        raise RuntimeError('Canonical source proof required')
    for name in ('native_thread_data_root_final1', 'thread_data_original_oracle_run3'):
        if not json.loads((scratch / name / 'result.json').read_text())['passed']:
            raise RuntimeError('Final independent runtime/original layout required')
    manifest_dirs = []
    for name in ('native_thread_data_impl_host3', 'native_thread_zone_impl_host1', 'native_thread_data_normal1'):
        manifest_dirs.extend(scratch / name / arch for arch in ('x86', 'x64'))
    manifest_dirs.extend(scratch / name for name in ('native_thread_data_impl_O0_3', 'native_thread_data_impl_O3_3',
                                                    'native_thread_zone_impl_O0_1', 'native_thread_zone_impl_O3_1'))
    for directory in manifest_dirs:
        if not json.loads((directory / 'build-result.json').read_text(encoding='utf-8-sig'))['completed']:
            raise RuntimeError('Final build failed')
        inputs = json.loads((directory / 'source-manifest.json').read_text(encoding='utf-8-sig'))
        if any(digest(ROOT / path) != sha.lower() for path, sha in inputs.items()):
            raise RuntimeError('Build input no longer current: ' + str(directory))
    for arch in ('x86', 'x64'):
        directory = scratch / 'native_thread_zone_impl_host1' / arch
        if json.loads((directory / 'root-run-result.json').read_text(encoding='utf-8-sig'))['exit'] != 0:
            raise RuntimeError('Integrated host runtime failed')
        if not json.loads((directory / 'root-collector-result.json').read_text())['actual_thread_data_integrated']:
            raise RuntimeError('Integrated host collector failed')
    for opt in ('O0', 'O3'):
        if not json.loads((scratch / f'native_thread_zone_root_{opt}_run1/result.json').read_text())['actual_thread_data_integrated']:
            raise RuntimeError('Integrated HONOR collector failed')
        commands = json.loads((scratch / f'native_thread_zone_impl_{opt}_1/compile_commands.json').read_text())
        hunk = [row for row in commands if Path(row['file']).name == 'hunk_services.cpp' and
                'bo1_android_hunk_services.dir' in row['command'].replace('\\', '/')]
        competing = [row for row in commands if Path(row['file']).name == 'hunk_services.cpp' and
                     'bo1_native_thread_data.dir' in row['command'].replace('\\', '/')]
        if len(hunk) != 1 or competing:
            raise RuntimeError('Integrated hunk-owner target duplicated/missing')
    protected = json.loads((scratch / 'default_resolution_baseline/hashes.json').read_text(encoding='utf-8-sig'))
    if any(digest(path) != sha.lower() for path, sha in protected.items()):
        raise RuntimeError('Protected file changed')
    reference_git = json.loads((scratch / 'default_resolution_baseline/git.json').read_text(encoding='utf-8-sig'))
    for name, expected in reference_git.items():
        actual = {key: subprocess.check_output(['git', '-C', str(ROOT.parent / name)] + command, text=True)
                  for key, command in [('head', ['rev-parse', 'HEAD']), ('branch', ['branch', '--show-current']), ('status', ['status', '--short'])]}
        if actual != expected:
            raise RuntimeError('Protected reference Git changed')
    archives = {}
    for name in ('arm64_default_resolution', 'arm64_startup_reader', 'arm64_native_startup_zone',
                 'arm64_pmem_owner_stack', 'arm64_native_critical_sections'):
        directory = ROOT / 'docs/port/evidence' / name
        rows = json.loads((directory / 'inventory.json').read_text())['files']
        if any(digest(directory / row['archive']) != row['sha256'] for row in rows):
            raise RuntimeError('Frozen archive changed')
        archives[name] = len(rows)
    # Collector must reject altered scope/types before accepting the MP asset rows.
    lines = (scratch / 'native_thread_zone_impl_host1/x64/root-run.log').read_text(encoding='utf-8-sig').splitlines()
    oracle = inspect('D:/Games/Call of Duty - Black Ops/zone/Common/code_pre_gfx_mp.ff')
    rejected = 0
    for field, value in [('pointer_bytes', 4), ('thread_context', True), ('canonical_slots', True),
                         ('actual_cmd_reset', 1), ('actual_cmd_reset', False), ('com_init_executed', True), ('extra', 0)]:
        marker = json.loads(lines[0])
        marker['startup_thread_integration'][field] = value
        changed = '\n'.join([json.dumps(marker)] + lines[1:])
        try:
            validate(changed, 8, oracle, True)
        except (ValueError, KeyError, IndexError):
            rejected += 1
        else:
            raise RuntimeError('Collector accepted altered scope/type')
    result = {'passed': True, 'baseline': 'f117f5d', 'protected_paths': len(protected),
              'reference_git_unchanged': True, 'frozen_archive_records': archives,
              'current_build_manifests': len(manifest_dirs), 'integration_collector_rejections': rejected,
              'single_hunk_owner_target': True,
              'scope': 'Original thread data and CmdReset plus MP71 in same native process. Controlled Dvar inputs; no Com_Init/DB worker/gameplay.'}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    if args.archive:
        archive = ROOT / 'docs/port/evidence/arm64_native_thread_data'
        if archive.exists():
            raise RuntimeError('Never overwrite frozen evidence')
        sources = {output}
        for pattern in ('native_thread_data_impl_*', 'native_thread_data_root_*', 'native_thread_data_normal*',
                        'native_thread_data_source*.json', 'thread_data_original_oracle*',
                        'native_thread_zone_impl_*', 'native_thread_zone_root_*'):
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
        (archive / 'inventory.json').write_text(json.dumps({'schema': 'BlackOpsARM.TASK44.archive.v1', 'files': records}, indent=2) + '\n', encoding='utf-8')
        print(f'Frozen {len(records)} original-layout/thread/startup metadata records')
    print('Final native thread integration, source/current manifests and protected evidence pass')


if __name__ == '__main__':
    main()
