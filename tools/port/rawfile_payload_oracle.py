"""Independent streaming provenance for the genuine second RawFile children.

Only code_pre_gfx's actual directory entry1 is supported. Zombie entry1 remains
TechniqueSet/type7 without a search or skip. No game text/hex or payload bytes
are emitted; exact byte extents and hashes identify the original child data.
"""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import traceback
import zlib

sys.dont_write_bytecode = True
from rawfile_header_oracle import CODE_SHA, ZOMBIE_SHA, verify_directory
from stringtable_payload_oracle import _stream_prefix
from verify_temp_cursor_source import function, lf

ROOT = Path(__file__).resolve().parents[2]
SOURCE_CHECKPOINT = '7f7f005'
OUTPUT_CHUNK_BYTES = 4096


def sha(data):
    return hashlib.sha256(data).hexdigest()


def source_contract():
    commit = subprocess.check_output(
        ['git', '--no-optional-locks', 'rev-parse', '--verify', SOURCE_CHECKPOINT + '^{commit}'], cwd=ROOT).decode().strip()
    records = {}
    for path, names in (
        ('src/database/db_load.cpp', ('Load_RawFile', 'Load_XString', 'AllocLoad_raw_byte', 'Load_ConstCharArray')),
        ('src/database/db_stream_load.cpp', ('Load_XStringCustom', 'Load_Stream')),
        ('src/database/db_stream.cpp', ('DB_AllocStreamPos',))):
        raw = subprocess.check_output(['git', '--no-optional-locks', 'show', commit + ':' + path], cwd=ROOT)
        records[path] = {'git_blob_sha256': sha(raw), 'functions': {
            name: {'lf_body_sha256': sha(function(lf(raw), name))} for name in names}}
    return {'checkpoint': commit, 'sources': records,
            'name_contract': 'Inline -1 name: AllocLoad_raw_byte(mask0), then Load_XStringCustom through first NUL inclusive',
            'buffer_contract': 'Only if buffer is nonzero: AllocLoad_raw_byte(mask0), then Load_ConstCharArray(len+1); exact byte copy, no terminator validation in these leaves',
            'alignment_contract': 'Original disk mask0; native DB_AllocStreamPos uses mask0 OR sizeof(void*)-1',
            'production_functions_executed': False}


def stream_code_children(source, previous):
    """Discover name end while hashing the entire decode; retain no later bytes."""
    if (previous['raw_sha256'] != CODE_SHA or previous['status'] != 'supported'
            or previous['prefix_bytes'] != 10419 or previous['rawfile_source_end'] != 10419
            or previous['rawfile_source_start'] != 10407
            or previous['rawfile_header'] != {'name_tag': 0xffffffff, 'length': 140, 'buffer_tag': 0xffffffff}):
        raise RuntimeError('Unsupported genuine RawFile header provenance')
    name_start = previous['rawfile_source_end']
    buffer_bytes = previous['rawfile_header']['length'] + 1
    if previous['rawfile_future_buffer_bytes'] != buffer_bytes:
        raise RuntimeError('Header future buffer request differs from signed len+1')
    decoder = zlib.decompressobj()
    raw_digest, decoded_digest = hashlib.sha256(), hashlib.sha256()
    prefix, padding = bytearray(), bytearray()
    total, adler, name_end, payload_end = 0, 1, None, None
    maximum_output = 0
    with Path(source).open('rb') as stream:
        header = stream.read(12)
        raw_digest.update(header)
        if header != b'IWffu100' + struct.pack('<I', 473):
            raise RuntimeError('Expected unchanged unsigned FastFile version473')
        while chunk := stream.read(65536):
            raw_digest.update(chunk)
            if decoder.eof:
                padding.extend(chunk)
            else:
                pending = chunk
                while pending:
                    output = decoder.decompress(pending, OUTPUT_CHUNK_BYTES)
                    start = total
                    total += len(output)
                    maximum_output = max(maximum_output, len(output))
                    if total > previous['decoded_bytes']:
                        raise RuntimeError('Decode exceeds the genuine frozen source extent')
                    decoded_digest.update(output)
                    adler = zlib.adler32(output, adler)
                    if name_end is None and total > name_start:
                        local_start = max(0, name_start - start)
                        nul = output.find(b'\0', local_start)
                        if nul >= 0:
                            name_end = start + nul + 1
                            payload_end = name_end + buffer_bytes
                            if payload_end > previous['decoded_bytes']:
                                raise RuntimeError('Genuine len+1 buffer exceeds decoded extent')
                    if payload_end is None:
                        prefix.extend(output)
                    else:
                        prefix.extend(output[:max(0, payload_end - len(prefix))])
                    if decoder.eof:
                        padding.extend(decoder.unused_data)
                        break
                    remaining = decoder.unconsumed_tail
                    if not output and len(remaining) == len(pending):
                        raise RuntimeError('Independent decoder made no progress')
                    pending = remaining
            if len(padding) > 31 or any(padding):
                raise RuntimeError('Invalid original trailing padding')
    if (not decoder.eof or total != previous['decoded_bytes']
            or raw_digest.hexdigest() != previous['raw_sha256']
            or decoded_digest.hexdigest() != previous['decoded_sha256']
            or adler != previous['decoded_adler32'] or len(padding) != previous['padding_bytes']):
        raise RuntimeError('Complete raw/decoded provenance differs from the frozen header reference')
    if name_end is None or payload_end is None or len(prefix) != payload_end:
        raise RuntimeError('Missing bounded NUL name or complete genuine buffer bytes')
    if sha(memoryview(prefix)[:previous['prefix_bytes']]) != previous['prefix_sha256']:
        raise RuntimeError('Preceding genuine header/payload prefix changed')
    verify_directory(prefix, previous)
    actual_header = struct.unpack_from('<IiI', prefix, previous['rawfile_source_start'])
    if actual_header != (0xffffffff, 140, 0xffffffff):
        raise RuntimeError('Actual decoded RawFile fields differ from the frozen header')
    return prefix, name_end, payload_end, {
        'decoded_bytes': total, 'decoded_sha256': decoded_digest.hexdigest(),
        'decoded_adler32': adler, 'raw_sha256': raw_digest.hexdigest(),
        'padding_bytes': len(padding), 'maximum_output_chunk_bytes': maximum_output,
        'retained_prefix_bytes': len(prefix), 'full_decoded_buffer_retained': False,
    }


