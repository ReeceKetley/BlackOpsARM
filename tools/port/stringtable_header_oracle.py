"""Independent first StringTable disk header, without original string text/hex."""
from pathlib import Path
import hashlib
import struct
import zlib

from asset_directory_oracle import reference as directory_reference, sha


def reference(source, oracle):
    # Reuse prior metadata exactly; its bounded buffers are released before this
    # additional host streaming pass. No pass retains the complete payload.
    result = directory_reference(source, oracle)
    records = result['directory_records']
    if not records or records[0]['type'] != 37 or records[0]['tag'] != 0xffffffff:
        raise RuntimeError('Genuine first asset is not the supported inline StringTable boundary')
    start, wanted = result['prefix_bytes'], result['prefix_bytes'] + 20
    if wanted > result['decoded_bytes']:
        raise RuntimeError('Genuine StringTable header exceeds validated decoded length')
    decoder = zlib.decompressobj()
    prefix, tail = bytearray(), bytearray()
    raw_digest, decoded_digest = hashlib.sha256(), hashlib.sha256()
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
                        raise RuntimeError('Independent header decoder made no progress')
                    pending = next_pending
            if len(tail) > 31 or any(tail):
                raise RuntimeError('Invalid original FastFile trailing padding')
    if (not decoder.eof or len(prefix) != wanted or total != result['decoded_bytes']
            or raw_digest.hexdigest() != result['raw_sha256'] or decoded_digest.hexdigest() != result['decoded_sha256']
            or adler != result['decoded_adler32']):
        raise RuntimeError('Complete header stream provenance differs from frozen/preceding gates')
    if hashlib.sha256(memoryview(prefix)[:start]).hexdigest() != result['prefix_sha256']:
        raise RuntimeError('Preceding genuine directory prefix changed between independent passes')
    name, columns, rows, values, indices = struct.unpack_from('<IiiII', prefix, start)
    if columns < 0 or rows < 0:
        raise RuntimeError('Negative genuine StringTable dimensions')
    if any(tag != 0xffffffff for tag in [name, values, indices]):
        raise RuntimeError('Unsupported genuine StringTable child tags; no silent skips')
    result.update({
        'schema': 'BlackOpsARM.TASK-0018.stringtable-header-oracle.v1',
        'directory_prefix_bytes': start, 'directory_prefix_sha256': result['prefix_sha256'],
        'prefix_bytes': wanted, 'prefix_sha256': hashlib.sha256(prefix).hexdigest(),
        'stringtable_source_start': start, 'decoded_source_end': wanted, 'source_consumed_bytes': wanted - 52,
        'stringtable_header': {'name_tag': name, 'column_count': columns, 'row_count': rows,
                               'values_tag': values, 'cell_index_tag': indices},
        'stringtable_header_sha256': hashlib.sha256(memoryview(prefix)[start:wanted]).hexdigest(),
        'stringtable_disk_bytes': 20, 'stringtable_native64_bytes': 32,
        'stringtable_cell_count': columns * rows, 'first_asset_type': records[0]['type'],
        'oracle_passes': 3,
        'scope': 'First numeric StringTable header only after genuine directory; no name, cells/index payloads, fixups, asset registration or rendering',
    })
    return result
