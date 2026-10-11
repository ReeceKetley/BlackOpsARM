"""Full-stream original-game oracle retaining only a bounded raw-string prefix.

Returned metadata contains lengths, offsets and digests, never string text/hex
or original byte buffers. Pointer tags are validated, not silently skipped.
"""
from pathlib import Path
import hashlib
import struct
import zlib


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def reference(source, oracle):
    raw_digest = hashlib.sha256()
    decoded_digest = hashlib.sha256()
    decoder = zlib.decompressobj()
    decoded_bytes, adler = 0, 1
    prefix, tail = bytearray(), bytearray()
    phase, wanted = 'header', 52
    fields, xfile, tags, inline_indices, strings = None, None, [], [], []
    string_start, table_end = 52, 52

    def accept(output):
        nonlocal decoded_bytes, adler, phase, wanted, fields, xfile, tags
        nonlocal inline_indices, string_start, table_end
        decoded_digest.update(output)
        decoded_bytes += len(output)
        adler = zlib.adler32(output, adler)
        cursor = 0
        while phase != 'done':
            if phase in ('header', 'table'):
                take = min(len(output) - cursor, wanted - len(prefix))
                prefix.extend(output[cursor:cursor + take])
                cursor += take
                if len(prefix) != wanted:
                    return
                if phase == 'header':
                    xfile = list(struct.unpack_from('<9I', prefix))
                    fields = struct.unpack_from('<iIiI', prefix, 36)
                    count, tag, assets, _ = fields
                    if count < 0 or count > 1 << 20 or assets < 0:
                        raise RuntimeError('Invalid genuine envelope counts')
                    if xfile != oracle['xFileFields']:
                        raise RuntimeError('Genuine XFile allocation metadata differs')
                    wanted = 52 + (4 * count if tag else 0)
                    table_end = wanted
                    phase = 'table'
                else:
                    count, tag, _, _ = fields
                    tags = list(struct.unpack_from('<' + 'I' * count, prefix, 52)) if tag else []
                    unexpected = [{'index': i, 'tag': f'{tag:08x}'} for i, tag in enumerate(tags)
                                  if tag not in (0, 0xffffffff)]
                    if unexpected:
                        raise RuntimeError('Unsupported genuine raw-string tags; no skips: ' + str(unexpected))
                    inline_indices = [i for i, tag in enumerate(tags) if tag == 0xffffffff]
                    string_start = len(prefix)
                    phase = 'strings' if inline_indices else 'done'
                continue
            if cursor == len(output):
                return
            end = output.find(b'\0', cursor)
            stop = len(output) if end < 0 else end + 1
            take = stop - cursor
            if (len(prefix) + take - 52 > min(xfile[6], 1 << 20)
                    or len(prefix) + take - string_start > 65536):
                raise RuntimeError('Genuine raw-string prefix exceeds bounded gate domain')
            prefix.extend(output[cursor:stop])
            cursor = stop
            if end < 0:
                return
            value = memoryview(prefix)[string_start:]
            strings.append({'index': inline_indices[len(strings)], 'decoded_offset': string_start,
                            'length_including_nul': len(value), 'sha256': hashlib.sha256(value).hexdigest(),
                            'adler32': zlib.adler32(value)})
            del value
            string_start = len(prefix)
            if len(strings) == len(inline_indices):
                phase = 'done'

    with Path(source).open('rb') as stream:
        raw = stream.read(12)
        raw_digest.update(raw)
        if len(raw) != 12 or raw[:8] != b'IWffu100' or struct.unpack('<I', raw[8:])[0] != 473:
            raise RuntimeError('Expected genuine unsigned IWffu100/version473 original')
        while chunk := stream.read(65536):
            raw_digest.update(chunk)
            if decoder.eof:
                tail.extend(chunk)
            else:
                pending = chunk
                while pending:
                    output = decoder.decompress(pending, 65536)
                    accept(output)
                    if decoder.eof:
                        tail.extend(decoder.unused_data)
                        break
                    next_pending = decoder.unconsumed_tail
                    if not output and len(next_pending) == len(pending):
                        raise RuntimeError('Independent decoder made no progress')
                    pending = next_pending
            if len(tail) > 31 or any(tail):
                raise RuntimeError('Invalid trailing FastFile padding')
    if not decoder.eof or phase != 'done':
        raise RuntimeError('Truncated wrapped stream, table or unterminated raw string')
    if (raw_digest.hexdigest() != oracle['fileSHA256'] or decoded_bytes != oracle['decodedBytes']
            or decoded_digest.hexdigest() != oracle['decodedSHA256']
            or adler != int(oracle['decodedAdler32'], 16) or decoded_bytes != 36 + xfile[0]):
        raise RuntimeError('Full original-stream provenance differs from frozen reference')
    table_bytes = table_end - 52
    disk_cursor, native_cursor, padding = table_bytes, len(tags) * 8, 0
    for entry in strings:
        pad = (-native_cursor) % 8
        padding += pad
        native_cursor += pad
        entry.update({'block4_disk_offset': disk_cursor, 'block4_native64_offset': native_cursor,
                      'native64_alignment_padding': pad})
        disk_cursor += entry['length_including_nul']
        native_cursor += entry['length_including_nul']
    raw_view = memoryview(prefix)[table_end:]
    raw_sha, raw_adler = hashlib.sha256(raw_view).hexdigest(), zlib.adler32(raw_view)
    del raw_view
    return {
        'schema': 'BlackOpsARM.TASK-0015.raw-strings-oracle.v1',
        'original_path': str(Path(source).resolve()), 'raw_sha256': raw_digest.hexdigest(),
        'decoded_bytes': decoded_bytes, 'decoded_sha256': decoded_digest.hexdigest(),
        'decoded_adler32': adler, 'padding_bytes': len(tail), 'xfile_fields': xfile,
        'prefix_bytes': len(prefix), 'prefix_sha256': hashlib.sha256(prefix).hexdigest(),
        'string_count': fields[0], 'strings_tag': fields[1], 'asset_count': fields[2], 'assets_tag': fields[3],
        'table_tags': tags, 'table_disk_bytes': table_bytes,
        'table_sha256': hashlib.sha256(memoryview(prefix)[52:table_end]).hexdigest(),
        'null_slots': [i for i, tag in enumerate(tags) if tag == 0],
        'inline_string_count': len(strings), 'raw_string_bytes': len(prefix) - table_end,
        'raw_strings_sha256': raw_sha, 'raw_strings_adler32': raw_adler,
        'max_string_bytes': max((s['length_including_nul'] for s in strings), default=0),
        'empty_slots': [s['index'] for s in strings if s['length_including_nul'] == 1],
        'decoded_source_end': len(prefix), 'block4_disk_cursor': disk_cursor,
        'block4_native64_cursor': native_cursor, 'native64_padding_bytes': padding,
        'strings': strings,
        'oracle_codec': 'Python system zlib ' + zlib.ZLIB_RUNTIME_VERSION + '; full SHA/Adler, max64KiB output chunks',
        'scope': 'Raw bounded bytes/terminators only; no interning, encoded-offset fixups, directory or asset payload parsing',
    }
