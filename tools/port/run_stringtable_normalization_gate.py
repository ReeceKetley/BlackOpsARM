"""Reproduce original-compatible index normalization and actual StringTable queries.

Original assets already on HONOR are SHA-verified and reused. Native decoder
processes finish before zone allocations. Evidence contains no string text/hex;
the small original prefixes stay in ignored local/device scratch directories.
"""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import os
import shlex
import struct
import subprocess
import traceback

from stringtable_payload_oracle import reference, sha

ROOT = Path(__file__).resolve().parents[2]
ASSETS = {'code_pre_gfx': 'retained.inflate-genuine-oracle.json',
          'zombie_pentagon': 'retained.inflate-zombie-oracle.json'}


def normalized_query_reference(oracle, original_asset):
    """Use original executed lower-bound results and its digest-based row model."""
    source = oracle['stringtable_lookup_records']
    original = original_asset['normalized_queries']
    if len(original['query_records']) != len(source) or original['query_count'] != len(source):
        raise RuntimeError('Missing/extra original normalized query reference')
    queries = []
    for old, normalized in zip(source, original['query_records']):
        if old['index'] != normalized['index'] or old['hash'] != normalized['hash']:
            raise RuntimeError('Original normalized query identity/hash mismatch')
        query = dict(old)
        query.update({'first_hash_index': normalized['first_hash_index'],
                      'row': normalized['source_row_model'], 'linear_row': normalized['linear_row']})
        queries.append(query)
    misses = sum(query['row'] == -1 for query in queries)
    disagreements = sum(query['row'] != query['linear_row'] for query in queries)
    if misses != original['misses'] or disagreements != original['row_disagreements']:
        raise RuntimeError('Original normalized counters disagree with original query records')
    return {'queries': queries, 'row_queries': len(queries), 'row_misses': misses,
            'row_disagreements': disagreements, 'column_queries': len(queries),
            'column_disagreements': sum(query['column'] != query['linear_column'] for query in queries),
            'casefold_row_agrees': True, 'casefold_column_agrees': True}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for name in ['android-build', 'host-build', 'output']:
        ap.add_argument('--' + name, type=Path, required=True)
    ap.add_argument('--serial', default='AKWJ9X5116G04618')
    ap.add_argument('--adb', type=Path, default=Path('D:/Android/Sdk/platform-tools/adb.exe'))
    ap.add_argument('--retail-root', type=Path, default=Path('D:/Games/Call of Duty - Black Ops'))
    ap.add_argument('--reuse-results', type=Path, default=ROOT / 'build-android-audit/asset_directory_run1/results.json')
    ap.add_argument('--original-results', type=Path, default=ROOT / 'build-android-audit/original_stringtable_reference_run3/results.json')
    args = ap.parse_args()
    out = args.output.resolve()
    if not out.is_relative_to(ROOT) or out.exists():
        raise RuntimeError('Use a fresh output directory inside BlackOpsARM')
    out.mkdir(parents=True)
    invocations = []
    env = dict(os.environ)
    for key in ['BO1_STREAM_TRACE', 'BO1_TRACE_ZONE', 'BO1_TRACE_ASSET', 'BO1_TRACE_STACK_OP']:
        env.pop(key, None)
    env['BO1_DIAG_LOG'] = str(out / 'diagnostics.log')

    def write_json(name, value):
        (out / name).write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')

    def run(label, argv):
        argv = [str(a) for a in argv]
        timed_out, launch_error = False, None
        try:
            completed = subprocess.run(argv, cwd=out, env=env, capture_output=True, text=True,
                                       encoding='utf-8', errors='replace', timeout=180)
            stdout, stderr, code = completed.stdout, completed.stderr, completed.returncode
        except subprocess.TimeoutExpired as error:
            def readable(value):
                return value.decode('utf-8', errors='replace') if isinstance(value, bytes) else (value or '')
            stdout, stderr, code = readable(error.stdout), readable(error.stderr), None
            timed_out = True
        except OSError as error:
            stdout, stderr, code = '', str(error), None
            launch_error = str(error)
        for suffix, value in [('stdout', stdout), ('stderr', stderr)]:
            (out / f'{label}.{suffix}.log').write_text(value, encoding='utf-8')
        invocations.append({'label': label, 'argv': argv, 'cwd': str(out), 'exit': code,
                            'timed_out': timed_out, 'launch_error': launch_error})
        write_json('invocations.json', invocations)
        if code != 0:
            raise RuntimeError(f'{label}: exit {code}; diagnostics retained')
        return stdout

    adb = [args.adb, '-s', args.serial]

    def shell(label, words):
        return run(label, adb + ['shell', ' '.join(shlex.quote(str(word)) for word in words)])

    result = {'schema': 'BlackOpsARM.TASK-0025.stringtable-normalization-gate.v1',
              'recorded_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'device_serial': args.serial, 'oracles': {}, 'platforms': {},
              'scope': 'Actual first StringTable normalization and queries against verified original CRT/reference order; no asset registry or rendering',
              'reference_scope': 'Independent payload provenance plus unmodified original sort/FirstHashIndex execution; original row reference uses lowercase digests. Execution ends after first asset.',
              'alias_scope': 'Specific genuine side-envelope alias12 replacement and retained alias4 only; no general duplicate-key ordering guarantee',
              'reader_boundary': 'Finite checked fixture reader, separate complete native decode process; production asynchronous/refill pipeline remains unvalidated',
              'original_index_order_scope': 'Actual Load_shortArray preserves original order before the explicitly measured normalization step',
              'original_byte_buffers_in_evidence': False}
    try:
        original = json.loads(args.original_results.read_text(encoding='utf-8'))
        if (original.get('schema') != 'BlackOpsARM.TASK-0024.original-stringtable-reference.v1'
                or original.get('status') != 'passed' or original.get('stubbed_services') != []
                or original.get('exe_sha256') != '2ba4a04e81de2b60582da0e4489efde21e6cb07904a223bae633b6ccaebbf759'
                or set(original.get('assets', {})) != set(ASSETS)):
            raise RuntimeError('Missing/unsupported unchanged original instruction reference')
        result['original_reference'] = {'path': str(args.original_results.resolve()),
                                        'sha256': sha(args.original_results), 'exe_sha256': original['exe_sha256'],
                                        'locator_sha256': original['locator_sha256'], 'functions': original['functions'],
                                        'query_scope': original['query_scope'], 'stubbed_services': original['stubbed_services']}
        native = args.android_build.resolve() / 'bo1_stringtable_normalization_tests'
        host_binaries = {arch: args.host_build.resolve() / arch / 'stringtable_normalization.exe' for arch in ['x86', 'x64']}
        for binary in [native, *host_binaries.values()]:
            if not binary.is_file():
                raise RuntimeError('Missing StringTable lookup gate executable: ' + str(binary))
        reused = json.loads(args.reuse_results.read_text(encoding='utf-8'))
        old_device = reused.get('reused_original_device_directory', reused['device_directory'])
        if (reused['schema'] not in ('BlackOpsARM.TASK-0016.asset-directory-gate.v1', 'BlackOpsARM.TASK-0015.raw-strings-gate.v1')
                or reused['device_serial'] != args.serial or reused.get('all_tests_completed') is not True
                or not old_device.startswith('/data/local/tmp/blackopsarm-phase1-string-table-')
                or '/' in old_device.removeprefix('/data/local/tmp/')):
            raise RuntimeError('Invalid previous device-original provenance')
        result.update({'reused_original_device_directory': old_device,
                       'reuse_results_path': str(args.reuse_results.resolve()), 'reuse_results_sha256': sha(args.reuse_results)})
        normalized_queries = {}
        for asset, leaf in ASSETS.items():
            oracle_path = ROOT / 'docs/port/evidence/arm64_inflate' / leaf
            oracle = json.loads(oracle_path.read_text(encoding='utf-8'))
            ref = reference(args.retail_root / 'zone/common' / f'{asset}.ff', oracle)
            original_asset = original['assets'][asset]
            if (any(original_asset[key] != ref[key] for key in ['raw_sha256', 'decoded_sha256', 'prefix_bytes', 'prefix_sha256'])
                    or original_asset['original_indices'] != ref['stringtable_indices']['values']
                    or original_asset['original_indices_sha256'] != ref['stringtable_indices']['sha256']
                    or original_asset['cell_count'] != ref['stringtable_cell_count']
                    or original_asset['column_count'] != ref['stringtable_header']['column_count']):
                raise RuntimeError('Original sort reference genuine provenance differs')
            ref.update({'execution_prefix_bytes': ref['prefix_bytes'],
                        'execution_prefix_sha256': ref['prefix_sha256']})
            ref.update({'oracle_path': oracle_path.relative_to(ROOT).as_posix(), 'oracle_sha256': sha(oracle_path)})
            if reused['oracles'][asset]['raw_sha256'] != ref['raw_sha256']:
                raise RuntimeError('Previous original raw digest differs')
            result['oracles'][asset] = ref
            normalized_queries[asset] = normalized_query_reference(ref, original_asset)
            write_json('independent-oracles.json', result['oracles'])
        device = '/data/local/tmp/blackopsarm-phase1-stringtable-normalization-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S%f')
        result['device_directory'] = device
        write_json('results.json', result)
        # Verify reusable originals before pushing the new small executable.
        for asset, ref in result['oracles'].items():
            if shell(asset + '-reused-device-sha', ['sha256sum', f'{old_device}/{asset}.ff']).split()[0] != ref['raw_sha256']:
                raise RuntimeError('Reusable device original hash mismatch')
        shell('device-create', ['mkdir', device])
        run('device-metadata', adb + ['shell', 'getprop ro.product.model; getprop ro.build.version.sdk; uname -a; cat /proc/meminfo'])
        run('device-probe-push', adb + ['push', native, device + '/probe'])
        shell('device-probe-chmod', ['chmod', '700', device + '/probe'])
        if shell('device-probe-sha', ['sha256sum', device + '/probe']).split()[0] != sha(native):
            raise RuntimeError('Device executable hash mismatch')
        for target in ['windows_x86', 'windows_x64', 'honor_arm64']:
            on_device = target == 'honor_arm64'
            binary = device + '/probe' if on_device else str(host_binaries[target.split('_')[1]])

            def execute(label, params):
                return shell(target + '-' + label, [binary] + params) if on_device else run(target + '-' + label, [binary] + params)

            params, decodes = ['gate'], {}
            for asset, ref in result['oracles'].items():
                prefix = f'{device}/{asset}.prefix' if on_device else str(out / f'{target}-{asset}.prefix')
                source = f'{old_device}/{asset}.ff' if on_device else ref['original_path']
                print(target, asset, 'separate full decode and bounded first-StringTable prefix validation', flush=True)
                decoded = json.loads(execute(asset + '-decode', ['decode', source, prefix]))
                decode_keys = {'decoded_bytes', 'decoded_adler32', 'vector_capacity_bytes', 'prefix_bytes',
                               'decoded_buffer_released'}
                if (not isinstance(decoded, dict) or set(decoded) != decode_keys
                        or any(type(decoded[key]) is not int for key in decode_keys - {'decoded_buffer_released'})
                        or decoded['decoded_bytes'] != ref['decoded_bytes'] or decoded['decoded_adler32'] != ref['decoded_adler32']
                        or decoded['prefix_bytes'] != ref['execution_prefix_bytes'] or decoded['decoded_buffer_released'] is not True
                        or decoded['vector_capacity_bytes'] < ref['decoded_bytes']
                        or any(key in decoded for key in ['prefix_hex', 'string_text', 'string_hex'])):
                    raise RuntimeError('Native full decode/prefix metadata differs or emits original bytes')
                digest = shell(target + '-' + asset + '-prefix-sha', ['sha256sum', prefix]).split()[0] if on_device else sha(prefix)
                if digest != ref['execution_prefix_sha256']:
                    raise RuntimeError('Native asset-directory prefix file hash differs')
                decodes[asset] = decoded
                params.extend([prefix, str(ref['decoded_bytes'])])
            rows = [json.loads(line) for line in execute('stringtable-normalization', params).splitlines() if line.strip()]
            fixtures, cycles, summaries, measurement_records, directory_records, header_records, cell_records, name_records, resolved_records, index_records, hash_records, vector_records, comparison_records, lookup_fixture_records, lookup_records, normalization_fixture_records, normalization_records = ([], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [], [])
            expected_record_order = ['fixture'] * 138 + ['hash_vectors', 'comparison_vectors', 'lookup_fixtures', 'normalization_fixtures']
            first_asset_records = ['directory_measurements', 'cell_measurements', 'resolved_string_measurements',
                                   'index_measurements', 'hash_measurements', 'normalization_measurements', 'lookup_measurements',
                                   'name_measurements', 'header_measurements', 'asset_index']
            expected_record_order.extend(first_asset_records)
            expected_record_order.append('string_measurements')
            expected_record_order.extend(first_asset_records)
            expected_record_order.extend(['asset_index'] * 10 + ['schema'])
            if len(rows) != len(expected_record_order):
                raise RuntimeError('Missing or extra lookup gate records')
            for index, row in enumerate(rows):
                if not isinstance(row, dict):
                    raise RuntimeError('Gate record must be a JSON object')
                record_categories = [key for key in ['fixture', 'hash_vectors', 'comparison_vectors', 'lookup_fixtures',
                                     'normalization_fixtures', 'normalization_measurements',
                                     'directory_measurements', 'cell_measurements', 'resolved_string_measurements',
                                     'index_measurements', 'hash_measurements', 'lookup_measurements', 'name_measurements',
                                     'header_measurements', 'asset_index', 'string_measurements', 'schema']
                                     if key in row and not (key == 'asset_index' and 'directory_measurements' in row)]
                if record_categories != [expected_record_order[index]]:
                    raise RuntimeError('Unknown, ambiguous or out-of-order lookup gate record')
                if 'hash_vectors' in row:
                    if (set(row) != {'hash_vectors'} or vector_records or cycles or summaries
                            or measurement_records or directory_records or len(fixtures) != 138):
                        raise RuntimeError('Unexpected/duplicate hash-vector record or incorrect prerequisite order')
                    vector_records.append(row['hash_vectors'])
                    continue
                if 'comparison_vectors' in row:
                    if set(row) != {'comparison_vectors'}:
                        raise RuntimeError('Unknown comparison vector fields')
                    comparison_records.append(row['comparison_vectors'])
                    continue
                if 'lookup_fixtures' in row:
                    if set(row) != {'lookup_fixtures'}:
                        raise RuntimeError('Unknown controlled lookup fixture fields')
                    lookup_fixture_records.append(row['lookup_fixtures'])
                    continue
                if 'normalization_fixtures' in row:
                    if set(row) != {'normalization_fixtures'}:
                        raise RuntimeError('Unknown normalization fixture fields')
                    normalization_fixture_records.append(row['normalization_fixtures'])
                    continue
                if 'normalization_measurements' in row:
                    if (set(row) != {'normalization_measurements'} or summaries or len(cycles) not in (0, 1)
                            or len(normalization_records) != len(cycles) or len(hash_records) != len(cycles) + 1):
                        raise RuntimeError('Unexpected/duplicate normalization measurements record')
                    normalization_records.append(row['normalization_measurements'])
                    continue
                if 'lookup_measurements' in row:
                    if (set(row) != {'lookup_measurements'} or summaries or len(cycles) not in (0, 1)
                            or len(lookup_records) != len(cycles) or len(hash_records) != len(cycles) + 1
                            or len(normalization_records) != len(cycles) + 1):
                        raise RuntimeError('Unexpected/duplicate actual lookup measurements record')
                    lookup_records.append(row['lookup_measurements'])
                    continue
                if 'directory_measurements' in row:
                    if (set(row) - {'asset_index', 'directory_measurements'} or summaries or len(cycles) not in (0, 1)
                            or len(directory_records) != len(cycles)):
                        raise RuntimeError('Unexpected/duplicate directory measurements record')
                    if 'asset_index' in row and row['asset_index'] != len(cycles):
                        raise RuntimeError('Directory measurements have incorrect asset identity')
                    directory_records.append(row)
                    continue
                if 'header_measurements' in row:
                    if (set(row) != {'header_measurements'} or summaries or len(cycles) not in (0, 1)
                            or len(header_records) != len(cycles) or len(directory_records) != len(cycles) + 1
                            or len(name_records) != len(cycles) + 1):
                        raise RuntimeError('Unexpected/duplicate first-header measurements record')
                    header_records.append(row['header_measurements'])
                    continue
                if 'cell_measurements' in row:
                    if (set(row) != {'cell_measurements'} or summaries or len(cycles) not in (0, 1)
                            or len(cell_records) != len(cycles) or len(directory_records) != len(cycles) + 1):
                        raise RuntimeError('Unexpected/duplicate unresolved cell measurements record')
                    cell_records.append(row['cell_measurements'])
                    continue
                if 'name_measurements' in row:
                    if (set(row) != {'name_measurements'} or summaries or len(cycles) not in (0, 1)
                            or len(name_records) != len(cycles) or len(cell_records) != len(cycles) + 1
                            or len(resolved_records) != len(cycles) + 1 or len(index_records) != len(cycles) + 1
                            or len(hash_records) != len(cycles) + 1 or len(lookup_records) != len(cycles) + 1):
                        raise RuntimeError('Unexpected/duplicate StringTable name measurements record')
                    name_records.append(row['name_measurements'])
                    continue
                if 'resolved_string_measurements' in row:
                    if (set(row) != {'resolved_string_measurements'} or summaries or len(cycles) not in (0, 1)
                            or len(resolved_records) != len(cycles) or len(cell_records) != len(cycles) + 1):
                        raise RuntimeError('Unexpected/duplicate resolved cell-string measurements record')
                    resolved_records.append(row['resolved_string_measurements'])
                    continue
                if 'index_measurements' in row:
                    if (set(row) != {'index_measurements'} or summaries or len(cycles) not in (0, 1)
                            or len(index_records) != len(cycles) or len(resolved_records) != len(cycles) + 1):
                        raise RuntimeError('Unexpected/duplicate original signed16 index measurements record')
                    index_records.append(row['index_measurements'])
                    continue
                if 'hash_measurements' in row:
                    if (set(row) != {'hash_measurements'} or summaries or len(cycles) not in (0, 1)
                            or len(hash_records) != len(cycles) or len(index_records) != len(cycles) + 1):
                        raise RuntimeError('Unexpected/duplicate actual stored/computed hash measurements record')
                    hash_records.append(row['hash_measurements'])
                    continue
                categories = [key for key in ['fixture', 'asset_index', 'schema', 'string_measurements'] if key in row]
                if len(categories) != 1:
                    raise RuntimeError('Unknown or ambiguous asset-directory record')
                category = categories[0]
                if category == 'fixture':
                    fixture_keys = {'fixture', 'status', 'no_mutation'}
                    if row['fixture'] in {'directory_actual', 'header_actual', 'cells_actual',
                                           'initialized_actual', 'indices_actual'}:
                        fixture_keys.add('cases')
                    if set(row) != fixture_keys:
                        raise RuntimeError('Unknown or missing fixture fields')
                    if (cycles or summaries or not isinstance(row.get('status'), str)
                            or not isinstance(row.get('no_mutation'), bool)):
                        raise RuntimeError('Invalid preflight/no-mutation fixture record')
                    fixtures.append(row)
                elif category == 'asset_index':
                    if summaries:
                        raise RuntimeError('Genuine cycle emitted after summary')
                    cycles.append(row)
                elif category == 'string_measurements':
                    if (set(row) != {'string_measurements'} or measurement_records or summaries or len(cycles) != 1
                            or (cycles[0]['asset_index'], cycles[0]['round'], cycles[0]['alloc_type']) != (0, 0, 0)):
                        raise RuntimeError('Per-string measurements emitted at unexpected/duplicate cycle')
                    measurement_records.append(row)
                else:
                    summary_keys = {'schema', 'pointer_bytes', 'fixtures_passed', 'genuine_cycles_passed',
                                    'logical_baseline_restored'}
                    if (set(row) != summary_keys or row['schema'] != 'BlackOpsARM.TASK-0023.stringtable-lookup.v1'
                            or index != len(rows) - 1):
                        raise RuntimeError('Unknown/nonterminal summary schema')
                    summaries.append(row)
            if len(summaries) != 1:
                raise RuntimeError('Expected exactly one asset-directory gate summary')
            summary = summaries[0]
            pointer_bytes = 4 if target == 'windows_x86' else 8
            if len(vector_records) != 1:
                raise RuntimeError('Missing actual independent/original hash-vector evidence')
            vectors = vector_records[0]
            vector_keys = {'vectors', 'overflow_vectors', 'original_windows_vectors', 'actual_wrapper_vectors',
                           'independent_ascii_oracle_matches', 'no_mutation'}
            if (not isinstance(vectors, dict) or set(vectors) != vector_keys
                    or vectors['vectors'] != 12857 or vectors['actual_wrapper_vectors'] != 12857
                    or vectors['original_windows_vectors'] != (0 if on_device else 12857)
                    or type(vectors['overflow_vectors']) is not int or vectors['overflow_vectors'] <= 1000
                    or vectors['independent_ascii_oracle_matches'] is not True or vectors['no_mutation'] is not True):
                raise RuntimeError('Actual wrapping/hash-wrapper/original Windows vector contract failed')
            expected_comparison = {'cases': 262180, 'unsigned_byte_domain': 256,
                                   'actual_original_bodies': True, 'independent_oracle_matches': True,
                                   'no_mutation': True}
            expected_lookup_fixture = {'cases': 22, 'actual_original_bodies': True, 'real_hash_collision': True,
                                       'controlled_error_queries': 2, 'no_mutation': True}
            if comparison_records != [expected_comparison]:
                raise RuntimeError('Actual unsigned-byte/prefix comparison vector contract failed')
            if lookup_fixture_records != [expected_lookup_fixture]:
                raise RuntimeError('Actual controlled collision/error lookup fixture contract failed')
            normalization_fixture_keys = {'cases', 'sorted_cases', 'order_digest', 'concurrent_tables', 'concurrent_passes',
                                          'no_global_context', 'native_width_positions', 'invalid_no_mutation'}
            if len(normalization_fixture_records) != 1:
                raise RuntimeError('Expected exactly one normalization fixture record')
            normalization_fixture = normalization_fixture_records[0]
            if (not isinstance(normalization_fixture, dict) or set(normalization_fixture) != normalization_fixture_keys
                    or any(type(normalization_fixture[key]) is not int for key in
                           ['cases', 'sorted_cases', 'order_digest', 'concurrent_tables', 'concurrent_passes'])
                    or normalization_fixture['cases'] != 86 or normalization_fixture['sorted_cases'] != 60
                    or normalization_fixture['concurrent_tables'] != 2 or normalization_fixture['concurrent_passes'] != 16
                    or not 0 <= normalization_fixture['order_digest'] <= 0xffffffff
                    or any(normalization_fixture[key] is not True for key in
                           ['no_global_context', 'native_width_positions', 'invalid_no_mutation'])):
                raise RuntimeError('Actual normalization fixture/domain/concurrency contract failed')
            statuses = ['ok', 'ok', 'ok', 'unterminated', 'invalid_source', 'unterminated',
                        'unsupported_tag', 'unsupported_tag', 'unsupported_tag', 'unsupported_tag',
                        'disk_bounds', 'native_bounds', 'native_bounds' if pointer_bytes == 8 else 'ok', 'disk_bounds',
                        'native_bounds', 'native_bounds', 'commit_overflow']
            expected_fixtures = [(f'preflight_{number}', status, True) for number, status in enumerate(statuses, 1)]
            expected_fixtures.extend([('short_source', 'unterminated', True),
                                      ('short_destination', 'native_bounds', True),
                                      ('invalid_lengths', 'invalid_source', True),
                                      ('exact_end', 'ok', False),
                                      ('exhausted', 'unterminated', True),
                                      ('overlap', 'invalid_source', True),
                                      ('injected_failure', 'invalid_source', True)])
            directory_statuses = ['negative_count', 'ok', 'ok', 'native_bounds', 'source_bounds',
                                  'invalid_source', 'disk_bounds', 'native_bounds', 'ok', 'disk_bounds',
                                  'disk_bounds', 'native_bounds',
                                  'helper_domain' if pointer_bytes == 8 else 'commit_overflow',
                                  'invalid_type', 'invalid_type', 'invalid_type']
            expected_fixtures.extend([(f'directory_preflight_{number}', status, True)
                                      for number, status in enumerate(directory_statuses, 1)])
            expected_fixtures.append(('directory_actual', 'ok', False))
            header_statuses = ['unsupported_asset', 'ok', 'unsupported_tag', 'unsupported_tag', 'unsupported_tag',
                               'short_header', 'short_header', 'source_bounds', 'negative_dimension', 'negative_dimension',
                               'cell_count_overflow', 'cell_count_overflow', 'helper_domain' if pointer_bytes == 8 else 'ok',
                               'source_bounds', 'disk_bounds', 'native_bounds', 'disk_bounds', 'native_bounds',
                               'disk_bounds', 'native_bounds', 'disk_bounds', 'native_bounds', 'ok']
            expected_fixtures.extend([(f'header_preflight_{number}', status, True)
                                      for number, status in enumerate(header_statuses, 1)])
            expected_fixtures.append(('header_actual', 'ok', False))
            cells_statuses = ['negative_dimension', 'negative_dimension', 'count_overflow', 'ok',
                              'native_bounds', 'ok', 'source_bounds', 'invalid_source', 'disk_bounds',
                              'native_bounds', 'disk_bounds', 'native_bounds', 'disk_bounds', 'native_bounds',
                              'helper_domain' if pointer_bytes == 8 else 'source_bounds',
                              'helper_domain' if pointer_bytes == 8 else 'commit_overflow',
                              'commit_overflow', 'commit_overflow']
            expected_fixtures.extend([(f'cells_preflight_{number}', status, True)
                                      for number, status in enumerate(cells_statuses, 1)])
            expected_fixtures.append(('cells_actual', 'ok', False))
            initialized_statuses = ['absent', 'inline', 'unsupported_tag', 'wrong_block', 'wrong_block', 'wrong_block', 'ok',
                                    'uninitialized', 'uninitialized', 'uninitialized', 'uninitialized', 'uninitialized',
                                    'invalid_owner', 'invalid_owner', 'stale_owner', 'stale_owner',
                                    'invalid_string', 'invalid_string', 'invalid_string', 'disk_bounds', 'native_bounds',
                                    'native_bounds', 'disk_bounds', 'invalid_address', 'native_bounds', 'ambiguous_start']
            expected_fixtures.extend([(f'initialized_preflight_{number}', status, True)
                                      for number, status in enumerate(initialized_statuses, 1)])
            expected_fixtures.append(('initialized_actual', 'ok', False))
            indices_statuses = ['negative_dimension', 'negative_dimension', 'count_overflow', 'ok', 'ok',
                                'native_bounds', 'ok', 'source_bounds', 'invalid_source', 'disk_bounds',
                                'native_bounds', 'disk_bounds', 'native_bounds', 'disk_bounds', 'native_bounds',
                                'index_domain', 'commit_overflow', 'commit_overflow', 'commit_overflow',
                                'disk_bounds', 'native_bounds', 'ok', 'index_range', 'index_range', 'duplicate_index', 'ok']
            expected_fixtures.extend([(f'indices_preflight_{number}', status, True)
                                      for number, status in enumerate(indices_statuses, 1)])
            expected_fixtures.append(('indices_actual', 'ok', False))
            if len(fixtures) != len(expected_fixtures):
                raise RuntimeError('Missing or extra raw-string/directory fixture records')
            for row, (name, status, unchanged) in zip(fixtures, expected_fixtures):
                if (row['fixture'] != name or row['status'] != status or row['no_mutation'] is not unchanged):
                    raise RuntimeError('Raw-string fixture identity/status/mutation differs: ' + str(row))
            if fixtures[40].get('cases') != 7:
                raise RuntimeError('Actual directory fixture must verify all seven runtime cases')
            if fixtures[64].get('cases') != 3:
                raise RuntimeError('Actual header fixture must verify all three runtime cases')
            if fixtures[83].get('cases') != 7:
                raise RuntimeError('Actual cell fixture must verify all seven runtime cases')
            if fixtures[110].get('cases') != 5:
                raise RuntimeError('Actual initialized-string fixture must verify all five runtime cases')
            if fixtures[-1].get('cases') != 7:
                raise RuntimeError('Actual scalar-index fixture must verify all seven runtime cases')
            if (len(cycles) != 12 or summary['genuine_cycles_passed'] != 12 or not fixtures
                    or len({f['fixture'] for f in fixtures}) != len(fixtures)
                    or summary['fixtures_passed'] != 162 or summary['pointer_bytes'] != pointer_bytes
                    or summary['logical_baseline_restored'] is not True):
                raise RuntimeError('Raw-string fixture/lifecycle summary failed')
            if len(measurement_records) != 1:
                raise RuntimeError('Expected exactly one raw-string measurements record')
            measurements = measurement_records[0]['string_measurements']
            large = result['oracles']['zombie_pentagon']
            by_index = {s['index']: s for s in large['strings']}
            expected_measurements = [
                {'index': index, 'length': by_index[index]['length_including_nul'] if tag else 0,
                 'adler32': by_index[index]['adler32'] if tag else 1}
                for index, tag in enumerate(large['table_tags'])]
            if measurements != expected_measurements:
                raise RuntimeError('Actual per-slot indices/lengths/Adler digests differ from independent reference')
            if len(directory_records) != 2:
                raise RuntimeError('Missing genuine directory measurements for either asset')
            directories = {}
            for asset_index, record in enumerate(directory_records):
                asset = list(ASSETS)[asset_index]
                if record['directory_measurements'] != result['oracles'][asset]['directory_records']:
                    raise RuntimeError('Actual per-index directory types/tags differ from genuine original')
                directories[asset] = record['directory_measurements']
            if len(header_records) != 2:
                raise RuntimeError('Missing actual header measurements for either genuine asset')
            headers = {}
            for asset_index, actual_header in enumerate(header_records):
                asset = list(ASSETS)[asset_index]
                ref = result['oracles'][asset]
                expected_header = dict(ref['stringtable_header'])
                expected_header.update({'source_start': ref['stringtable_source_start'],
                                        'cell_count': ref['stringtable_cell_count'],
                                        'disk_start': 0, 'native_start': 0, 'disk_end': 20,
                                        'native_end_before_pop': 32 if pointer_bytes == 8 else 20,
                                        'native_cursor_after_pop': 0,
                                        'shadow_after_pop': 0})
                if actual_header != expected_header:
                    raise RuntimeError('Actual header fields/temporary cursor contract differs from genuine original')
                headers[asset] = actual_header
            if len(cell_records) != 2 or len(name_records) != 2:
                raise RuntimeError('Missing actual name/cell measurements for either genuine asset')
            cell_measurements, name_measurements = {}, {}
            for asset_index, actual_cells in enumerate(cell_records):
                asset = list(ASSETS)[asset_index]
                ref = result['oracles'][asset]
                expected_cells = [{key: cell[key] for key in ['index', 'tag', 'signed_hash']}
                                  for cell in ref['stringtable_cells']]
                if actual_cells != expected_cells:
                    raise RuntimeError('Actual unresolved tags/signed hashes differ from genuine packed cell records')
                name = ref['stringtable_name']
                expected_name = {'source_start': name['source_start'], 'length': name['length_including_nul'],
                                 'adler32': name['adler32'], 'disk_start': name['disk_start'], 'disk_end': name['disk_end'],
                                 'native_start': name['native64_start'] if pointer_bytes == 8 else name['disk_start'],
                                 'native_end': name['native64_end'] if pointer_bytes == 8 else name['disk_end']}
                if name_records[asset_index] != expected_name:
                    raise RuntimeError('Actual name bytes/alignment differ from independent original-string digest')
                cell_measurements[asset], name_measurements[asset] = actual_cells, name_records[asset_index]
            if len(resolved_records) != 2:
                raise RuntimeError('Missing actual resolved-string measurements for either genuine asset')
            resolved_measurements = {}
            for asset_index, actual_resolved in enumerate(resolved_records):
                asset = list(ASSETS)[asset_index]
                expected_resolved = []
                for cell in result['oracles'][asset]['stringtable_cells']:
                    string = cell['string']
                    kind = 'inline' if string['kind'] == 'cell' else string['kind']
                    if kind == 'inline':
                        disk, cell_native_target = string['disk_start'], string['native64_start']
                    elif kind == 'offset':
                        disk, cell_native_target = string['disk_target'], string['native64_target']
                    elif kind == 'null':
                        disk, cell_native_target = 0, 0
                    else:
                        raise RuntimeError('Unknown independent cell-string kind')
                    expected_resolved.append({'index': cell['index'], 'tag': cell['tag'], 'kind': kind,
                                              'length': string.get('length_including_nul', 0),
                                              'adler32': string.get('adler32', 1), 'disk_target': disk,
                                              'native_target': cell_native_target if pointer_bytes == 8 else disk})
                if actual_resolved != expected_resolved:
                    raise RuntimeError('Actual guarded cell strings/earlier-start offsets differ from genuine original')
                resolved_measurements[asset] = actual_resolved
            if len(index_records) != 2:
                raise RuntimeError('Missing actual signed16 index measurements for either genuine asset')
            indices_measurements = {}
            for asset_index, actual_indices in enumerate(index_records):
                asset = list(ASSETS)[asset_index]
                indices = result['oracles'][asset]['stringtable_indices']
                expected_indices = {'source_start': indices['source_start'], 'count': indices['count'],
                                    'bytes': indices['disk_bytes'], 'adler32': indices['adler32'],
                                    'disk_start': indices['disk_start'], 'disk_end': indices['disk_end'],
                                    'native_start': indices['native64_start'] if pointer_bytes == 8 else indices['disk_start'],
                                    'native_end': indices['native64_end'] if pointer_bytes == 8 else indices['disk_end'],
                                    'values': indices['values']}
                if actual_indices != expected_indices:
                    raise RuntimeError('Actual original signed16 words/alignment differ from independent unchanged retail order')
                indices_measurements[asset] = actual_indices
            if len(hash_records) != 2:
                raise RuntimeError('Missing actual per-cell stored/computed hash evidence for either asset')
            hash_measurements = {}
            for asset_index, actual_hashes in enumerate(hash_records):
                asset = list(ASSETS)[asset_index]
                expected_hashes = [{'index': cell['index'], 'stored': cell['signed_hash'], 'computed': cell['signed_hash']}
                                   for cell in result['oracles'][asset]['stringtable_cells']]
                if actual_hashes != expected_hashes:
                    raise RuntimeError('Actual canonical/wrapper hash does not preserve genuine stored signed bits')
                hash_measurements[asset] = actual_hashes
            if len(normalization_records) != 2:
                raise RuntimeError('Missing actual normalization measurements for either genuine asset')
            normalization_measurements = {}
            normalization_keys = {'cell_count', 'element_bytes', 'only_indices_changed', 'exact_policy', 'sorted_indices'}
            for asset_index, actual_normalization in enumerate(normalization_records):
                asset = list(ASSETS)[asset_index]
                reference_asset = original['assets'][asset]
                if (not isinstance(actual_normalization, dict) or set(actual_normalization) != normalization_keys
                        or type(actual_normalization['cell_count']) is not int
                        or actual_normalization['cell_count'] != reference_asset['cell_count']
                        or type(actual_normalization['element_bytes']) is not int or actual_normalization['element_bytes'] != 2
                        or actual_normalization['only_indices_changed'] is not True or actual_normalization['exact_policy'] is not True
                        or not isinstance(actual_normalization['sorted_indices'], list)
                        or any(type(value) is not int for value in actual_normalization['sorted_indices'])
                        or actual_normalization['sorted_indices'] != reference_asset['normalized_indices']):
                    raise RuntimeError('Actual normalization order/ownership differs from unchanged original instructions')
                words = actual_normalization['sorted_indices']
                normalized_sha = hashlib.sha256(struct.pack('<' + 'h' * len(words), *words)).hexdigest()
                if normalized_sha != reference_asset['normalized_indices_sha256']:
                    raise RuntimeError('Normalized index digest differs from original executable reference')
                normalization_measurements[asset] = dict(actual_normalization, normalized_indices_sha256=normalized_sha)
            if len(lookup_records) != 2:
                raise RuntimeError('Missing actual query measurements for either genuine asset')
            lookup_measurements = {}
            lookup_keys = {'queries', 'row_queries', 'row_misses', 'row_disagreements', 'column_queries',
                           'column_disagreements', 'casefold_row_agrees', 'casefold_column_agrees'}
            query_keys = {'index', 'hash', 'first_hash_index', 'row', 'linear_row', 'column', 'linear_column'}
            for asset_index, actual_lookup in enumerate(lookup_records):
                asset = list(ASSETS)[asset_index]
                ref = result['oracles'][asset]
                expected_lookup = normalized_queries[asset]
                queries = expected_lookup['queries']
                if (not isinstance(actual_lookup, dict) or set(actual_lookup) != lookup_keys
                        or not isinstance(actual_lookup['queries'], list)
                        or any(not isinstance(query, dict) or set(query) != query_keys
                               or any(type(value) is not int for value in query.values())
                               for query in actual_lookup['queries'])):
                    raise RuntimeError('Unknown/missing actual lookup measurement fields')
                misses = sum(query['row'] == -1 for query in queries)
                row_disagreements = sum(query['row'] != query['linear_row'] for query in queries)
                column_disagreements = sum(query['column'] != query['linear_column'] for query in queries)
                if (misses != expected_lookup['row_misses']
                        or row_disagreements != expected_lookup['row_disagreements']
                        or column_disagreements != expected_lookup['column_disagreements']):
                    raise RuntimeError('Independent query counters disagree with independent query records')
                if (actual_lookup != expected_lookup or actual_lookup['casefold_row_agrees'] is not True
                        or actual_lookup['casefold_column_agrees'] is not True):
                    raise RuntimeError('Actual original lookup results/casefold/counters differ from independent source model')
                lookup_measurements[asset] = actual_lookup
            identities = set()
            for cycle_index, row in enumerate(cycles):
                expected_keys = {'asset_index', 'round', 'alloc_type', 'source_consumed_bytes', 'decoded_source_end',
                                 'block4_disk_cursor', 'block4_native_cursor', 'name_bytes', 'cell_count',
                                 'inline_cell_strings', 'offset_cell_strings', 'inline_cell_bytes',
                                 'block0_native_after_pop', 'block0_shadow_after_pop', 'every_cell_string_matches',
                                 'exact_initialized_targets', 'actual_owned_slot_conversion',
                                 'provenance_cleared_before_reset', 'indices_loaded', 'original_index_order_preserved',
                                 'scalar_stride_two', 'scalar_loader_cleared', 'actual_hash_matches',
                                 'actual_lookup_executed', 'actual_casefold_queries_agree',
                                 'row_queries', 'row_query_misses', 'row_query_disagreements',
                                 'column_queries', 'column_query_disagreements',
                                 'all_aliases_match', 'live_header_scope_verified',
                                 'prior_block4_preserved', 'reset_and_release_pass'}
                if set(row) != expected_keys:
                    raise RuntimeError('Unknown or missing actual header cycle fields')
                identity = (row['asset_index'], row['round'], row['alloc_type'])
                if (identity[0] not in (0, 1) or identity[1] not in range(3) or identity[2] not in (0, 1)
                        or identity in identities or identity != (cycle_index % 2, cycle_index // 4, (cycle_index // 2) % 2)):
                    raise RuntimeError('Invalid or duplicate genuine cycle identity')
                identities.add(identity)
                ref = result['oracles'][list(ASSETS)[identity[0]]]
                name, array = ref['stringtable_name'], ref['stringtable_cell_array']
                expectations = {'source_consumed_bytes': ref['execution_prefix_bytes'] - 52,
                                'decoded_source_end': ref['execution_prefix_bytes'], 'block4_disk_cursor': ref['stringtable_indices']['disk_end'],
                                'block4_native_cursor': ref['stringtable_indices']['native64_end'] if pointer_bytes == 8 else ref['stringtable_indices']['disk_end']}
                inline_bytes = sum(c['string']['length_including_nul'] for c in ref['stringtable_cells'] if c['string']['kind'] == 'cell')
                expectations.update({'name_bytes': name['length_including_nul'], 'cell_count': array['count'],
                                     'inline_cell_strings': ref['inline_cell_string_count'],
                                     'offset_cell_strings': ref['offset_cell_string_count'], 'inline_cell_bytes': inline_bytes,
                                     'block0_native_after_pop': 0,
                                     'block0_shadow_after_pop': 0,
                                     'row_queries': normalized_queries[list(ASSETS)[identity[0]]]['row_queries'],
                                     'row_query_misses': normalized_queries[list(ASSETS)[identity[0]]]['row_misses'],
                                     'row_query_disagreements': normalized_queries[list(ASSETS)[identity[0]]]['row_disagreements'],
                                     'column_queries': normalized_queries[list(ASSETS)[identity[0]]]['column_queries'],
                                     'column_query_disagreements': normalized_queries[list(ASSETS)[identity[0]]]['column_disagreements']})
                for key, expected in expectations.items():
                    if type(row[key]) is not int or row[key] != expected:
                        raise RuntimeError(f'{target} {identity}: {key} differs from independently decoded original')
                for key in ['all_aliases_match', 'reset_and_release_pass',
                            'every_cell_string_matches', 'exact_initialized_targets', 'actual_owned_slot_conversion',
                            'provenance_cleared_before_reset', 'indices_loaded', 'original_index_order_preserved',
                            'scalar_stride_two', 'scalar_loader_cleared',
                            'actual_hash_matches', 'actual_lookup_executed', 'actual_casefold_queries_agree',
                            'live_header_scope_verified', 'prior_block4_preserved']:
                    if row[key] is not True:
                        raise RuntimeError(f'{target} {identity}: {key} failed')
            result['platforms'][target] = {'binary_sha256': sha(native if on_device else binary), 'fresh_decodes': decodes,
                                           'preflight_fixtures': fixtures, 'string_measurements': measurements,
                                           'directory_measurements': directories,
                                           'header_measurements': headers,
                                           'cell_measurements': cell_measurements, 'name_measurements': name_measurements,
                                           'resolved_string_measurements': resolved_measurements,
                                           'index_measurements': indices_measurements,
                                           'hash_measurements': hash_measurements, 'hash_vectors': vectors,
                                           'comparison_vectors': comparison_records[0],
                                           'lookup_fixtures': lookup_fixture_records[0],
                                           'lookup_measurements': lookup_measurements,
                                           'normalization_fixtures': normalization_fixture,
                                           'normalization_measurements': normalization_measurements,
                                           'runtime_directory_cases': fixtures[40]['cases'],
                                           'runtime_header_cases': fixtures[64]['cases'],
                                           'runtime_cell_cases': fixtures[83]['cases'],
                                           'runtime_initialized_cases': fixtures[110]['cases'],
                                           'runtime_indices_cases': fixtures[-1]['cases'],
                                           'cycles': cycles, 'summary': summary}
            write_json('results.json', result)
        if result['platforms']['windows_x64']['cycles'] != result['platforms']['honor_arm64']['cycles']:
            raise RuntimeError('Windows x64 and HONOR asset-directory logical contracts differ')
        if result['platforms']['windows_x64']['string_measurements'] != result['platforms']['honor_arm64']['string_measurements']:
            raise RuntimeError('Windows x64 and HONOR per-slot string measurements differ')
        if result['platforms']['windows_x64']['directory_measurements'] != result['platforms']['honor_arm64']['directory_measurements']:
            raise RuntimeError('Windows x64 and HONOR per-index asset-directory measurements differ')
        if result['platforms']['windows_x64']['header_measurements'] != result['platforms']['honor_arm64']['header_measurements']:
            raise RuntimeError('Windows x64 and HONOR actual header measurements differ')
        for field in ['cell_measurements', 'name_measurements', 'resolved_string_measurements', 'index_measurements', 'hash_measurements', 'lookup_measurements', 'normalization_measurements']:
            if result['platforms']['windows_x64'][field] != result['platforms']['honor_arm64'][field]:
                raise RuntimeError('Windows x64 and HONOR actual name/cell measurements differ: ' + field)
        overflow_count = result['platforms']['windows_x86']['hash_vectors']['overflow_vectors']
        for target in ['windows_x64', 'honor_arm64']:
            if result['platforms'][target]['hash_vectors']['overflow_vectors'] != overflow_count:
                raise RuntimeError('Actual wrapping-vector coverage differs across platforms')
            if result['platforms'][target]['hash_measurements'] != result['platforms']['windows_x86']['hash_measurements']:
                raise RuntimeError('Actual genuine signed hash bits differ from original Windows reference')
            for field in ['lookup_measurements', 'comparison_vectors', 'lookup_fixtures', 'normalization_fixtures', 'normalization_measurements']:
                if result['platforms'][target][field] != result['platforms']['windows_x86'][field]:
                    raise RuntimeError('Actual query/comparison evidence differs from Windows x86 reference: ' + field)
        result['native64_contract_matches'], result['all_tests_completed'] = True, True
        result['original_query_results_match_windows_reference'] = True
        result['normalized_order_matches_original_instructions'] = True
        result['normalized_original_reference_lookup_misses'] = {
            asset: result['platforms']['windows_x86']['lookup_measurements'][asset]['row_misses'] for asset in ASSETS}
        write_json('results.json', result)
        print('PASS actual normalization/queries match original order, Windows and reference:', out, flush=True)
    except Exception as error:
        write_json('failure.json', {'error_type': type(error).__name__, 'error': str(error), 'traceback': traceback.format_exc()})
        write_json('results.json', result)
        raise


if __name__ == '__main__':
    main()
