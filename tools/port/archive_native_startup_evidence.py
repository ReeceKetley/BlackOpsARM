"""Freeze complete-zone logs/metadata without assets, decoded data or binaries."""
from pathlib import Path
import gzip
import hashlib
import json


def main():
    root = Path(__file__).resolve().parents[2]
    scratch = root / 'build-android-audit'
    archive = root / 'docs/port/evidence/arm64_native_startup_zone'
    if archive.exists():
        raise RuntimeError('Never overwrite a frozen archive')
    sources = set()
    for pattern in ('native_startup_zone_host_*/x86', 'native_startup_zone_host_*/x64',
                    'native_startup_zone_arm_*', 'native_startup_zone_O*_run*',
                    'native_zone_load_normal_probe*/x86', 'native_zone_load_normal_probe*/x64'):
        for path in scratch.glob(pattern):
            if path.is_dir():
                sources.update(p for p in path.iterdir() if p.is_file() and p.suffix in ('.log', '.json', '.txt'))
    for pattern in ('native_zone_load_source*.json', 'native_startup_protected_final*.json',
                    'startup_zone_oracle*run2.json'):
        sources.update(scratch.glob(pattern))
    final = scratch / 'native_startup_protected_final2.json'
    if not json.loads(final.read_text())['passed']:
        raise RuntimeError('Final protection must pass first')
    archive.mkdir(parents=True)
    records = []
    for source in sorted(sources):
        original = source.read_bytes()
        name = '__'.join(source.relative_to(scratch).parts)
        compressed = source.suffix == '.log' or len(original) > 65536
        stored = gzip.compress(original, mtime=0) if compressed else original
        if compressed:
            name += '.gz'
        target = archive / name
        if target.exists():
            raise RuntimeError('Archive name collision')
        target.write_bytes(stored)
        records.append({'source': source.relative_to(root).as_posix(), 'archive': name,
                        'sha256': hashlib.sha256(stored).hexdigest(), 'bytes': len(stored),
                        'original_sha256': hashlib.sha256(original).hexdigest(),
                        'original_bytes': len(original), 'gzip': compressed})
    (archive / 'inventory.json').write_text(json.dumps({'schema': 'BlackOpsARM.TASK41.archive.v1', 'files': records}, indent=2) + '\n', encoding='utf-8')
    print(f'Archived {len(records)} complete-zone metadata/log records')


if __name__ == '__main__':
    main()
