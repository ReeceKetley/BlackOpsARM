"""Adversarial checks for the independent full MP startup metadata oracle."""
from pathlib import Path
import argparse
import json
import struct
import zlib
from startup_zone_oracle import inspect, inspect_payload


def check(path):
    result = inspect(path)
    data = zlib.decompress(Path(path).read_bytes()[12:])
    rows = result['assets']
    if [r['index'] for r in rows] != list(range(71)):
        raise RuntimeError('Directory coverage incomplete')
    if rows[0]['source_start'] != 620 or rows[-1]['source_end'] != 150108:
        raise RuntimeError('Whole source endpoints differ')
    if any(a['source_end'] != b['source_start'] for a, b in zip(rows, rows[1:])):
        raise RuntimeError('Asset extents are not contiguous')
    cases = []

    def mutate(label, offset, fmt, value):
        candidate = bytearray(data)
        struct.pack_into(fmt, candidate, offset, value)
        cases.append((label, candidate))

    mutate('xfile-size', 0, '<I', 0)
    mutate('script-count', 36, '<I', 1)
    mutate('asset-count', 44, '<I', 70)
    mutate('asset-type', 52, '<i', 8)
    mutate('root-null', 56, '<I', 0)
    table = next(r for r in rows if r['type'] == 37)
    raw = next(r for r in rows if r['type'] == 36)
    mutate('table-negative-columns', table['source_start'] + 4, '<i', -1)
    mutate('table-values-null', table['source_start'] + 12, '<I', 0)
    mutate('raw-negative-length', raw['source_start'] + 4, '<i', -1)
    mutate('raw-buffer-null', raw['source_start'] + 8, '<I', 0)
    cell_start = table['name']['source_end']
    mutate('cell-uninitialized-alias', cell_start, '<I', 0x80000001)
    mutate('index-outside-table', table['source_end'] - 2, '<h', -1)
    cases.extend([('truncated-source', data[:-1]), ('extra-source', data + b'\0')])
    rejected = []
    for label, candidate in cases:
        try:
            inspect_payload(candidate)
        except ValueError as error:
            rejected.append({'case': label, 'reason': str(error)})
        else:
            raise RuntimeError('Malformed oracle control accepted: ' + label)
    return {'schema': 'BlackOpsARM.startup-zone-oracle-controls.v1',
            'passed': True, 'genuine_assets': len(rows),
            'rejected_controls': rejected, 'production_functions_executed': False}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('retail_path')
    parser.add_argument('output')
    args = parser.parse_args()
    result = check(args.retail_path)
    output = Path(args.output)
    if output.exists():
        raise SystemExit('Use fresh evidence path')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(f"Oracle genuine coverage and {len(result['rejected_controls'])} malformed controls pass")
