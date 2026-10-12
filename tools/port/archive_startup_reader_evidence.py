"""Freeze reader prerequisite logs/metadata, excluding binaries and game bytes."""
from pathlib import Path
import gzip
import hashlib
import json


def main():
    root = Path(__file__).resolve().parents[2]
    scratch = root / 'build-android-audit'
    archive = root / 'docs/port/evidence/arm64_startup_reader'
    if archive.exists():
        raise RuntimeError('Evidence archive already exists; never overwrite')
    sources = set()
    for pattern in ('native_zone_file_host_*/x86', 'native_zone_file_host_*/x64',
                    'native_zone_file_arm_*', 'startup_codec_O*_run1',
                    'host-inflate-20261012-133349194/x86', 'host-inflate-20261012-133349194/x64'):
        for path in scratch.glob(pattern):
            if path.is_dir():
                sources.update(p for p in path.iterdir() if p.is_file() and p.suffix in ('.log', '.json', '.txt'))
            elif path.suffix in ('.log', '.json'):
                sources.add(path)
    for name in ('startup_zone_oracle_run1.json', 'startup_zone_oracle_controls_run1.json',
                 'startup_reader_protected_run1.json', 'startup_codec_arm_O0_build.log',
                 'startup_codec_arm_O3_build.log'):
        sources.add(scratch / name)
    for name in ('native_zone_file_final_O0_run1', 'native_zone_file_final_O3_run1'):
        sources.update(p for p in (scratch / name).iterdir() if p.suffix in ('.log', '.json'))
    if not json.loads((scratch / 'startup_reader_protected_run1.json').read_text())['passed']:
        raise RuntimeError('Protection verification must pass first')
    archive.mkdir(parents=True)
    records = []
    for source in sorted(sources):
        original = source.read_bytes()
        relative = source.relative_to(root).as_posix()
        name = '__'.join(source.relative_to(scratch).parts)
        compressed = source.suffix == '.log' or len(original) > 65536
        data = gzip.compress(original, mtime=0) if compressed else original
        if compressed:
            name += '.gz'
        target = archive / name
        if target.exists():
            raise RuntimeError('Archive name collision')
        target.write_bytes(data)
        records.append({'source': relative, 'archive': name,
                        'sha256': hashlib.sha256(data).hexdigest(), 'bytes': len(data),
                        'original_sha256': hashlib.sha256(original).hexdigest(),
                        'original_bytes': len(original), 'gzip': compressed})
    inventory = {'schema': 'BlackOpsARM.startup-reader-prerequisite.archive.v1',
                 'scope': 'Reader ownership/codec prerequisite, not complete MP dispatch or startup',
                 'files': records}
    (archive / 'inventory.json').write_text(json.dumps(inventory, indent=2) + '\n', encoding='utf-8')
    print(f'Archived {len(records)} reader metadata/log records; no game assets or binaries')


if __name__ == '__main__':
    main()
