"""Independent oracle rejection controls, using copies of bounded retail prefixes."""
import argparse
import json
import struct
from pathlib import Path
from bootstrap_default_oracle import Graph, KNOWN, decode


def check(source):
    expected = KNOWN[Path(source).stem]
    data, provenance = decode(source, expected)
    valid = Graph(data).parse(expected[3])
    records = valid['records']
    root = next(r['source_start'] for r in records if r['kind'] == 'technique_set')
    technique = next(r['source_start'] for r in records if r['kind'] == 'technique')
    declaration = next(r['source_start'] for r in records if r['kind'] == 'declaration')
    vertex = next(r['source_start'] for r in records if r['kind'] == 'vertex_shader')
    pixel = next(r['source_start'] for r in records if r['kind'] == 'pixel_shader')
    directory = valid['directory']['source_start']
    controls = [
        ('negative_string_count', 36, '<i', -1),
        ('absent_asset_directory', 48, '<I', 0),
        ('unsupported_string_tag', 52 + 4, '<I', 123),
        ('wrong_first_asset_type', directory, '<i', 5),
        ('unsupported_directory_tag', directory + 4, '<I', 0xfffffffe),
        ('absent_default_name', root, '<I', 0),
        ('unsupported_default_scalar', root + 4, '<I', 1),
        ('future_technique_reference', root + 8, '<I', 0x80000001 + valid['block4_disk_end']),
        ('wrong_block_technique_reference', root + 8, '<I', 1),
        ('interior_technique_reference', root + 8 + 5 * 4, '<I', 0x80000001 + valid['fixups'][1]['disk_target'] + 1),
        ('unsupported_pass_count', technique + 6, '<H', 2),
        ('unsupported_stream_count', declaration, '<B', 17),
        ('retail_runtime_handle', vertex + 4, '<I', 1),
        ('absent_vertex_program', vertex + 8, '<I', 0),
        ('oversized_vertex_program', vertex + 12, '<H', 65535),
        ('future_shader_name', pixel, '<I', 0x80000001 + valid['block4_disk_end']),
    ]
    passed = []
    for label, offset, fmt, value in controls:
        bad = bytearray(data)
        struct.pack_into(fmt, bad, offset, value)
        try:
            Graph(bad).parse(expected[3])
        except RuntimeError:
            passed.append(label)
        else:
            raise RuntimeError('Malformed bounded graph accepted: ' + label)
    for label, end in [('truncated_header', root + 527), ('truncated_final_name', valid['source_end'] - 1)]:
        try:
            Graph(data[:end]).parse(expected[3])
        except RuntimeError:
            passed.append(label)
        else:
            raise RuntimeError('Truncated graph accepted: ' + label)
    return {'original_path': str(Path(source).resolve()), **provenance,
            'source_end': valid['source_end'], 'rejected_controls': passed,
            'metadata_only': True, 'genuine_native_execution': False}


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('--game-root', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    results = [check(args.game_root / 'zone/common' / (name + '.ff')) for name in KNOWN]
    args.output.write_text(json.dumps({'status': 'passed', 'targets': results}, indent=2) + '\n', encoding='utf-8')
    print('Independent bootstrap graph controls rejected:', sum(len(r['rejected_controls']) for r in results))
