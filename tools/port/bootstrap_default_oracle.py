"""Independent bounded retail bootstrap graph oracle; exports metadata only."""
import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path

KNOWN = {
    'code_post_gfx_mp': ('48133f39f8a7a931cd7b4d79d2f60d813cbf44c0c3cd9d38d7bd89e40f5a668b',
                         'b0fd31e65f5549003e0ba7b26ef268ca3f32d95ea8856884191f7abc51c53f22', 3840157, 7966),
    'code_post_gfx': ('d47945f6290e930c6b2db6801e7f87412f2f63fd6ab4e9dc1a3b28b1994a98e9',
                     '2dbc5c0a954aa79ffa8068ead1fb84015c502d085b4650890e83a91a54fe975d', 6222433, 9204),
}


def decode(source, expected):
    raw, decoded = hashlib.sha256(), hashlib.sha256()
    decoder = zlib.decompressobj()
    prefix, tail = bytearray(), bytearray()
    total, adler = 0, 1
    with Path(source).open('rb') as stream:
        header = stream.read(12)
        if header != b'IWffu100' + struct.pack('<I', 473):
            raise RuntimeError('Wrong original retail FastFile envelope')
        raw.update(header)
        while chunk := stream.read(65536):
            raw.update(chunk)
            if decoder.eof:
                tail.extend(chunk)
            else:
                pending = chunk
                while pending:
                    output = decoder.decompress(pending, 65536)
                    decoded.update(output)
                    total += len(output)
                    if total > expected[2]:
                        raise RuntimeError('Decoded original exceeds its audited extent')
                    adler = zlib.adler32(output, adler)
                    prefix.extend(output[:max(0, 65536 - len(prefix))])
                    if decoder.eof:
                        tail.extend(decoder.unused_data)
                        break
                    remaining = decoder.unconsumed_tail
                    if not output and len(remaining) == len(pending):
                        raise RuntimeError('Independent decode made no progress')
                    pending = remaining
            if len(tail) > 31 or any(tail):
                raise RuntimeError('Invalid trailing retail padding')
    if (not decoder.eof or (raw.hexdigest(), decoded.hexdigest(), total) != expected[:3]):
        raise RuntimeError('Full retail provenance differs')
    return prefix, {'raw_sha256': raw.hexdigest(), 'decoded_sha256': decoded.hexdigest(),
                    'decoded_bytes': total, 'decoded_adler32': adler, 'padding_bytes': len(tail)}