def byte_record(data, start, end):
    view = memoryview(data)[start:end]
    return {'source_start': start, 'source_end': end, 'bytes': end - start,
            'sha256': sha(view), 'adler32': zlib.adler32(view),
            'nul_count': sum(byte == 0 for byte in view)}


def reference(source, previous):
    record = previous['next_directory_record']
    if record['index'] != 1 or record['tag'] != 0xffffffff:
        raise RuntimeError('No genuine inline next-directory entry1 provenance')
    if previous['raw_sha256'] == ZOMBIE_SHA:
        if previous['status'] != 'deferred' or record['type'] != 7 or previous['prefix_bytes'] != 65678:
            raise RuntimeError('Zombie must defer at its actual TechniqueSet boundary')
        prefix = _stream_prefix(source, previous, 65678)
        verify_directory(prefix, previous)
        result = dict(previous)
        result.update({'schema': 'BlackOpsARM.rawfile-payload-oracle.v1', 'status': 'deferred',
                       'reason': 'Actual next directory entry1 is TechniqueSet type7',
                       'child_extension_bytes': 0, 'children_scanned': False,
                       'no_next_asset_skipped': True, 'retained_decoded_buffer_limit': 65678,
                       'full_decoded_buffer_retained': False,
                       'scope': 'Complete streaming provenance verified; no TechniqueSet or later RawFile children parsed'})
        return result
    if previous['raw_sha256'] != CODE_SHA or record['type'] != 36:
        raise RuntimeError('Unsupported original or next asset type; no search/skip')
    prefix, name_end, payload_end, measured = stream_code_children(source, previous)
    name_start = previous['rawfile_source_end']
    name = byte_record(prefix, name_start, name_end)
    name.update({'bytes_including_nul': name['bytes'], 'text_bytes': name['bytes'] - 1,
                 'first_nul_is_final_byte': True, 'encoding_interpreted': False,
                 'disk_alignment_mask': 0, 'native64_alignment_bytes': 8})
    buffer = byte_record(prefix, name_end, payload_end)
    stored_length = previous['rawfile_header']['length']
    buffer.update({'stored_length': stored_length, 'production_copy_bytes': stored_length + 1,
                   'stored_length_region_sha256': sha(memoryview(prefix)[name_end:name_end + stored_length]),
                   'stored_length_region_adler32': zlib.adler32(memoryview(prefix)[name_end:name_end + stored_length]),
                   'stored_length_region_nul_count': sum(byte == 0 for byte in memoryview(prefix)[name_end:name_end + stored_length]),
                   'additional_byte': prefix[payload_end - 1],
                   'additional_byte_is_nul': prefix[payload_end - 1] == 0,
                   'terminator_observation_is_not_production_validation': True,
                   'disk_alignment_mask': 0, 'native64_alignment_bytes': 8})
    disk = previous['stringtable_indices']['disk_end']
    native = previous['stringtable_indices']['native64_end']
    for child in (name, buffer):
        aligned = (native + 7) & ~7
        child.update({'disk_start': disk, 'disk_end': disk + child['bytes'],
                      'native64_start': aligned, 'native64_end': aligned + child['bytes'],
                      'disk_alignment_padding': 0, 'native64_alignment_padding': aligned - native})
        disk, native = child['disk_end'], child['native64_end']
    result = dict(previous)
    result.update({'schema': 'BlackOpsARM.rawfile-payload-oracle.v1', 'status': 'supported',
                   'previous_prefix_bytes': previous['prefix_bytes'], 'previous_prefix_sha256': previous['prefix_sha256'],
                   'prefix_bytes': payload_end, 'prefix_sha256': sha(prefix),
                   'execution_prefix_bytes': payload_end, 'execution_prefix_sha256': sha(prefix),
                   'decoded_source_end': payload_end, 'source_consumed_bytes': payload_end - 52,
                   'child_extension_bytes': payload_end - previous['prefix_bytes'],
                   'retained_decoded_buffer_limit': payload_end, 'rawfile_name': name, 'rawfile_buffer': buffer,
                   'rawfile_payload_source_start': name_start, 'rawfile_payload_source_end': payload_end,
                   'rawfile_payload_disk_end': disk, 'rawfile_payload_native64_end': native,
                   'streaming_provenance': measured, 'rawfile_children_unread': False,
                   'children_scanned': True, 'no_next_asset_skipped': True,
                   'production_loader_executed': False, 'full_registry_executed': False,
                   'original_byte_buffers_in_evidence': False,
                   'scope': 'Independent genuine entry1 name and exact len+1 buffer bytes/digests; cursor extents derived only from existing production alignment and preceding metadata, not a native execution or allocation policy'})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--previous-results', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / 'build-android-audit').resolve()) or output.exists():
        raise RuntimeError('Use a fresh ignored Android evidence directory')
    output.mkdir(parents=True)
    result = {'schema': 'BlackOpsARM.rawfile-payload-oracle-batch.v1',
              'recorded_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'argv': [sys.executable, *sys.argv], 'source_sha256': sha(Path(__file__).read_bytes()),
              'helper_sha256': {name: sha((ROOT / 'tools/port' / name).read_bytes()) for name in (
                  'rawfile_header_oracle.py', 'stringtable_payload_oracle.py', 'verify_temp_cursor_source.py')},
              'native_gate_executed': False, 'independent_python_zlib': zlib.ZLIB_RUNTIME_VERSION,
              'original_byte_buffers_in_evidence': False, 'oracles': {}, 'status': 'incomplete'}
    try:
        previous_raw = args.previous_results.read_bytes()
        previous = json.loads(previous_raw)
        if (previous.get('schema') != 'BlackOpsARM.TASK-0028.rawfile-header-gate.v1'
                or previous.get('all_tests_completed') is not True
                or set(previous.get('oracles', {})) != {'code_pre_gfx', 'zombie_pentagon'}):
            raise RuntimeError('Require completed genuine TASK28 header evidence')
        result.update({'previous_results_path': str(args.previous_results.resolve()),
                       'previous_results_sha256': sha(previous_raw), 'production_source_contract': source_contract()})
        for name in ('code_pre_gfx', 'zombie_pentagon'):
            prior = previous['oracles'][name]
            result['oracles'][name] = reference(prior['original_path'], prior)
        result['status'] = 'passed'
    except Exception as error:
        result.update(status='failed', error_type=type(error).__name__, error=str(error), traceback=traceback.format_exc())
        (output / 'results.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        raise
    (output / 'results.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'status': 'passed', 'output': str(output), 'boundaries': {
        name: {'status': oracle['status'], 'prefix_bytes': oracle['prefix_bytes'],
               'child_extension_bytes': oracle['child_extension_bytes']}
        for name, oracle in result['oracles'].items()}}))


if __name__ == '__main__':
    main()
