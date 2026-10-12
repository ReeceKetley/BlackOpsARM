"""Reproduce the first StringTable resolved cell strings after genuine directory data.

Original assets already on HONOR are SHA-verified and reused. Native decoder
processes finish before zone allocations. Evidence contains no string text/hex;
the small original prefixes stay in ignored local/device scratch directories.
"""
from pathlib import Path
import argparse
import datetime
import json
import os
import shlex
import subprocess
import traceback

from stringtable_payload_oracle import reference, sha

ROOT = Path(__file__).resolve().parents[2]
ASSETS = {'code_pre_gfx': 'retained.inflate-genuine-oracle.json',
          'zombie_pentagon': 'retained.inflate-zombie-oracle.json'}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for name in ['android-build', 'host-build', 'output']:
        ap.add_argument('--' + name, type=Path, required=True)
    ap.add_argument('--serial', default='AKWJ9X5116G04618')
    ap.add_argument('--adb', type=Path, default=Path('D:/Android/Sdk/platform-tools/adb.exe'))
    ap.add_argument('--retail-root', type=Path, default=Path('D:/Games/Call of Duty - Black Ops'))
    ap.add_argument('--reuse-results', type=Path, default=ROOT / 'build-android-audit/asset_directory_run1/results.json')
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

    result = {'schema': 'BlackOpsARM.TASK-0021.stringtable-indices-gate.v1',
              'recorded_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'device_serial': args.serial, 'oracles': {}, 'platforms': {},
              'scope': 'Actual first StringTable signed16 indices after resolved cell strings; preserve original words without sorting; stop before hash/query, registry or rendering',
              'reference_scope': 'Independent payload oracle may audit later first-asset bytes as provenance; execution prefix ends after original signed16 indices before next asset',
              'alias_scope': 'Specific genuine side-envelope alias12 replacement and retained alias4 only; no general duplicate-key ordering guarantee',
              'reader_boundary': 'Finite checked fixture reader, separate complete native decode process; production asynchronous/refill pipeline remains unvalidated',
              'original_byte_buffers_in_evidence': False}
    try:
        native = args.android_build.resolve() / 'bo1_stringtable_indices_tests'
        host_binaries = {arch: args.host_build.resolve() / arch / 'stringtable_indices.exe' for arch in ['x86', 'x64']}
        for binary in [native, *host_binaries.values()]:
            if not binary.is_file():
                raise RuntimeError('Missing asset-directory gate executable: ' + str(binary))
        reused = json.loads(args.reuse_results.read_text(encoding='utf-8'))
        old_device = reused.get('reused_original_device_directory', reused['device_directory'])
        if (reused['schema'] not in ('BlackOpsARM.TASK-0016.asset-directory-gate.v1', 'BlackOpsARM.TASK-0015.raw-strings-gate.v1')
                or reused['device_serial'] != args.serial or reused.get('all_tests_completed') is not True
                or not old_device.startswith('/data/local/tmp/blackopsarm-phase1-string-table-')
                or '/' in old_device.removeprefix('/data/local/tmp/')):
            raise RuntimeError('Invalid previous device-original provenance')
        result.update({'reused_original_device_directory': old_device,
                       'reuse_results_path': str(args.reuse_results.resolve()), 'reuse_results_sha256': sha(args.reuse_results)})
        for asset, leaf in ASSETS.items():
            oracle_path = ROOT / 'docs/port/evidence/arm64_inflate' / leaf
            oracle = json.loads(oracle_path.read_text(encoding='utf-8'))
            ref = reference(args.retail_root / 'zone/common' / f'{asset}.ff', oracle)
            ref.update({'execution_prefix_bytes': ref['prefix_bytes'],
                        'execution_prefix_sha256': ref['prefix_sha256']})
            ref.update({'oracle_path': oracle_path.relative_to(ROOT).as_posix(), 'oracle_sha256': sha(oracle_path)})
            if reused['oracles'][asset]['raw_sha256'] != ref['raw_sha256']:
                raise RuntimeError('Previous original raw digest differs')
            result['oracles'][asset] = ref
            write_json('independent-oracles.json', result['oracles'])
        device = '/data/local/tmp/blackopsarm-phase1-stringtable-indices-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S%f')
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
                print(target, asset, 'separate full decode and bounded asset-directory prefix validation', flush=True)
                decoded = json.loads(execute(asset + '-decode', ['decode', source, prefix]))
                if (decoded['decoded_bytes'] != ref['decoded_bytes'] or decoded['decoded_adler32'] != ref['decoded_adler32']
                        or decoded['prefix_bytes'] != ref['execution_prefix_bytes'] or decoded['decoded_buffer_released'] is not True
                        or decoded['vector_capacity_bytes'] < ref['decoded_bytes']
                        or any(key in decoded for key in ['prefix_hex', 'string_text', 'string_hex'])):
                    raise RuntimeError('Native full decode/prefix metadata differs or emits original bytes')
                digest = shell(target + '-' + asset + '-prefix-sha', ['sha256sum', prefix]).split()[0] if on_device else sha(prefix)
                if digest != ref['execution_prefix_sha256']:
                    raise RuntimeError('Native asset-directory prefix file hash differs')
                decodes[asset] = decoded
                params.extend([prefix, str(ref['decoded_bytes'])])
            rows = [json.loads(line) for line in execute('stringtable-indices', params).splitlines() if line.strip()]
            fixtures, cycles, summaries, measurement_records, directory_records, header_records, cell_records, name_records, resolved_records, index_records = [], [], [], [], [], [], [], [], [], []
            for index, row in enumerate(rows):
                if not isinstance(row, dict):
                    raise RuntimeError('Gate record must be a JSON object')
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
                            or len(resolved_records) != len(cycles) + 1 or len(index_records) != len(cycles) + 1):
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
                categories = [key for key in ['fixture', 'asset_index', 'schema', 'string_measurements'] if key in row]
                if len(categories) != 1:
                    raise RuntimeError('Unknown or ambiguous asset-directory record')
                category = categories[0]
                if category == 'fixture':
                    if (cycles or summaries or not isinstance(row.get('status'), str)
                            or not isinstance(row.get('no_mutation'), bool)):
                        raise RuntimeError('Invalid preflight/no-mutation fixture record')
                    fixtures.append(row)
                elif category == 'asset_index':
                    if summaries:
                        raise RuntimeError('Genuine cycle emitted after summary')
                    cycles.append(row)
                elif category == 'string_measurements':
                    if (measurement_records or summaries or len(cycles) != 1
                            or (cycles[0]['asset_index'], cycles[0]['round'], cycles[0]['alloc_type']) != (0, 0, 0)):
                        raise RuntimeError('Per-string measurements emitted at unexpected/duplicate cycle')
                    measurement_records.append(row)
                else:
                    if row['schema'] != 'BlackOpsARM.TASK-0021.stringtable-indices.v1' or index != len(rows) - 1:
                        raise RuntimeError('Unknown/nonterminal summary schema')
                    summaries.append(row)
            if len(summaries) != 1:
                raise RuntimeError('Expected exactly one asset-directory gate summary')
            summary = summaries[0]
            pointer_bytes = 4 if target == 'windows_x86' else 8
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
                                        'shadow_after_pop': 20 if pointer_bytes == 8 else 0})
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
            identities = set()
            for row in cycles:
                expected_keys = {'asset_index', 'round', 'alloc_type', 'source_consumed_bytes', 'decoded_source_end',
                                 'block4_disk_cursor', 'block4_native_cursor', 'name_bytes', 'cell_count',
                                 'inline_cell_strings', 'offset_cell_strings', 'inline_cell_bytes',
                                 'block0_native_after_pop', 'block0_shadow_after_pop', 'every_cell_string_matches',
                                 'exact_initialized_targets', 'actual_owned_slot_conversion',
                                 'provenance_cleared_before_reset', 'indices_loaded', 'original_index_order_preserved',
                                 'scalar_stride_two', 'scalar_loader_cleared',
                                 'all_aliases_match', 'live_header_scope_verified',
                                 'prior_block4_preserved', 'reset_and_release_pass'}
                if set(row) != expected_keys:
                    raise RuntimeError('Unknown or missing actual header cycle fields')
                identity = (row['asset_index'], row['round'], row['alloc_type'])
                if (identity[0] not in (0, 1) or identity[1] not in range(3) or identity[2] not in (0, 1)
                        or identity in identities):
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
                                     'block0_shadow_after_pop': 20 if pointer_bytes == 8 else 0})
                for key, expected in expectations.items():
                    if row[key] != expected:
                        raise RuntimeError(f'{target} {identity}: {key} differs from independently decoded original')
                for key in ['all_aliases_match', 'reset_and_release_pass',
                            'every_cell_string_matches', 'exact_initialized_targets', 'actual_owned_slot_conversion',
                            'provenance_cleared_before_reset', 'indices_loaded', 'original_index_order_preserved',
                            'scalar_stride_two', 'scalar_loader_cleared',
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
        for field in ['cell_measurements', 'name_measurements', 'resolved_string_measurements', 'index_measurements']:
            if result['platforms']['windows_x64'][field] != result['platforms']['honor_arm64'][field]:
                raise RuntimeError('Windows x64 and HONOR actual name/cell measurements differ: ' + field)
        result['native64_contract_matches'], result['all_tests_completed'] = True, True
        write_json('results.json', result)
        print('PASS genuine first StringTable original signed16 indices without sorting:', out, flush=True)
    except Exception as error:
        write_json('failure.json', {'error_type': type(error).__name__, 'error': str(error), 'traceback': traceback.format_exc()})
        write_json('results.json', result)
        raise


if __name__ == '__main__':
    main()