class Graph:
    def __init__(self, data):
        self.data, self.source = data, 52
        self.disk, self.native = 0, 0
        self.records, self.initialized, self.fixups = [], {}, []

    def read(self, count):
        if count < 0 or self.source + count > len(self.data):
            raise RuntimeError('Graph exceeds bounded decoded prefix')
        start = self.source
        self.source += count
        return self.data[start:self.source]

    def record(self, kind, count, native_count=None, disk_align=3, native_align=7, block=4):
        start = self.source
        payload = self.read(count)
        native_count = count if native_count is None else native_count
        disk_start = (self.disk + disk_align) & ~disk_align if block == 4 else 0
        native_start = (self.native + native_align) & ~native_align if block == 4 else 0
        entry = {'kind': kind, 'source_start': start, 'source_end': self.source,
                 'disk_bytes': count, 'native64_bytes': native_count, 'block': block,
                 'disk_start': disk_start, 'native64_start': native_start,
                 'sha256': hashlib.sha256(payload).hexdigest(), 'adler32': zlib.adler32(payload)}
        self.records.append(entry)
        if block == 4:
            self.disk, self.native = disk_start + count, native_start + native_count
        return payload, entry

    def string(self, kind):
        end = self.data.find(0, self.source)
        if end < 0:
            raise RuntimeError('Unterminated graph string')
        data, rec = self.record(kind, end + 1 - self.source, disk_align=0)
        self.initialized[('string', rec['disk_start'])] = rec
        return data, rec

    def string_pointer(self, kind, tag):
        if tag == 0xffffffff:
            return self.string(kind)[1]
        if tag:
            return self.reference('string', tag)
        return None

    def reference(self, kind, tag):
        decoded = tag - 1
        block, offset = decoded >> 29, decoded & 0x1fffffff
        target = self.initialized.get((kind, offset))
        if block != 4 or target is None or target['source_end'] > self.source:
            raise RuntimeError(f'Unproven earlier {kind} pointer: {tag:08x}')
        self.fixups.append({'kind': kind, 'tag': tag, 'disk_target': offset,
                            'native64_target': target['native64_start'], 'target_source': target['source_start']})
        return target

    def declaration(self, tag):
        if not tag:
            return None
        if tag != 0xffffffff:
            return self.reference('declaration', tag)
        data, rec = self.record('declaration', 108, 184)
        if data[0] > 16 or data[1] not in (0, 1) or data[2] not in (0, 1) or any(data[36:108]):
            raise RuntimeError('Unsupported declaration routing/resource state')
        rec.update(stream_count=data[0], has_optional_source=bool(data[1]), disk_is_loaded=bool(data[2]),
                   routing_sha256=hashlib.sha256(data[4:36]).hexdigest())
        self.initialized[('declaration', rec['disk_start'])] = rec
        return rec

    def shader(self, kind, tag):
        if not tag:
            return None
        if tag != 0xffffffff:
            return self.reference(kind, tag)
        data, rec = self.record(kind, 16, 32)
        name, runtime, program, words = struct.unpack_from('<IIIH', data)
        if not name or runtime or not program or not words:
            raise RuntimeError('Unsupported genuine shader header')
        self.string_pointer(kind + '_name', name)
        _, code = self.record(kind + '_bytecode', words * 4, disk_align=3)
        rec.update(program_words=words, bytecode_source=code['source_start'], bytecode_sha256=code['sha256'])
        self.initialized[(kind, rec['disk_start'])] = rec
        return rec

    def technique(self, tag, index):
        if not tag:
            return
        if tag != 0xffffffff:
            self.reference('technique', tag)
            return
        data, rec = self.record('technique', 8, 16)
        name, flags, count = struct.unpack('<IHH', data)
        if not name or count != 1:
            raise RuntimeError('Only independently verified one-pass technique supported')
        rec.update(technique_index=index, flags=flags, pass_count=count)
        passes, pass_record = self.record('pass_array', 20 * count, 40 * count, disk_align=0, native_align=0)
        for i in range(count):
            decl, vertex, pixel, prim, obj, stable, sampler, args = struct.unpack_from('<III4BI', passes, 20 * i)
            self.declaration(decl)
            self.shader('vertex_shader', vertex)
            self.shader('pixel_shader', pixel)
            total = prim + obj + stable
            pass_record.update(per_prim=prim, per_object=obj, stable=stable, sampler_flags=sampler)
            if args:
                arg_data, arg_record = self.record('arguments', total * 8, total * 16)
                arg_record['fields'] = []
                for n in range(total):
                    arg_type, dest, value = struct.unpack_from('<HHI', arg_data, n * 8)
                    arg_record['fields'].append({'type': arg_type, 'dest': dest, 'value': value})
                    if arg_type in (1, 7) and value:
                        if value == 0xffffffff:
                            _, literal = self.record('literal', 16)
                            self.initialized[('literal', literal['disk_start'])] = literal
                        else:
                            self.reference('literal', value)
            elif total:
                raise RuntimeError('Arguments absent with nonzero genuine count')
        self.string_pointer('technique_name', name)
        self.initialized[('technique', rec['disk_start'])] = rec

    def parse(self, expected_start):
        xfile = list(struct.unpack_from('<9I', self.data))
        strings, strings_tag, assets, assets_tag = struct.unpack_from('<iIiI', self.data, 36)
        if not 0 <= strings <= 65536 or not 0 < assets <= 65536 or not strings_tag or not assets_tag:
            raise RuntimeError('Unsupported bootstrap list envelope')
        tags, _ = self.record('string_pointer_table', strings * 4, strings * 8, disk_align=0, native_align=0)
        for tag in struct.unpack('<' + 'I' * strings, tags):
            if tag == 0xffffffff:
                self.string('script_string')
            elif tag:
                raise RuntimeError('Unsupported script-string reference')
        directory, directory_record = self.record('asset_directory', assets * 8, assets * 16)
        entries = [struct.unpack_from('<iI', directory, n * 8) for n in range(assets)]
        if any(not 0 <= typ < 43 or tag != 0xffffffff for typ, tag in entries) or entries[0] != (7, 0xffffffff):
            raise RuntimeError('Unsupported real bootstrap directory')
        if self.source != expected_start:
            raise RuntimeError('First bootstrap payload source differs')
        data, root = self.record('technique_set', 528, 1056, block=0)
        if struct.unpack_from('<I', data)[0] != 0xffffffff or any(data[4:8]):
            raise RuntimeError('Unsupported default scalar/name header')
        name, _ = self.string('technique_set_name')
        if name != b'default\0':
            raise RuntimeError('First genuine TechniqueSet is not canonical default')
        tags = struct.unpack_from('<130I', data, 8)
        for i, tag in enumerate(tags):
            self.technique(tag, i)
        return {'xfile_fields': xfile, 'string_count': strings, 'asset_count': assets,
                'directory_source_end': root['source_start'], 'directory': directory_record,
                'next_asset_type': entries[1][0], 'source_end': self.source,
                'block4_disk_end': self.disk, 'block4_native64_end': self.native,
                'null_techniques': tags.count(0), 'inline_techniques': [i for i, t in enumerate(tags) if t == 0xffffffff],
                'records': self.records, 'fixups': self.fixups,
                'prefix_sha256': hashlib.sha256(self.data[:self.source]).hexdigest()}


def reference(source):
    key = Path(source).stem
    if key not in KNOWN:
        raise RuntimeError('Original is outside audited bootstrap input set')
    data, provenance = decode(source, KNOWN[key])
    if struct.unpack_from('<I', data)[0] + 36 != provenance['decoded_bytes']:
        raise RuntimeError('Original XFile size differs from complete decoded extent')
    graph = Graph(data).parse(KNOWN[key][3])
    return {'schema': 'BlackOpsARM.bootstrap-default-oracle.v1', 'original_path': str(Path(source).resolve()),
            **provenance, **graph}


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('source', type=Path)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    result = reference(args.source)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('Verified complete retail provenance and first bootstrap default graph:', result['source_end'])
