"""Independent packed asset-directory metadata; never return original string bytes."""
from pathlib import Path
from collections import Counter
import hashlib
import struct
import zlib

from raw_strings_oracle import reference as raw_reference, sha


def reference(source, oracle):
    # Reuse the frozen raw-string reference without changing its API/source.
    # It releases its bounded byte prefix before this second host streaming pass.
    # Neither pass retains a complete decompressed buffer.
    result = raw_reference(source, oracle)
    start = result['decoded_source_end']
    count = result['asset_count'] if result['assets_tag'] else 0
    if count < 0 or count > 1 << 20:
        raise RuntimeError('Genuine asset-directory count exceeds bounded helper domain')
    wanted = start + count * 8
    if wanted > result['decoded_bytes']:
        raise RuntimeError('Genuine directory exceeds validated decoded source')
    decoder = zlib.decompressobj()
    raw_digest, decoded_digest = hashlib.sha256(), hashlib.sha256()
    prefix, tail = bytearray(), bytearray()
    total, adler = 0, 1
    with Path(source).open('rb') as stream:
        header = stream.read(12)
        raw_digest.update(header)
        if len(header) != 12 or header[:8] != b'IWffu100' or struct.unpack('<I', header[8:])[0] != 473:
            raise RuntimeError('Expected unchanged unsigned original FastFile header')
        while chunk := stream.read(65536):
            raw_digest.update(chunk)
            if decoder.eof:
                tail.extend(chunk)
            else:
                pending = chunk
                while pending:
                    output = decoder.decompress(pending, 65536)
                    decoded_digest.update(output)
                    total += len(output)
                    adler = zlib.adler32(output, adler)
                    prefix.extend(output[:max(0, wanted - len(prefix))])
                    if decoder.eof:
                        tail.extend(decoder.unused_data)
                        break
                    next_pending = decoder.unconsumed_tail
                    if not output and len(next_pending) == len(pending):
                        raise RuntimeError('Independent directory decoder made no progress')
                    pending = next_pending
            if len(tail) > 31 or any(tail):
                raise RuntimeError('Invalid trailing original FastFile padding')
    if (not decoder.eof or len(prefix) != wanted or total != result['decoded_bytes']
            or raw_digest.hexdigest() != result['raw_sha256'] or decoded_digest.hexdigest() != result['decoded_sha256']
            or adler != result['decoded_adler32']):
        raise RuntimeError('Independent complete directory decode differs from raw-string/frozen provenance')
    if hashlib.sha256(memoryview(prefix)[:start]).hexdigest() != result['prefix_sha256']:
        raise RuntimeError('Preceding raw-string prefix changed across independent passes')
    records = []
    for index in range(count):
        asset_type, tag = struct.unpack_from('<iI', prefix, start + index * 8)
        if not 0 <= asset_type < 43:
            raise RuntimeError(f'Unsupported genuine asset type at index {index}: {asset_type}')
        if tag != 0xffffffff:
            raise RuntimeError(f'Unsupported genuine directory header tag at index {index}: {tag}; no entries skipped')
        records.append({'index': index, 'type': asset_type, 'tag': tag})
    disk_start = (result['block4_disk_cursor'] + 3) & ~3
    native_start = (result['block4_native64_cursor'] + 7) & ~7
    result.update({
        'schema': 'BlackOpsARM.TASK-0016.asset-directory-oracle.v1',
        'raw_prefix_bytes': start, 'raw_prefix_sha256': result['prefix_sha256'],
        'prefix_bytes': wanted, 'prefix_sha256': hashlib.sha256(prefix).hexdigest(),
        'directory_count': count, 'directory_source_start': start, 'directory_disk_bytes': count * 8,
        'directory_native64_bytes': count * 16,
        'directory_sha256': hashlib.sha256(memoryview(prefix)[start:wanted]).hexdigest(),
        'directory_disk_start': disk_start, 'directory_native64_start': native_start,
        'raw_block4_disk_end': result['block4_disk_cursor'], 'raw_block4_native64_end': result['block4_native64_cursor'],
        'directory_disk_padding': disk_start - result['block4_disk_cursor'],
        'directory_native64_padding': native_start - result['block4_native64_cursor'],
        'block4_disk_cursor': disk_start + count * 8, 'block4_native64_cursor': native_start + count * 16,
        'decoded_source_end': wanted, 'source_consumed_bytes': wanted - 52,
        'directory_records': records,
        'directory_type_histogram': [{'type': key, 'count': value} for key, value in sorted(Counter(r['type'] for r in records).items())],
        'directory_tag_histogram': [{'tag': key, 'count': value} for key, value in sorted(Counter(r['tag'] for r in records).items())],
        'oracle_passes': 2,
        'scope': 'Numeric packed directory only following verified raw strings; no asset-body dispatch/fixups, interning or original string text/hex',
    })
    return result
