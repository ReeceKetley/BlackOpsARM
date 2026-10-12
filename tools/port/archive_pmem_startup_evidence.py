"""Freeze TASK42 metadata/log evidence, including the original x64 failure."""
from pathlib import Path
import gzip
import hashlib
import json


def main():
    root = Path(__file__).resolve().parents[2]
    scratch = root / 'build-android-audit'
    archive = root / 'docs/port/evidence/arm64_pmem_owner_stack'
    if archive.exists():
        raise RuntimeError('Never overwrite frozen evidence')
    verified = scratch / 'pmem_startup_verified1.json'
    if not json.loads(verified.read_text())['passed']:
        raise RuntimeError('Final source/runtime/preservation verification required')
    sources = {verified}
    for name in ('pmem_owner_stack_before1', 'pmem_owner_stack_after1',
                 'native_zone_file_multiowner1', 'native_zone_file_multiowner2',
                 'native_startup_zone_pmem1', 'native_zone_pmem_normal1',
                 'native_startup_zone_pmem_O0_1', 'native_startup_zone_pmem_O3_1',
                 'native_startup_zone_pmem_O0_run1', 'native_startup_zone_pmem_O3_run1'):
        sources.update(path for path in (scratch / name).rglob('*')
                       if path.is_file() and path.suffix in ('.json', '.txt', '.log'))
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
        records.append({'source': source.relative_to(root).as_posix(), 'archive': name,
                        'sha256': hashlib.sha256(stored).hexdigest(), 'bytes': len(stored),
                        'original_sha256': hashlib.sha256(original).hexdigest(),
                        'original_bytes': len(original), 'gzip': compressed})
    (archive / 'inventory.json').write_text(json.dumps({'schema': 'BlackOpsARM.TASK42.archive.v1', 'files': records}, indent=2) + '\n', encoding='utf-8')
    print(f'Archived {len(records)} TASK42 metadata/log records')


if __name__ == '__main__':
    main()
