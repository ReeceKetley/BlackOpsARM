"""Strict complete-zone runtime collector using independent retail metadata."""
import argparse
import json
from pathlib import Path
from startup_zone_oracle import inspect


def validate(stdout, pointer_bytes, oracle, thread_integration=False):
    if pointer_bytes not in (4, 8):
        raise ValueError('Unsupported runtime ABI')
    lines = stdout.splitlines()
    if thread_integration:
        marker = json.loads(lines.pop(0))
        expected = {'startup_thread_integration': {'pointer_bytes': pointer_bytes,
                    'thread_context': 0, 'canonical_slots': 4,
                    'actual_cmd_reset': True, 'com_init_executed': False}}
        if marker != expected or set(marker) != set(expected) or any(type(marker['startup_thread_integration'][key]) is not type(value)
                for key, value in expected['startup_thread_integration'].items()):
            raise ValueError('Actual main-thread integration record differs')
    if len(lines) != 4 * 72:
        raise ValueError('Expected four complete 71-asset cycles')
    expected_queries = 2 * sum(row.get('cell_count', 0) for row in oracle['assets'])
    for cycle in range(4):
        for index, reference in enumerate(oracle['assets']):
            record = json.loads(lines[cycle * 72 + index])
            if set(record) != {'startup_zone_asset'}:
                raise ValueError('Unexpected runtime record schema')
            actual = record['startup_zone_asset']
            expected = {'cycle': cycle, 'index': index, 'type': reference['type'],
                        'source_start': reference['source_start'], 'source_end': reference['source_end'],
                        'name_bytes': reference['name']['bytes'],
                        'name_adler32': reference['name']['adler32'],
                        'disk_end': reference['block4_disk_end'],
                        'native_end': reference['block4_native64_end'] if pointer_bytes == 8 else reference['block4_disk_end']}
            if reference['type'] == 36:
                expected.update(length=reference['length'], buffer_bytes=reference['buffer_bytes'],
                                buffer_adler32=reference['buffer_adler32'])
            else:
                expected.update(rows=reference['rows'], columns=reference['columns'],
                                cells=reference['cell_count'], query_rows=reference['query_rows'],
                                cell_hash_adler32=reference['cell_hash_adler32'],
                                cell_string_adler32=reference['cell_string_adler32'],
                                normalized_index_adler32=reference['normalized_index_adler32'])
            if set(actual) != set(expected):
                raise ValueError('Asset fields differ: ' + str(index))
            for key, value in expected.items():
                if type(actual[key]) is not type(value) or actual[key] != value:
                    raise ValueError(f'Asset {index} field {key} differs')
                if isinstance(value, list) and any(type(item) is not int for item in actual[key]):
                    raise ValueError('Query row is not an integer')
        record = json.loads(lines[cycle * 72 + 71])
        expected = {'cycle': cycle, 'assets': 71, 'decoded_consumed': oracle['decoded_bytes'],
                    'queries': expected_queries, 'high_water': 71, 'free_entries': 32752,
                    'controlled_caller_inputs': True, 'com_init_executed': False}
        if set(record) != {'startup_zone_cycle'} or set(record['startup_zone_cycle']) != set(expected):
            raise ValueError('Unexpected cycle schema')
        if any(type(record['startup_zone_cycle'][key]) is not type(value)
               or record['startup_zone_cycle'][key] != value for key, value in expected.items()):
            raise ValueError('Cycle evidence differs')
    result = {'passed': True, 'pointer_bytes': pointer_bytes, 'cycles': 4,
            'registered_asset_records': 284, 'queries_per_cycle': expected_queries,
            'complete_source_bytes': oracle['decoded_bytes'], 'controlled_caller_inputs': True,
            'com_init_executed': False}
    if thread_integration:
        result['actual_thread_data_integrated'] = True
        result['actual_cmd_reset'] = True
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('runtime_log')
    parser.add_argument('--pointer-bytes', type=int, required=True)
    parser.add_argument('--retail', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--thread-integration', action='store_true')
    args = parser.parse_args()
    output = Path(args.output)
    if output.exists():
        raise SystemExit('Use fresh collector evidence')
    result = validate(Path(args.runtime_log).read_text(encoding='utf-8-sig'), args.pointer_bytes, inspect(args.retail), args.thread_integration)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('All 284 registered assets and complete table queries match independent oracle')
