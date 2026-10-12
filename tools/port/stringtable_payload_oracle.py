"""Independent first StringTable payload metadata; never emit game text/bytes.

Full original/decoded SHA and Adler are verified. Retained decoded buffers stop
at independently audited first-asset endpoints, not at the whole zone. These
source extent anchors do not supply or fabricate production allocation metadata.
"""
from pathlib import Path
import hashlib
import struct
import sys
import zlib

sys.dont_write_bytecode = True
from stringtable_header_oracle import reference as header_reference, sha

FIRST_ASSET_ENDS = {
    '565f038d4a5a10912bda5741e80f31805f9c7c76fc18296f2a8591d58f1ee41f': 10407,
    '70686402a689d5c575f18d2d76e6cec9f9a95ff70446372960f1b5c7f41d92e0': 65678,
}


def _stream_prefix(source, prior, wanted):
    decoder = zlib.decompressobj()
    raw_digest, decoded_digest = hashlib.sha256(), hashlib.sha256()
    prefix, tail = bytearray(), bytearray()
    total, adler = 0, 1
    with Path(source).open('rb') as stream:
        header = stream.read(12)
        raw_digest.update(header)
        if header != b'IWffu100' + struct.pack('<I', 473):
            raise RuntimeError('Expected unchanged genuine unsigned FastFile header')
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
                    remaining = decoder.unconsumed_tail
                    if not output and len(remaining) == len(pending):
                        raise RuntimeError('Independent payload decoder made no progress')
                    pending = remaining
            if len(tail) > 31 or any(tail):
                raise RuntimeError('Invalid original FastFile padding')
    if (not decoder.eof or len(prefix) != wanted or total != prior['decoded_bytes']
            or raw_digest.hexdigest() != prior['raw_sha256']
            or decoded_digest.hexdigest() != prior['decoded_sha256']
            or adler != prior['decoded_adler32'] or len(tail) != prior['padding_bytes']):
        raise RuntimeError('Complete payload decode provenance differs from frozen/header references')
    if hashlib.sha256(memoryview(prefix)[:prior['prefix_bytes']]).hexdigest() != prior['prefix_sha256']:
        raise RuntimeError('Preceding StringTable header prefix changed between independent passes')
    return prefix


