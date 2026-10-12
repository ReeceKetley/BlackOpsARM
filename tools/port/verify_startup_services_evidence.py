"""Freeze verified startup-service and genuine same-process zone evidence."""
from pathlib import Path
import argparse
import gzip
import hashlib
import json

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


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
        raise RuntimeError('Use fresh worktree output')
    service = read(scratch / 'startup_services_root_final1/result.json')
    if not service['passed'] or service['full_com_init'] or len(service['runs']) != 6 or len(service['controls']) != 30:
        raise RuntimeError('Complete scoped service acceptance required')
    zone = read(scratch / 'startup_services_zone_host_run1/result.json')
    if not zone['passed'] or len(zone['runs']) != 2:
        raise RuntimeError('Integrated Windows zone acceptance missing')
    for row in zone['runs']:
        if not row['result']['actual_startup_services_integrated'] or row['result']['registered_asset_records'] != 284:
            raise RuntimeError('Actual initialized services and zone records required')
    for opt in ('O0', 'O3'):
        result = read(scratch / f'startup_services_zone_{opt}_run1/result.json')
        if not result['passed'] or not result['actual_startup_services_integrated'] or result['registered_asset_records'] != 284:
            raise RuntimeError('Integrated HONOR zone acceptance missing')
    current = [scratch / 'native_startup_services_zone_host1' / arch for arch in ('x86', 'x64')]
    current += [scratch / f'native_startup_services_zone_{opt}_final1' for opt in ('O0', 'O3')]
    for directory in current:
        if not read(directory / 'build-result.json')['completed']:
            raise RuntimeError('Integrated build failed')
        for path, sha in read(directory / 'source-manifest.json').items():
            if digest(ROOT / path) != sha.lower():
                raise RuntimeError('Integrated build no longer current: ' + path)
    for name in ('startup_crt_honor3/result.json', 'startup_dvar_layout1/result.json',
                 'startup_string_pointer1/result.json', 'startup_services_source_final1.json',
                 'startup_services_protected1.json'):
        if read(scratch / name)['passed'] is not True:
            raise RuntimeError('Required prerequisite evidence failed: ' + name)
    normal = scratch / 'startup_services_windows_full1'
    for arch in ('x86', 'x64'):
        if not read(normal / arch / 'build-result.json')['completed']:
            raise RuntimeError('Normal Windows compatibility build failed')
        for path, sha in read(normal / arch / 'source-manifest.json').items():
            if digest(ROOT / path) != sha.lower():
                raise RuntimeError('Normal Windows production input changed')
    result = dict(passed=True, baseline='8f81f9c', current_integrated_builds=4,
                  service_runs=6, unavailable_controls=30, integrated_zone_runs=4,
                  actual_com_init_completed=False, protected_paths=19930,
                  scope='Genuine integrated services and MP71; actual Com_Init is the next execution target.')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    if args.archive:
        archive = ROOT / 'docs/port/evidence/arm64_native_startup_services'
        if archive.exists():
            raise RuntimeError('Never overwrite frozen evidence')
        sources = {output}
        for pattern in ('startup_crt_*', 'startup_dvar_layout*', 'startup_string_pointer*',
                        'startup_services_*', 'startup_common_windows*', 'native_startup_services_*',
                        'native_com_header_probe.log'):
            for path in scratch.glob(pattern):
                if path.is_dir():
                    sources.update(row for row in path.rglob('*') if row.is_file() and row.suffix in ('.json', '.txt', '.log'))
                elif path.suffix in ('.json', '.txt', '.log'):
                    sources.add(path)
        archive.mkdir(parents=True)
        records = []
        for source in sorted(sources):
            raw = source.read_bytes()
            compressed = source.suffix == '.log' or len(raw) > 65536
            stored = gzip.compress(raw, mtime=0) if compressed else raw
            name = '__'.join(source.relative_to(scratch).parts) + ('.gz' if compressed else '')
            (archive / name).write_bytes(stored)
            records.append(dict(source=source.relative_to(ROOT).as_posix(), archive=name,
                sha256=hashlib.sha256(stored).hexdigest(), bytes=len(stored),
                original_sha256=hashlib.sha256(raw).hexdigest(), original_bytes=len(raw), gzip=compressed))
        (archive / 'inventory.json').write_text(json.dumps(dict(schema='BlackOpsARM.TASK46.archive.v1', files=records), indent=2) + '\n', encoding='utf-8')
        print('Frozen', len(records), 'startup-service metadata records')


if __name__ == '__main__':
    main()
