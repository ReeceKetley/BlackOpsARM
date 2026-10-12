"""Check current native mutex service evidence and freeze metadata only."""
from pathlib import Path
import argparse
import gzip
import hashlib
import json
import subprocess

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
    modified = subprocess.check_output(['git', 'diff', '7d1aaa6', '--name-only', '--', 'src'], cwd=ROOT, text=True).splitlines()
    if modified != ['src/platform/android/script_pool_bridge.cpp']:
        raise RuntimeError('Unexpected production source delta')
    for path in ('src/platform/android/script_string_services.cpp', 'src/platform/android/script_string_services.h',
                 'src/universal/critical_sections.h', 'src/win32/win_common.cpp'):
        before = subprocess.check_output(['git', 'show', '7d1aaa6:' + path], cwd=ROOT).replace(b'\r\n', b'\n')
        if (ROOT / path).read_bytes().replace(b'\r\n', b'\n') != before:
            raise RuntimeError('Pool service/enum/Windows implementation changed')
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
    for name in ('arm64_default_resolution', 'arm64_startup_reader', 'arm64_native_startup_zone', 'arm64_pmem_owner_stack'):
        directory = ROOT / 'docs/port/evidence' / name
        rows = json.loads((directory / 'inventory.json').read_text())['files']
        if any(digest(directory / row['archive']) != row['sha256'] for row in rows):
            raise RuntimeError('Frozen archive changed')
        archives[name] = len(rows)
    manifest_dirs = []
    for name in ('native_critical_sections_host2', 'native_startup_zone_critical1', 'default_resolution_critical_host1'):
        manifest_dirs.extend(scratch / name / arch for arch in ('x86', 'x64'))
    for name in ('native_startup_zone_critical_O0_1', 'native_startup_zone_critical_O3_1',
                 'default_resolution_critical_O0_1', 'default_resolution_critical_O3_1', 'native_critical_sections_run2'):
        manifest_dirs.append(scratch / name)
    for directory in manifest_dirs:
        inputs = json.loads((directory / 'source-manifest.json').read_text(encoding='utf-8-sig'))
        if any(digest(ROOT / path) != sha.lower() for path, sha in inputs.items()):
            raise RuntimeError('Build/runtime source no longer current: ' + str(directory))
    if not json.loads((scratch / 'native_critical_sections_run2/result.json').read_text())['passed']:
        raise RuntimeError('Isolated mutex checks failed')
    for arch in ('x86', 'x64'):
        if not json.loads((scratch / 'native_startup_zone_critical1' / arch / 'collector-result.json').read_text())['passed']:
            raise RuntimeError('Host MP71 collector failed')
    for opt in ('O0', 'O3'):
        if not json.loads((scratch / f'native_startup_zone_critical_{opt}_run1/result.json').read_text())['passed']:
            raise RuntimeError('HONOR MP71 collector failed')
        if json.loads((scratch / f'default_resolution_critical_{opt}_run1/results.json').read_text())['status'] != 'passed':
            raise RuntimeError('Real SL/MT/default ownership gate failed')
    result = {'passed': True, 'baseline': '7d1aaa6', 'production_delta': modified,
              'protected_paths': len(protected), 'reference_git_unchanged': True,
              'frozen_archive_records': archives, 'current_source_manifests': len(manifest_dirs),
              'scope': '75 native recursive critical sections; preserved pool locks and MP71/default ownership. No TLS/Com_Init claim.'}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    if args.archive:
        archive = ROOT / 'docs/port/evidence/arm64_native_critical_sections'
        if archive.exists():
            raise RuntimeError('Never overwrite frozen evidence')
        sources = {output}
        for pattern in ('native_critical_sections_host*', 'native_critical_sections_run*',
                        'native_startup_zone_critical*', 'default_resolution_critical*'):
            for directory in scratch.glob(pattern):
                sources.update(path for path in directory.rglob('*') if path.is_file()
                               and path.suffix in ('.json', '.txt', '.log'))
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
        (archive / 'inventory.json').write_text(json.dumps({'schema': 'BlackOpsARM.TASK43.archive.v1', 'files': records}, indent=2) + '\n', encoding='utf-8')
        print(f'Frozen {len(records)} mutex/startup/default metadata records')
    print('Current service/runtime/source evidence and protected references pass')


if __name__ == '__main__':
    main()