def reference(source, oracle):
    result = header_reference(source, oracle)
    wanted = FIRST_ASSET_ENDS.get(result['raw_sha256'])
    if wanted is None or wanted > result['decoded_bytes']:
        raise RuntimeError('No independently audited first-asset source extent for this original')
    prefix = _stream_prefix(source, result, wanted)
    source_position = result['decoded_source_end']
    header = result['stringtable_header']
    count = header['column_count'] * header['row_count']
    if not 0 <= count <= 1 << 20:
        raise RuntimeError('Genuine cell count outside actual native expansion helper domain')
    if any(header[key] != 0xffffffff for key in ['name_tag', 'values_tag', 'cell_index_tag']):
        raise RuntimeError('Unsupported genuine StringTable child tags; no silent skips')
    disk_cursor, native_cursor = result['block4_disk_cursor'], result['block4_native64_cursor']

    def string_record(kind, index):
        nonlocal source_position, disk_cursor, native_cursor
        start = source_position
        try:
            end = prefix.index(0, start) + 1
        except ValueError as error:
            raise RuntimeError('Unterminated first-asset string inside verified source extent') from error
        data = memoryview(prefix)[start:end]
        native_start = (native_cursor + 7) & ~7
        record = {'kind': kind, 'index': index, 'source_start': start, 'source_end': end,
                  'length_including_nul': end - start, 'sha256': hashlib.sha256(data).hexdigest(),
                  'adler32': zlib.adler32(data), 'disk_start': disk_cursor, 'disk_end': disk_cursor + end - start,
                  'native64_start': native_start, 'native64_end': native_start + end - start,
                  'disk_alignment_padding': 0, 'native64_alignment_padding': native_start - native_cursor}
        source_position = end
        disk_cursor = record['disk_end']
        native_cursor = record['native64_end']
        return record

    name = string_record('name', 0)
    cell_source_start = source_position
    cell_source_end = cell_source_start + count * 8
    if cell_source_end > wanted:
        raise RuntimeError('Packed cell descriptors exceed independently verified first-asset extent')
    disk_start, native_start = (disk_cursor + 3) & ~3, (native_cursor + 7) & ~7
    array = {'count': count, 'source_start': cell_source_start, 'source_end': cell_source_end,
             'disk_bytes': count * 8, 'native64_bytes': count * 16,
             'sha256': hashlib.sha256(memoryview(prefix)[cell_source_start:cell_source_end]).hexdigest(),
             'disk_start': disk_start, 'disk_end': disk_start + count * 8,
             'native64_start': native_start, 'native64_end': native_start + count * 16,
             'disk_alignment_padding': disk_start - disk_cursor,
             'native64_alignment_padding': native_start - native_cursor}
    disk_cursor, native_cursor, source_position = array['disk_end'], array['native64_end'], cell_source_end
    initialized = {}
    cells = []
    inline_count, offset_count = 0, 0
    for index in range(count):
        tag, signed_hash = struct.unpack_from('<Ii', prefix, cell_source_start + index * 8)
        entry = {'index': index, 'tag': tag, 'signed_hash': signed_hash,
                 'source_descriptor_start': cell_source_start + index * 8,
                 'disk_descriptor_start': disk_start + index * 8,
                 'native64_descriptor_start': native_start + index * 16}
        if tag == 0xffffffff:
            record = string_record('cell', index)
            initialized[record['disk_start']] = record
            entry['string'] = record
            inline_count += 1
        elif tag == 0:
            entry['string'] = {'kind': 'null', 'source_bytes': 0}
        else:
            block, within = (tag - 1) >> 29, (tag - 1) & 0x1fffffff
            target = initialized.get(within) if block == 4 else None
            if target is None:
                raise RuntimeError('Cell offset is invalid, interior or uninitialized; no entry is skipped: ' + str(index))
            entry['string'] = {'kind': 'offset', 'block': block, 'disk_target': within,
                               'native64_target': target['native64_start'], 'target_cell_index': target['index'],
                               'length_including_nul': target['length_including_nul'],
                               'sha256': target['sha256'], 'adler32': target['adler32'], 'source_bytes': 0,
                               'initialized_before_cell': index}
            offset_count += 1
        cells.append(entry)
    index_start, index_end = source_position, source_position + count * 2
    if index_end != wanted:
        raise RuntimeError('First-asset source endpoint differs from independently audited extent')
    indices = struct.unpack_from('<' + 'h' * count, prefix, index_start)
    if any(index < 0 or index >= count for index in indices):
        raise RuntimeError('Genuine signed16 indices fall outside the first table')
    index_disk_start, index_native_start = (disk_cursor + 1) & ~1, (native_cursor + 7) & ~7
    index_metadata = {'count': count, 'values': list(indices), 'source_start': index_start, 'source_end': index_end,
                      'disk_bytes': count * 2, 'native_bytes': count * 2,
                      'sha256': hashlib.sha256(memoryview(prefix)[index_start:index_end]).hexdigest(),
                      'adler32': zlib.adler32(memoryview(prefix)[index_start:index_end]),
                      'minimum_signed_value': min(indices, default=None), 'maximum_signed_value': max(indices, default=None),
                      'disk_start': index_disk_start, 'disk_end': index_disk_start + count * 2,
                      'native64_start': index_native_start, 'native64_end': index_native_start + count * 2,
                      'disk_alignment_padding': index_disk_start - disk_cursor,
                      'native64_alignment_padding': index_native_start - native_cursor}
    if index_metadata['disk_end'] > result['xfile_fields'][6]:
        raise RuntimeError('Genuine first-asset block4 footprint exceeds original XFile metadata')
    # Exact ephemeral byte spans, never strings/hex in returned evidence. Offset
    # cells borrow the already initialized earlier inline cell's folded bytes.
    folded = []
    for entry in cells:
        record = entry['string']
        if record['kind'] == 'offset':
            value = folded[record['target_cell_index']]
        elif record['kind'] == 'cell':
            data = memoryview(prefix)[record['source_start']:record['source_end'] - 1]
            if any(byte >= 128 for byte in data):
                raise RuntimeError('Non-ASCII genuine lookup bytes need a separately audited locale contract')
            value = bytes(data).lower()
        else:
            raise RuntimeError('An absent cell cannot provide a genuine lookup query')
        folded.append(value)
        record['lowercase_sha256'] = hashlib.sha256(value).hexdigest()
    hashes = [entry['signed_hash'] for entry in cells]
    columns = header['column_count']
    lookup_records = []
    for index, value in enumerate(folded):
        query_hash = 5381
        for byte in value:
            query_hash = (byte + 33 * query_hash) & 0xffffffff
        if query_hash & 0x80000000:
            query_hash -= 1 << 32
        if query_hash != hashes[index]:
            raise RuntimeError('Genuine stored hash disagrees with independent ASCII wrapping hash')
        begin, end = 0, count
        while begin < end:
            middle = (end + begin) // 2
            if hashes[indices[middle]] >= query_hash:
                end = middle
            else:
                begin = middle + 1
        first_hash_index = begin
        row = -1
        while begin < count:
            candidate = indices[begin]
            if hashes[candidate] != query_hash:
                break
            if candidate % columns == index % columns and folded[candidate] == value:
                row = candidate // columns
                break
            begin += 1
        # Independent linear retail-index reference: no binary ordering assumed.
        linear_row = next((candidate // columns for candidate in indices
                           if candidate % columns == index % columns and hashes[candidate] == query_hash
                           and folded[candidate] == value), -1)
        column = -1
        for candidate_column in range(columns - 1, -1, -1):
            candidate = candidate_column + columns * (index // columns)
            if hashes[candidate] == query_hash and folded[candidate] == value:
                column = candidate_column
                break
        matching_columns = [candidate % columns for candidate in range((index // columns) * columns,
                                                                       (index // columns + 1) * columns)
                            if hashes[candidate] == query_hash and folded[candidate] == value]
        linear_column = max(matching_columns, default=-1)
        lookup_records.append({'index': index, 'hash': query_hash, 'first_hash_index': first_hash_index,
                               'row': row, 'linear_row': linear_row, 'column': column, 'linear_column': linear_column})
    result.update({'schema': 'BlackOpsARM.stringtable-payload-oracle.v1',
                   'header_prefix_bytes': result['prefix_bytes'], 'header_prefix_sha256': result['prefix_sha256'],
                   'prefix_bytes': wanted, 'prefix_sha256': hashlib.sha256(prefix).hexdigest(),
                   'task19_prefix_bytes': cell_source_end,
                   'task19_prefix_sha256': hashlib.sha256(memoryview(prefix)[:cell_source_end]).hexdigest(),
                   'task20_prefix_bytes': index_start,
                   'task20_prefix_sha256': hashlib.sha256(memoryview(prefix)[:index_start]).hexdigest(),
                   'task20_block4_disk_cursor': disk_cursor,
                   'task20_block4_native64_cursor': native_cursor,
                   'decoded_source_end': wanted, 'source_consumed_bytes': wanted - 52,
                   'stringtable_name': name, 'stringtable_cell_array': array, 'stringtable_cells': cells,
                   'inline_cell_string_count': inline_count, 'offset_cell_string_count': offset_count,
                   'stringtable_indices': index_metadata,
                   'stringtable_lookup_records': lookup_records,
                   'stringtable_lookup_source_model_misses': sum(record['row'] < 0 for record in lookup_records),
                   'stringtable_lookup_row_disagreements': sum(record['row'] != record['linear_row'] for record in lookup_records),
                   'stringtable_lookup_column_disagreements': sum(record['column'] != record['linear_column'] for record in lookup_records),
                   'block4_disk_cursor': index_metadata['disk_end'],
                   'block4_native64_cursor': index_metadata['native64_end'], 'oracle_passes': 4,
                   'retained_decoded_buffer_limit': wanted,
                   'scope': 'Independent first StringTable payload numeric metadata/digests; prior initialized cell-string starts only; no game text/hex, registry/fixup execution or next-asset parsing',
                   'layout_contract': 'Win32 disk alignment0 strings/3 cells/1 indices; native64 minimum8 for all allocations. Alignment consumes no decoded-file bytes. Offset cells consume no decoded-file bytes.'})
    return result
