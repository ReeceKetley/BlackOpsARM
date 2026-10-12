"""Compare real engine string-pool behavior on Windows x86/x64 and HONOR.

Original-game hashes and cached bounded prefixes establish byte provenance;
string IDs come exclusively from actual engine outputs. No string text/hex is
stored in evidence. Only new small probes/prefixes may be placed on the device.
"""
from pathlib import Path
import argparse
import datetime
import json
import os
import shlex
import subprocess
import traceback

from raw_strings_oracle import reference, sha

ROOT = Path(__file__).resolve().parents[2]
ASSETS = {'code_pre_gfx': 'retained.inflate-genuine-oracle.json',
          'zombie_pentagon': 'retained.inflate-zombie-oracle.json'}


def normalized(value):
    """Normalize only the declared architecture width, never engine IDs/counters."""
    if isinstance(value, dict):
        return {key: normalized(item) for key, item in value.items() if key != 'pointer_bytes'}
    if isinstance(value, list):
        return [normalized(item) for item in value]
    return value


def validate_genuine(stdout, pointer_bytes, oracles):
    rows = [json.loads(line) for line in stdout.splitlines() if line.strip()]
    fixtures, cycles, measurements, summaries = [], [], [], []
    for index, row in enumerate(rows):
        if not isinstance(row, dict):
            raise RuntimeError('Interning record must be a JSON object')
        categories = [key for key in ['fixture', 'asset_index', 'string_measurements', 'schema'] if key in row]
        if len(categories) != 1:
            raise RuntimeError('Unknown or ambiguous actual interning record')
        category = categories[0]
        if category == 'fixture':
            if cycles or measurements or summaries:
                raise RuntimeError('Interning preflight record emitted after genuine data')
            fixtures.append(row)
        elif category == 'asset_index':
            if summaries:
                raise RuntimeError('Interning cycle emitted after summary')
            cycles.append(row)
        elif category == 'string_measurements':
            if (measurements or summaries or len(cycles) != 1
                    or (cycles[0]['asset_index'], cycles[0]['round'], cycles[0]['alloc_type']) != (0, 0, 0)):
                raise RuntimeError('Actual ID measurements emitted at unexpected or duplicate cycle')
            measurements.append(row['string_measurements'])
        else:
            if index != len(rows) - 1:
                raise RuntimeError('Nonterminal genuine interning summary')
            summaries.append(row)
    expected_summary = {'schema': 'BlackOpsARM.TASK-0017.intern-strings.v1', 'pointer_bytes': pointer_bytes,
                        'fixtures_passed': 24, 'genuine_cycles_passed': 12, 'logical_baseline_restored': True}
    if len(summaries) != 1 or summaries[0] != expected_summary or len(cycles) != 12 or len(measurements) != 1:
        raise RuntimeError('Actual genuine interning summary/cycle/measurement contract failed')
    statuses = ['ok', 'ok', 'ok', 'unterminated', 'invalid_source', 'unterminated',
                'unsupported_tag', 'unsupported_tag', 'unsupported_tag', 'unsupported_tag',
                'disk_bounds', 'native_bounds', 'native_bounds' if pointer_bytes == 8 else 'ok',
                'disk_bounds', 'native_bounds', 'native_bounds', 'commit_overflow']
    expected_fixtures = [(f'preflight_{number}', status, True) for number, status in enumerate(statuses, 1)]
    expected_fixtures.extend([('short_source', 'unterminated', True), ('short_destination', 'native_bounds', True),
                              ('invalid_lengths', 'invalid_source', True), ('exact_end', 'ok', False),
                              ('exhausted', 'unterminated', True), ('overlap', 'invalid_source', True),
                              ('injected_failure', 'invalid_source', True)])
    if len(fixtures) != len(expected_fixtures):
        raise RuntimeError('Missing or extra genuine interning prerequisite fixtures')
    for row, (name, status, unchanged) in zip(fixtures, expected_fixtures):
        if (set(row) != {'fixture', 'status', 'no_mutation'} or row['fixture'] != name
                or row['status'] != status or row['no_mutation'] is not unchanged):
            raise RuntimeError('Genuine interning prerequisite status/mutation mismatch')
    large = oracles['zombie_pentagon']
    actual = measurements[0]
    by_index = {entry['index']: entry for entry in large['strings']}
    if not isinstance(actual, list) or len(actual) != len(large['table_tags']):
        raise RuntimeError('Missing per-index actual pool ID measurements')
    unique_ids, id_digests, digest_ids = set(), {}, {}
    for index, (row, tag) in enumerate(zip(actual, large['table_tags'])):
        source = by_index.get(index)
        if (not isinstance(row, dict) or set(row) != {'index', 'length', 'adler32', 'id'}
                or row['index'] != index or row['length'] != (source['length_including_nul'] if tag else 0)
                or row['adler32'] != (source['adler32'] if tag else 1)
                or type(row['id']) is not int or not 0 <= row['id'] < 65536
                or (row['id'] == 0) != (tag == 0)):
            raise RuntimeError('Actual per-index ID/length/Adler differs from original bytes/null semantics')
        if row['id']:
            identifier, digest = row['id'], source['sha256']
            if identifier in id_digests and id_digests[identifier] != digest:
                raise RuntimeError('Actual pool ID aliases distinct genuine byte strings')
            if digest in digest_ids and digest_ids[digest] != identifier:
                raise RuntimeError('Actual pool fails to deduplicate equal genuine byte strings')
            id_digests[identifier], digest_ids[digest] = digest, identifier
            unique_ids.add(identifier)
    expected_keys = {'asset_index', 'round', 'alloc_type', 'inline_string_count', 'raw_string_bytes',
                     'source_consumed_bytes', 'decoded_source_end', 'block4_disk_cursor', 'block4_native_cursor',
                     'unique_interned_strings', 'pool_live_allocations', 'pool_live_buckets', 'pool_debug_refs',
                     'pool_released', 'hash_free_list_restored', 'all_aliases_match', 'reset_and_release_pass',
                     'strings_used_after_release'}
    identities = set()
    for row in cycles:
        if set(row) != expected_keys:
            raise RuntimeError('Unknown/missing genuine pool cycle fields')
        identity = (row['asset_index'], row['round'], row['alloc_type'])
        if identity[0] not in (0, 1) or identity[1] not in range(3) or identity[2] not in (0, 1) or identity in identities:
            raise RuntimeError('Unexpected or duplicate genuine pool cycle identity')
        identities.add(identity)
        ref = oracles[list(ASSETS)[identity[0]]]
        unique = len(unique_ids) if identity[0] else 0
        expected = {'inline_string_count': ref['inline_string_count'], 'raw_string_bytes': ref['raw_string_bytes'],
                    'source_consumed_bytes': ref['prefix_bytes'] - 52, 'decoded_source_end': ref['decoded_source_end'],
                    'block4_disk_cursor': ref['block4_disk_cursor'],
                    'block4_native_cursor': ref['block4_native64_cursor'] if pointer_bytes == 8 else ref['block4_disk_cursor'],
                    'unique_interned_strings': unique, 'pool_live_allocations': unique, 'pool_debug_refs': unique}
        for key, value in expected.items():
            if row[key] != value:
                raise RuntimeError('Actual genuine pool count/cursor differs: ' + key)
        if (type(row['pool_live_buckets']) is not int or row['pool_live_buckets'] < 0
                or (row['pool_live_buckets'] == 0) != (unique == 0)):
            raise RuntimeError('Actual genuine pool bucket counter is inconsistent')
        for key in ['pool_released', 'hash_free_list_restored', 'all_aliases_match', 'reset_and_release_pass']:
            if row[key] is not True:
                raise RuntimeError('Actual genuine pool cleanup/ownership failed: ' + key)
        if (not isinstance(row['strings_used_after_release'], list) or len(row['strings_used_after_release']) != 32
                or any(type(value) is not int for value in row['strings_used_after_release'])):
            raise RuntimeError('Missing actual post-release production diagnostics')
    return {'preflight_fixtures': fixtures, 'string_measurements': actual, 'cycles': cycles, 'summary': summaries[0],
            'unique_measurement_ids': len(unique_ids)}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for name in ['android-build', 'host-build', 'output']:
        ap.add_argument('--' + name, type=Path, required=True)
    ap.add_argument('--serial', default='AKWJ9X5116G04618')
    ap.add_argument('--adb', type=Path, default=Path('D:/Android/Sdk/platform-tools/adb.exe'))
    ap.add_argument('--retail-root', type=Path, default=Path('D:/Games/Call of Duty - Black Ops'))
    ap.add_argument('--reuse-results', type=Path, default=ROOT / 'build-android-audit/raw_strings_run1/results.json')
    ap.add_argument('--fixtures-only', action='store_true')
    ap.add_argument('--intern-host-build', type=Path)
    args = ap.parse_args()
    out = args.output.resolve()
    if not out.is_relative_to(ROOT) or out.exists():
        raise RuntimeError('Use a fresh output directory inside BlackOpsARM')
    out.mkdir(parents=True)
    commands = []
    env = dict(os.environ)
    for key in ['BO1_STREAM_TRACE', 'BO1_TRACE_ZONE', 'BO1_TRACE_ASSET', 'BO1_TRACE_STACK_OP']:
        env.pop(key, None)
    env['BO1_DIAG_LOG'] = str(out / 'diagnostics.log')

    def write_json(name, value):
        (out / name).write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')

    def run(label, argv, expected_exit=0):
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
        commands.append({'label': label, 'argv': argv, 'cwd': str(out), 'exit': code,
                         'expected_exit': expected_exit, 'timed_out': timed_out, 'launch_error': launch_error})
        write_json('invocations.json', commands)
        if code != expected_exit or code is None:
            raise RuntimeError(f'{label}: exit {code}, expected {expected_exit}; diagnostics retained')
        return stdout, stderr

    adb = [args.adb, '-s', args.serial]

    def shell(label, words, expected_exit=0):
        return run(label, adb + ['shell', ' '.join(shlex.quote(str(word)) for word in words)], expected_exit)

    result = {'schema': 'BlackOpsARM.TASK-0017.string-pool-gate.v1',
              'recorded_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'device_serial': args.serial, 'oracles': {}, 'platforms': {},
              'id_reference': 'Actual Windows x86/x64 engine output only; no fabricated ID oracle',
              'scope': 'Real engine pool lifecycle and bounded genuine temporary strings; no asset dispatch or rendering',
              'original_byte_buffers_in_evidence': False}
    try:
        native = args.android_build.resolve() / 'bo1_string_pool_tests'
        hosts = {arch: args.host_build.resolve() / arch / 'string_pool.exe' for arch in ['x86', 'x64']}
        intern_native = args.android_build.resolve() / 'bo1_intern_strings_tests'
        intern_hosts = {}
        if not args.fixtures_only:
            if args.intern_host_build is None:
                raise RuntimeError('Use --intern-host-build for genuine interning, or --fixtures-only')
            intern_hosts = {arch: args.intern_host_build.resolve() / arch / 'intern_strings.exe' for arch in ['x86', 'x64']}
        binaries = [native, *hosts.values()] + ([] if args.fixtures_only else [intern_native, *intern_hosts.values()])
        for binary in binaries:
            if not binary.is_file():
                raise RuntimeError('Missing string-pool gate executable: ' + str(binary))
        reused = json.loads(args.reuse_results.read_text(encoding='utf-8'))
        if (reused.get('all_tests_completed') is not True or reused['device_serial'] != args.serial
                or reused['schema'] not in ['BlackOpsARM.TASK-0015.raw-strings-gate.v1',
                                           'BlackOpsARM.TASK-0016.asset-directory-gate.v1']):
            raise RuntimeError('Invalid cached prefix provenance')
        cached_device = reused['device_directory']
        original_device = reused['reused_original_device_directory']
        if (not cached_device.startswith('/data/local/tmp/blackopsarm-phase1-')
                or '/' in cached_device.removeprefix('/data/local/tmp/')
                or not original_device.startswith('/data/local/tmp/blackopsarm-phase1-')
                or '/' in original_device.removeprefix('/data/local/tmp/')):
            raise RuntimeError('Unexpected cached device paths')
        result.update({'reuse_results_path': str(args.reuse_results.resolve()), 'reuse_results_sha256': sha(args.reuse_results),
                       'cached_prefix_device_directory': cached_device, 'reused_original_device_directory': original_device})
        for asset, leaf in ASSETS.items():
            oracle_path = ROOT / 'docs/port/evidence/arm64_inflate' / leaf
            oracle = json.loads(oracle_path.read_text(encoding='utf-8'))
            ref = reference(args.retail_root / 'zone/common' / f'{asset}.ff', oracle)
            ref.update({'oracle_path': oracle_path.relative_to(ROOT).as_posix(), 'oracle_sha256': sha(oracle_path)})
            if reused['oracles'][asset]['raw_sha256'] != ref['raw_sha256']:
                raise RuntimeError('Cached asset provenance differs from original game')
            result['oracles'][asset] = ref
            write_json('independent-oracles.json', result['oracles'])
        device = '/data/local/tmp/blackopsarm-phase1-string-pool-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S%f')
        result['device_directory'] = device
        write_json('results.json', result)
        shell('device-create', ['mkdir', device])
        run('device-metadata', adb + ['shell', 'getprop ro.product.model; getprop ro.build.version.sdk; uname -a; cat /proc/meminfo'])
        run('device-probe-push', adb + ['push', native, device + '/probe'])
        shell('device-probe-chmod', ['chmod', '700', device + '/probe'])
        if shell('device-probe-sha', ['sha256sum', device + '/probe'])[0].split()[0] != sha(native):
            raise RuntimeError('Device executable hash mismatch')
        if not args.fixtures_only:
            run('device-intern-probe-push', adb + ['push', intern_native, device + '/intern-probe'])
            shell('device-intern-probe-chmod', ['chmod', '700', device + '/intern-probe'])
            if shell('device-intern-probe-sha', ['sha256sum', device + '/intern-probe'])[0].split()[0] != sha(intern_native):
                raise RuntimeError('Device interning executable hash mismatch')
        for asset, ref in result['oracles'].items():
            if shell(asset + '-original-sha', ['sha256sum', f'{original_device}/{asset}.ff'])[0].split()[0] != ref['raw_sha256']:
                raise RuntimeError('Reusable original device asset hash mismatch')
        for target in ['windows_x86', 'windows_x64', 'honor_arm64']:
            on_device = target == 'honor_arm64'
            binary = device + '/probe' if on_device else str(hosts[target.split('_')[1]])

            def execute(label, params, expected_exit=0):
                return shell(target + '-' + label, [binary] + params, expected_exit) if on_device else run(target + '-' + label, [binary] + params, expected_exit)

            stdout, _ = execute('fixtures', ['fixtures'])
            rows = [json.loads(line) for line in stdout.splitlines() if line.strip()]
            if len(rows) != 13 or not isinstance(rows[-1], dict):
                raise RuntimeError('Missing terminal fixture summary')
            fixtures, releases, summary = rows[:-1:2], rows[1:-1:2], rows[-1]
            pointer_bytes = 4 if target == 'windows_x86' else 8
            expected_summary = {'schema': 'BlackOpsARM.TASK-0017.string-pool.v1',
                                'pointer_bytes': pointer_bytes, 'cycles_passed': 6, 'strings_per_cycle': 10,
                                'transfer_cases': 6, 'user0_cases': 6, 'max_text_bytes': 65530,
                                'pool_baseline_restored': True, 'ref_string_bytes': 8, 'ref_data_bytes': 4,
                                'ref_string_offset': 4, 'memory_node_bytes': 16, 'hash_entry_bytes': 8,
                                'hash_free_list_restored': True}
            if summary != expected_summary:
                raise RuntimeError('Actual pool lifecycle/layout summary differs from fixture contract')
            for index, row in enumerate(fixtures):
                if (not isinstance(row, dict)
                        or set(row) != {'round', 'instance', 'ids', 'unique_strings', 'live_allocations', 'live_buckets'}
                        or row['round'] != index // 2 or row['instance'] != index % 2):
                    raise RuntimeError('Unexpected or reordered actual pool fixture record')
                ids = row['ids']
                if (not isinstance(ids, list) or len(ids) != 10
                        or any(type(value) is not int or not 0 < value < 65536 for value in ids)
                        or len(set(ids)) != 9 or ids[1] != ids[2] or ids[4] == ids[5]
                        or row['unique_strings'] != 9 or row['live_allocations'] != 9
                        or type(row['live_buckets']) is not int or row['live_buckets'] <= 0):
                    raise RuntimeError('Actual pool ID/dedup/allocation fixture failed')
                released = releases[index]
                if (not isinstance(released, dict)
                        or set(released) != {'release', 'round', 'instance', 'strings_used',
                                             'live_allocations', 'live_buckets', 'debug_refs'}
                        or released['release'] is not True
                        or released['round'] != row['round'] or released['instance'] != row['instance']
                        or any(type(released[key]) is not int or released[key] != 0
                               for key in ['live_allocations', 'live_buckets', 'debug_refs'])
                        or not isinstance(released['strings_used'], list) or len(released['strings_used']) != 32
                        or any(type(value) is not int for value in released['strings_used'])):
                    raise RuntimeError('Actual pre-shutdown ownership/release counters failed')
            expected_exit = 134 if on_device else 3
            fatal_stdout, fatal_stderr = execute('max-reject', ['max-reject'], expected_exit)
            diagnostics = fatal_stdout + '\n' + fatal_stderr
            for message in ['MT_GetSize: max allocation exceeded', 'Unsupported standalone VM terminal recovery']:
                if message not in diagnostics:
                    raise RuntimeError('Actual oversized MT rejection lacks required terminal diagnostic')
            result['platforms'][target] = {'binary_sha256': sha(native if on_device else binary),
                                           'fixture_records': fixtures, 'fixture_summary': summary,
                                           'release_records': releases,
                                           'max_rejection': {'mode': 'max-reject', 'text_bytes': 65531,
                                                             'exit': expected_exit, 'diagnostics_verified': True}}
            if not args.fixtures_only:
                intern_binary = device + '/intern-probe' if on_device else str(intern_hosts[target.split('_')[1]])
                params, prefix_provenance = ['gate'], {}
                for asset, ref in result['oracles'].items():
                    cache_target = 'windows_x64' if on_device else target
                    cached = args.reuse_results.resolve().parent / f'{cache_target}-{asset}.prefix'
                    prior = reused['oracles'][asset]
                    if sha(cached) != prior['prefix_sha256']:
                        raise RuntimeError('Cached host prefix no longer matches previous complete decode')
                    if on_device and shell(asset + '-cached-prefix-sha', ['sha256sum', f'{cached_device}/{asset}.prefix'])[0].split()[0] != prior['prefix_sha256']:
                        raise RuntimeError('Cached device prefix no longer matches previous complete decode')
                    bounded = cached.read_bytes()[:ref['prefix_bytes']]
                    local_prefix = out / f'{target}-{asset}.prefix'
                    local_prefix.write_bytes(bounded)
                    if len(bounded) != ref['prefix_bytes'] or sha(local_prefix) != ref['prefix_sha256']:
                        raise RuntimeError('Cached/trimmed raw prefix differs from independent full-stream reference')
                    if on_device:
                        prefix = f'{device}/{asset}.prefix'
                        run(asset + '-intern-prefix-push', adb + ['push', local_prefix, prefix])
                        if shell(asset + '-intern-prefix-sha', ['sha256sum', prefix])[0].split()[0] != ref['prefix_sha256']:
                            raise RuntimeError('Fresh device bounded prefix hash mismatch')
                    else:
                        prefix = str(local_prefix)
                    params.extend([prefix, str(ref['decoded_bytes'])])
                    prefix_provenance[asset] = {'cached_host_path': str(cached), 'cached_prefix_sha256': prior['prefix_sha256'],
                                                'prepared_prefix_bytes': ref['prefix_bytes'], 'prepared_prefix_sha256': ref['prefix_sha256'],
                                                'trimmed_directory_bytes': prior['prefix_bytes'] - ref['prefix_bytes']}
                if on_device:
                    intern_stdout, _ = shell(target + '-genuine-intern', [intern_binary] + params)
                else:
                    intern_stdout, _ = run(target + '-genuine-intern', [intern_binary] + params)
                genuine = validate_genuine(intern_stdout, pointer_bytes, result['oracles'])
                genuine.update({'binary_sha256': sha(intern_native if on_device else intern_binary),
                                'cached_prefix_provenance': prefix_provenance})
                result['platforms'][target]['genuine'] = genuine
            write_json('results.json', result)
        reference_rows = normalized({key: result['platforms']['windows_x86'][key]
                                     for key in ['fixture_records', 'release_records', 'fixture_summary']})
        for target in ['windows_x64', 'honor_arm64']:
            candidate = normalized({key: result['platforms'][target][key]
                                    for key in ['fixture_records', 'release_records', 'fixture_summary']})
            if reference_rows != candidate:
                raise RuntimeError('Actual engine pool fixture contract differs across ABIs')
        result['fixture_contract_matches'] = True
        result['fixture_tests_completed'] = True
        if not args.fixtures_only:
            reference_genuine = result['platforms']['windows_x86']['genuine']
            for target in ['windows_x64', 'honor_arm64']:
                candidate = result['platforms'][target]['genuine']
                if reference_genuine['string_measurements'] != candidate['string_measurements']:
                    raise RuntimeError('Actual Windows/ARM per-index genuine pool IDs or digests differ')
                if normalized(reference_genuine['summary']) != normalized(candidate['summary']):
                    raise RuntimeError('Actual Windows/ARM genuine pool summary differs')
                for left, right in zip(reference_genuine['cycles'], candidate['cycles']):
                    ref = result['oracles'][list(ASSETS)[left['asset_index']]]
                    adjusted = dict(right)
                    if adjusted['block4_native_cursor'] != ref['block4_native64_cursor']:
                        raise RuntimeError('Native64 genuine cursor differs from independently verified production alignment')
                    # Both cursor values were independently checked; express the
                    # sole native-width cursor difference in Win32 coordinates.
                    adjusted['block4_native_cursor'] = ref['block4_disk_cursor']
                    if left != adjusted:
                        raise RuntimeError('Actual Windows/ARM genuine ownership, IDs or diagnostics differ')
            result['genuine_contract_matches'] = True
        result['genuine_gate_completed'] = not args.fixtures_only
        result['genuine_contract_pending'] = args.fixtures_only
        result['all_tests_completed'] = not args.fixtures_only
        write_json('results.json', result)
        print('PASS actual string-pool fixtures/max rejection' + ('; genuine contract pending:' if args.fixtures_only else ' and genuine interning:'), out, flush=True)
    except Exception as error:
        write_json('failure.json', {'error_type': type(error).__name__, 'error': str(error), 'traceback': traceback.format_exc()})
        write_json('results.json', result)
        raise


if __name__ == '__main__':
    main()
