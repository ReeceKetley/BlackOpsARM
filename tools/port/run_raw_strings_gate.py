"""Reproduce genuine bounded raw-string leaves without interning or asset parsing.

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

from raw_strings_oracle import reference, sha

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
    ap.add_argument('--reuse-results', type=Path, default=ROOT / 'build-android-audit/string_table_run2/results.json')
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

    result = {'schema': 'BlackOpsARM.TASK-0015.raw-strings-gate.v1',
              'recorded_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'device_serial': args.serial, 'oracles': {}, 'platforms': {},
              'scope': 'Actual raw null-terminated string leaf with real zone blocks; no interning, encoded offset resolution, directory or asset payload parsing',
              'reader_boundary': 'Finite checked fixture reader, separate complete native decode process; production asynchronous/refill pipeline remains unvalidated',
              'original_byte_buffers_in_evidence': False}
    try:
        native = args.android_build.resolve() / 'bo1_raw_strings_tests'
        host_binaries = {arch: args.host_build.resolve() / arch / 'raw_strings.exe' for arch in ['x86', 'x64']}
        for binary in [native, *host_binaries.values()]:
            if not binary.is_file():
                raise RuntimeError('Missing raw-string gate executable: ' + str(binary))
        reused = json.loads(args.reuse_results.read_text(encoding='utf-8'))
        old_device = reused['device_directory']
        if (reused['schema'] != 'BlackOpsARM.TASK-0014.string-table-gate.v1'
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
            ref.update({'oracle_path': oracle_path.relative_to(ROOT).as_posix(), 'oracle_sha256': sha(oracle_path)})
            if reused['oracles'][asset]['raw_sha256'] != ref['raw_sha256']:
                raise RuntimeError('Previous original raw digest differs')
            result['oracles'][asset] = ref
            write_json('independent-oracles.json', result['oracles'])
        device = '/data/local/tmp/blackopsarm-phase1-raw-strings-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S%f')
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
                print(target, asset, 'separate full decode and bounded raw-string prefix validation', flush=True)
                decoded = json.loads(execute(asset + '-decode', ['decode', source, prefix]))
                if (decoded['decoded_bytes'] != ref['decoded_bytes'] or decoded['decoded_adler32'] != ref['decoded_adler32']
                        or decoded['prefix_bytes'] != ref['prefix_bytes'] or decoded['decoded_buffer_released'] is not True
                        or decoded['vector_capacity_bytes'] < ref['decoded_bytes']
                        or any(key in decoded for key in ['prefix_hex', 'string_text', 'string_hex'])):
                    raise RuntimeError('Native full decode/prefix metadata differs or emits original bytes')
                digest = shell(target + '-' + asset + '-prefix-sha', ['sha256sum', prefix]).split()[0] if on_device else sha(prefix)
                if digest != ref['prefix_sha256']:
                    raise RuntimeError('Native raw-string prefix file hash differs')
                decodes[asset] = decoded
                params.extend([prefix, str(ref['decoded_bytes'])])
            rows = [json.loads(line) for line in execute('raw-strings', params).splitlines() if line.strip()]
            fixtures, cycles, summaries, measurement_records = [], [], [], []
            for index, row in enumerate(rows):
                if not isinstance(row, dict):
                    raise RuntimeError('Gate record must be a JSON object')
                categories = [key for key in ['fixture', 'asset_index', 'schema', 'string_measurements'] if key in row]
                if len(categories) != 1:
                    raise RuntimeError('Unknown or ambiguous raw-string record')
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
                    if row['schema'] != 'BlackOpsARM.TASK-0015.raw-strings.v1' or index != len(rows) - 1:
                        raise RuntimeError('Unknown/nonterminal summary schema')
                    summaries.append(row)
            if len(summaries) != 1:
                raise RuntimeError('Expected exactly one raw-string gate summary')
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
            if len(fixtures) != len(expected_fixtures):
                raise RuntimeError('Missing or extra raw-string fixture records')
            for row, (name, status, unchanged) in zip(fixtures, expected_fixtures):
                if (row['fixture'] != name or row['status'] != status or row['no_mutation'] is not unchanged):
                    raise RuntimeError('Raw-string fixture identity/status/mutation differs: ' + str(row))
            if (len(cycles) != 12 or summary['genuine_cycles_passed'] != 12 or not fixtures
                    or len({f['fixture'] for f in fixtures}) != len(fixtures)
                    or summary['fixtures_passed'] != 24 or summary['pointer_bytes'] != pointer_bytes
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
            identities = set()
            for row in cycles:
                identity = (row['asset_index'], row['round'], row['alloc_type'])
                if (identity[0] not in (0, 1) or identity[1] not in range(3) or identity[2] not in (0, 1)
                        or identity in identities):
                    raise RuntimeError('Invalid or duplicate genuine cycle identity')
                identities.add(identity)
                ref = result['oracles'][list(ASSETS)[identity[0]]]
                expectations = {'inline_string_count': ref['inline_string_count'], 'raw_string_bytes': ref['raw_string_bytes'],
                                'max_string_bytes': ref['max_string_bytes'], 'null_slots': len(ref['null_slots']),
                                'empty_slots': len(ref['empty_slots']), 'source_consumed_bytes': ref['prefix_bytes'] - 52,
                                'decoded_source_end': ref['decoded_source_end'], 'block4_disk_cursor': ref['block4_disk_cursor'],
                                'block4_native_cursor': ref['block4_native64_cursor'] if pointer_bytes == 8 else ref['block4_disk_cursor'],
                                'native_padding_bytes': ref['native64_padding_bytes'] if pointer_bytes == 8 else 0}
                for key, expected in expectations.items():
                    if row[key] != expected:
                        raise RuntimeError(f'{target} {identity}: {key} differs from independently decoded original')
                for key in ['per_string_lengths_match', 'raw_bytes_match', 'mappings_match', 'all_aliases_match', 'reset_and_release_pass']:
                    if row[key] is not True:
                        raise RuntimeError(f'{target} {identity}: {key} failed')
            result['platforms'][target] = {'binary_sha256': sha(native if on_device else binary), 'fresh_decodes': decodes,
                                           'preflight_fixtures': fixtures, 'string_measurements': measurements,
                                           'cycles': cycles, 'summary': summary}
            write_json('results.json', result)
        if result['platforms']['windows_x64']['cycles'] != result['platforms']['honor_arm64']['cycles']:
            raise RuntimeError('Windows x64 and HONOR raw-string logical contracts differ')
        if result['platforms']['windows_x64']['string_measurements'] != result['platforms']['honor_arm64']['string_measurements']:
            raise RuntimeError('Windows x64 and HONOR per-slot string measurements differ')
        result['native64_contract_matches'], result['all_tests_completed'] = True, True
        write_json('results.json', result)
        print('PASS genuine bounded raw-string gate:', out, flush=True)
    except Exception as error:
        write_json('failure.json', {'error_type': type(error).__name__, 'error': str(error), 'traceback': traceback.format_exc()})
        write_json('results.json', result)
        raise


if __name__ == '__main__':
    main()
