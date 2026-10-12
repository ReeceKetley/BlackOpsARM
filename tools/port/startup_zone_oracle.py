"""Independent complete MP startup-zone metadata; never emit retail payloads.

This is a host oracle, not a runtime loader or production input generator.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import zlib

RAW_SHA = '75ffbf7a2ca0fe926e0f499abd182b5a1c3bb52e7c218c3956fa736dfda2a44f'
DECODED_SHA = '42ff63a5586b5876e0b2a54a45aa66b18c4a2678645715265f2bff141f044e88'


def inspect_payload(data):
    if len(data) != 150108:
        raise ValueError('Unexpected complete decoded size')
    data = bytes(data)
    fields = struct.unpack_from('<9I', data)
    if fields != (150072, 0, 36, 0, 0, 0, 149187, 0, 0):
        raise ValueError('Unexpected XFile layout')
    if struct.unpack_from('<4I', data, 36) != (0, 0, 71, 0xffffffff):
        raise ValueError('Unexpected asset-list envelope')
    directory = [struct.unpack_from('<iI', data, 52 + i * 8) for i in range(71)]
    if any(t not in (36, 37) or p != 0xffffffff for t, p in directory):
        raise ValueError('Unsupported directory root; no skipping')
    cursor, disk, native = 620, 568, 1136
    initialized, names, rows = {}, set(), []

    def take(count):
        nonlocal cursor
        if count < 0 or count > len(data) - cursor:
            raise ValueError('Asset exceeds finite decoded source')
        start = cursor
        cursor += count
        return data[start:cursor]

    def align(mask):
        nonlocal disk, native
        disk = (disk + mask) & ~mask
        native = (native + 7) & ~7

    def string():
        nonlocal cursor, disk, native
        end = data.find(b'\0', cursor)
        if end < 0:
            raise ValueError('Unterminated child string')
        start = cursor
        value = take(end - cursor + 1)
        align(0)
        initialized[disk] = (native, value)
        record = {'source_start': start, 'source_end': cursor,
                  'disk_start': disk, 'native64_start': native,
                  'bytes': len(value), 'sha256': hashlib.sha256(value).hexdigest(),
                  'adler32': zlib.adler32(value)}
        disk += len(value)
        native += len(value)
        return value, record

    for index, (kind, _) in enumerate(directory):
        start = cursor
        row = {'index': index, 'type': kind, 'source_start': start}
        if kind == 36:
            name_tag, length, buffer_tag = struct.unpack('<IiI', take(12))
            if name_tag != 0xffffffff or length < 0 or buffer_tag != 0xffffffff:
                raise ValueError('Unsupported RawFile children')
            name, row['name'] = string()
            payload = take(length + 1)
            align(0)
            row.update(length=length, buffer_bytes=len(payload),
                       buffer_adler32=zlib.adler32(payload),
                       buffer_sha256=hashlib.sha256(payload).hexdigest())
            disk += len(payload)
            native += len(payload)
        else:
            name_tag, columns, count_rows, values_tag, indices_tag = struct.unpack('<IiiII', take(20))
            count = columns * count_rows
            if (columns < 0 or count_rows < 0 or count > 32768
                    or any(p != 0xffffffff for p in (name_tag, values_tag, indices_tag))):
                raise ValueError('Unsupported StringTable dimensions/children')
            name, row['name'] = string()
            descriptors = take(count * 8)
            align(3)
            disk += count * 8
            native += count * 16
            inline, aliases = 0, 0
            cells = []
            for cell in range(count):
                tag, value_hash = struct.unpack_from('<Ii', descriptors, cell * 8)
                if tag == 0xffffffff:
                    _, record = string()
                    inline += 1
                elif tag:
                    block, offset = (tag - 1) >> 29, (tag - 1) & 0x1fffffff
                    if block != 4 or offset not in initialized:
                        raise ValueError('Alias is not an earlier initialized character start')
                    native_offset, value = initialized[offset]
                    record = {'disk_target': offset, 'native64_target': native_offset,
                              'bytes': len(value), 'adler32': zlib.adler32(value),
                              'sha256': hashlib.sha256(value).hexdigest()}
                    aliases += 1
                else:
                    raise ValueError('Unexpected null genuine cell')
                cells.append({'index': cell, 'signed_hash': value_hash, 'string': record})
            packed_indices = take(count * 2)
            indices = struct.unpack('<' + 'h' * count, packed_indices)
            if sorted(indices) != list(range(count)):
                raise ValueError('Original signed indices are not a complete permutation')
            align(1)
            disk += count * 2
            native += count * 2
            row.update(columns=columns, rows=count_rows, cell_count=count,
                       inline_strings=inline, aliases=aliases, cells=cells,
                       indices_sha256=hashlib.sha256(packed_indices).hexdigest(),
                       indices_adler32=zlib.adler32(packed_indices))
        if not name[:-1] or name.startswith(b',') or name.lower() in names:
            raise ValueError('Unexpected missing/default/duplicate startup asset name')
        names.add(name.lower())
        row.update(source_end=cursor, block4_disk_end=disk, block4_native64_end=native)
        rows.append(row)
    if cursor != len(data) or disk != fields[6]:
        raise ValueError('Complete zone leaves source or block4 residue')
    if sum(r['type'] == 37 for r in rows) != 3:
        raise ValueError('Unexpected complete startup histogram')
    return {'asset_count': 71, 'stringtables': 3, 'rawfiles': 68,
            'source_end': cursor, 'block4_disk_end': disk,
            'block4_native64_end': native, 'assets': rows,
            'production_functions_executed': False}


def inspect(path):
    path = Path(path)
    if path.stat().st_size > 1024 * 1024:
        raise ValueError('Retail input exceeds oracle bound')
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != RAW_SHA or raw[:12] != b'IWffu100' + struct.pack('<I', 473):
        raise ValueError('Retail provenance differs')
    codec = zlib.decompressobj()
    data = codec.decompress(raw[12:], 150109)
    if (len(data) != 150108 or not codec.eof or codec.unconsumed_tail
            or len(codec.unused_data) != 27 or any(codec.unused_data)
            or hashlib.sha256(data).hexdigest() != DECODED_SHA):
        raise ValueError('Complete bounded decode provenance differs')
    result = inspect_payload(data)
    result.update(schema='BlackOpsARM.startup-zone-oracle.v1',
                  raw_sha256=RAW_SHA, decoded_sha256=DECODED_SHA,
                  decoded_bytes=len(data), decoded_adler32=zlib.adler32(data),
                  raw_bytes=len(raw), padding_bytes=27)
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('retail_path')
    parser.add_argument('output')
    args = parser.parse_args()
    result = inspect(args.retail_path)
    output = Path(args.output)
    if output.exists():
        raise SystemExit('Use a fresh oracle evidence path')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('Complete genuine MP startup oracle: 71 assets, exact source/block4 end')
