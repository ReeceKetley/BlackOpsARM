"""Genuine second-directory RawFile numeric header; no game text or payload.

The preceding first-StringTable oracle supplies independently verified metadata.
One additional full streaming provenance pass retains only the required prefix.
Zone allocation metadata is reused unchanged, never inferred from payload size.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.dont_write_bytecode = True
from stringtable_payload_oracle import _stream_prefix

ROOT = Path(__file__).resolve().parents[2]
CODE_SHA = '565f038d4a5a10912bda5741e80f31805f9c7c76fc18296f2a8591d58f1ee41f'
ZOMBIE_SHA = '70686402a689d5c575f18d2d76e6cec9f9a95ff70446372960f1b5c7f41d92e0'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def verify_directory(prefix, previous):
    start = previous['directory_source_start']
    end = start + previous['directory_disk_bytes']
    if end > len(prefix) or sha(memoryview(prefix)[start:end]) != previous['directory_sha256']:
        raise RuntimeError('Genuine directory bytes differ from preceding frozen metadata')
    for index in (0, 1):
        actual = struct.unpack_from('<iI', prefix, start + index * 8)
        record = previous['directory_records'][index]
        if actual != (record['type'], record['tag']):
            raise RuntimeError('Next asset type/tag lacks genuine decoded directory provenance')


def reference(source, previous):
    """Extend frozen first-asset metadata by the actual 12-byte second header.

    Zombie's actual next type7 is explicitly deferred at its existing endpoint;
    this function neither searches ahead for RawFile nor silently skips assets.
    """
    records = previous['directory_records']
    if (len(records) < 2 or records[0]['index'] != 0
            or records[0]['type'] != 37 or records[0]['tag'] != 0xffffffff
            or records[1]['index'] != 1 or records[1]['tag'] != 0xffffffff):
        raise RuntimeError('Preceding genuine first asset/next directory entry differs')
    start = previous['stringtable_indices']['source_end']
    if previous['prefix_bytes'] != start:
        raise RuntimeError('Previous oracle must end exactly after the first asset indices')
    raw_sha = previous['raw_sha256']
    if raw_sha == ZOMBIE_SHA:
        if start != 65678 or records[1]['type'] != 7:
            raise RuntimeError('Genuine zombie next boundary differs from TechniqueSet')
        prefix = _stream_prefix(source, previous, start)
        verify_directory(prefix, previous)
        return {
            'schema': 'BlackOpsARM.rawfile-header-oracle.v1',
            'status': 'deferred', 'reason': 'Actual next directory asset is TechniqueSet type7',
            'original_path': str(Path(source).resolve()), 'raw_sha256': raw_sha,
            'decoded_sha256': previous['decoded_sha256'],
            'decoded_bytes': previous['decoded_bytes'], 'decoded_adler32': previous['decoded_adler32'],
            'next_directory_record': dict(records[1]), 'decoded_source_end': start,
            'prefix_bytes': start, 'prefix_sha256': sha(prefix),
            'header_extension_bytes': 0, 'no_next_asset_skipped': True,
            'scope': 'Complete original provenance verified; no TechniqueSet header or later asset parsed',
        }
    if raw_sha != CODE_SHA or start != 10407 or records[1]['type'] != 36:
        raise RuntimeError('Unsupported genuine second asset; no source-boundary guessing')
    end = start + 12
    if end > previous['decoded_bytes']:
        raise RuntimeError('Genuine RawFile header exceeds decoded source')
    prefix = _stream_prefix(source, previous, end)
    verify_directory(prefix, previous)
    name, length, buffer = struct.unpack_from('<IiI', prefix, start)
    # These are the exact inspected genuine values. Any changed input requires
    # a new audit rather than broadening the first supported boundary silently.
    if (name, length, buffer) != (0xffffffff, 140, 0xffffffff):
        raise RuntimeError('Genuine code_pre_gfx RawFile header fields differ')
    result = dict(previous)
    result.update({
        'schema': 'BlackOpsARM.rawfile-header-oracle.v1', 'status': 'supported',
        'original_path': str(Path(source).resolve()),
        'previous_prefix_bytes': start, 'previous_prefix_sha256': previous['prefix_sha256'],
        'prefix_bytes': end, 'prefix_sha256': sha(prefix),
        'execution_prefix_bytes': end, 'execution_prefix_sha256': sha(prefix),
        'retained_decoded_buffer_limit': end, 'header_extension_bytes': 12,
        'decoded_source_end': end, 'source_consumed_bytes': end - 52,
        'next_directory_record': dict(records[1]),
        'rawfile_source_start': start, 'rawfile_source_end': end,
        'rawfile_header': {'name_tag': name, 'length': length, 'buffer_tag': buffer},
        'rawfile_header_sha256': sha(memoryview(prefix)[start:end]),
        'rawfile_flags_field_present': False,
        'rawfile_disk_bytes': 12, 'rawfile_native64_bytes': 24,
        'rawfile_disk_field_offsets': {'name': 0, 'length': 4, 'buffer': 8},
        'rawfile_native64_field_offsets': {'name': 0, 'length': 8, 'buffer': 16},
        'rawfile_disk_alignment_request_mask': 3, 'rawfile_native64_alignment_bytes': 8,
        'rawfile_native64_padding_offsets': [12, 13, 14, 15],
        'rawfile_future_buffer_bytes': length + 1,
        'rawfile_future_buffer_length_origin': 'Actual Load_RawFile requests signed len+1, no flags field',
        'rawfile_tag_contract': {
            'asset': 'inline -1; no inserted-pointer reservation',
            'name': 'inline -1; not loaded at this header-only boundary',
            'buffer': 'nonzero presence tag; actual Load_RawFile unconditionally allocates len+1 bytes',
            'native64_special_tag_value': 0xffffffffffffffff,
        },
        'rawfile_children_unread': True, 'no_next_asset_skipped': True,
        'rawfile_block4_disk_cursor_unchanged': previous['stringtable_indices']['disk_end'],
        'rawfile_block4_native64_cursor_unchanged': previous['stringtable_indices']['native64_end'],
        'temp_cursor_scope': 'Field offsets are relative to the allocated TEMP header; absolute native shadow after rewind is a separate production stream-lifetime contract',
        'allocation_metadata_provenance': 'Unchanged genuine preceding xfile_fields; native header sizeof24 is a source layout, never a replacement block descriptor or expansion policy',
        'oracle_passes': previous.get('oracle_passes', 0) + 1,
        'scope': 'Genuine next directory asset1 RawFile12->native24 header only after completed first StringTable; no name/buffer loading, registration, dispatch to later assets or rendering',
    })
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--previous-results', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / 'build-android-audit').resolve()) or output.exists():
        raise RuntimeError('Use a fresh ignored Android evidence directory')
    previous_raw = args.previous_results.read_bytes()
    previous = json.loads(previous_raw)
    if not previous['all_tests_completed']:
        raise RuntimeError('Previous gate did not complete')
    output.mkdir(parents=True)
    result = {'schema': 'BlackOpsARM.rawfile-header-oracle-batch.v1',
              'source_sha256': sha(Path(__file__).read_bytes()),
              'payload_stream_helper_sha256': sha((ROOT / 'tools/port/stringtable_payload_oracle.py').read_bytes()),
              'previous_results_path': str(args.previous_results.resolve()),
              'previous_results_sha256': sha(previous_raw), 'argv': [sys.executable, *sys.argv],
              'oracles': {}}
    try:
        for name in ('code_pre_gfx', 'zombie_pentagon'):
            oracle = previous['oracles'][name]
            result['oracles'][name] = reference(oracle['original_path'], oracle)
        result['status'] = 'passed'
    except Exception as error:
        result.update(status='failed', error=str(error))
        (output / 'results.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        raise
    (output / 'results.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'status': result['status'], 'output': str(output),
                      'boundaries': {name: {'status': oracle['status'],
                                          'decoded_source_end': oracle['decoded_source_end'],
                                          'next_type': oracle['next_directory_record']['type']}
                                     for name, oracle in result['oracles'].items()}}))


if __name__ == '__main__':
    main()
