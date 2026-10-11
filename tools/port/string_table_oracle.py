"""Independent bounded-prefix oracle; stream full originals without retaining payloads."""
from pathlib import Path
import hashlib
import struct
import zlib


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def reference(source, oracle):
    """Validate the complete wrapped stream, retaining only metadata and pointer tags.

    The system codec is an independent reference, never the production probe's
    decoder. Its output chunks are capped at 64 KiB, including high-ratio input.
    No string bytes, directory entries, or asset payloads are parsed here.
    """
    raw_digest = hashlib.sha256()
    decoded_digest = hashlib.sha256()
    decoded_bytes = 0
    adler = 1
    prefix = bytearray()
    wanted = 52
    fields = None
    tail = bytearray()
    decoder = zlib.decompressobj()

    def accept(output):
        nonlocal decoded_bytes, adler, wanted, fields
        decoded_digest.update(output)
        decoded_bytes += len(output)
        adler = zlib.adler32(output, adler)
        take = min(len(output), wanted - len(prefix))
        prefix.extend(output[:take])
        if fields is None and len(prefix) == 52:
            fields = struct.unpack_from('<iIiI', prefix, 36)
            count, tag, asset_count, _ = fields
            if count < 0 or count > 1 << 20 or asset_count < 0:
                raise RuntimeError('Invalid genuine envelope counts')
            wanted = 52 + (4 * count if tag else 0)
            prefix.extend(output[take:take + min(len(output) - take, wanted - len(prefix))])

    with Path(source).open('rb') as stream:
        raw = stream.read(12)
        raw_digest.update(raw)
        if len(raw) != 12 or raw[:8] != b'IWffu100' or struct.unpack('<I', raw[8:])[0] != 473:
            raise RuntimeError('Only genuine unsigned IWffu100/version473 inputs are supported')
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
    if not decoder.eof or fields is None or len(prefix) != wanted:
        raise RuntimeError('Truncated wrapped stream or table prefix')
    if raw_digest.hexdigest() != oracle['fileSHA256']:
        raise RuntimeError('Raw original differs from frozen reference')
    if (decoded_bytes != oracle['decodedBytes'] or decoded_digest.hexdigest() != oracle['decodedSHA256']
            or adler != int(oracle['decodedAdler32'], 16)):
        raise RuntimeError('Complete independent decode differs from frozen reference')
    xfile = list(struct.unpack_from('<9I', prefix))
    if xfile != oracle['xFileFields'] or decoded_bytes != 36 + xfile[0]:
        raise RuntimeError('Genuine allocation metadata or decoded length mismatch')
    count, strings_tag, asset_count, assets_tag = fields
    tags = list(struct.unpack_from('<' + 'I' * count, prefix, 52)) if strings_tag else []
    unexpected = [{'slot': i, 'tag': f'{tag:08x}'} for i, tag in enumerate(tags)
                  if tag not in (0, 0xffffffff)]
    if unexpected:
        raise RuntimeError('Unsupported genuine string tags; no entries skipped: ' + str(unexpected))
    table = prefix[52:]
    return {
        'schema': 'BlackOpsARM.TASK-0014.string-table-oracle.v1',
        'original_path': str(Path(source).resolve()), 'raw_sha256': raw_digest.hexdigest(),
        'decoded_bytes': decoded_bytes, 'decoded_sha256': decoded_digest.hexdigest(),
        'decoded_adler32': adler, 'padding_bytes': len(tail), 'xfile_fields': xfile,
        'prefix_hex': prefix.hex(), 'prefix_bytes': len(prefix),
        'prefix_sha256': hashlib.sha256(prefix).hexdigest(),
        'envelope_hex': prefix[36:52].hex(), 'envelope_sha256': hashlib.sha256(prefix[36:52]).hexdigest(),
        'string_count': count, 'strings_tag': strings_tag, 'asset_count': asset_count, 'assets_tag': assets_tag,
        'table_hex': table.hex(), 'table_sha256': hashlib.sha256(table).hexdigest(),
        'table_disk_bytes': len(table), 'table_tags': tags,
        'null_slots': [i for i, tag in enumerate(tags) if tag == 0],
        'inline_slots': [i for i, tag in enumerate(tags) if tag == 0xffffffff],
        'string_bytes_parsed': False,
        'oracle_codec': 'Python system zlib ' + zlib.ZLIB_RUNTIME_VERSION + '; complete streamed SHA/Adler, max64KiB output chunks',
        'scope': 'Metadata and packed pointer tags only; null tags do not establish whether an inline string is empty',
    }
