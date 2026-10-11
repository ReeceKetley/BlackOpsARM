"""Run actual string pointer-table gates with independent full-stream provenance.

Separate native decoder processes finish and release their decoded vectors
before any zone allocation gate. Existing scratch/device evidence is preserved.
String bytes, interning, asset directories and payloads are outside this gate.
"""
from pathlib import Path
import argparse
import datetime
import json
import os
import shlex
import subprocess
import traceback

from string_table_oracle import reference, sha

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

    def write_json(name, data):
        (out / name).write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')

    def run(label, argv):
        argv = [str(a) for a in argv]
        timed_out = False
        launch_error = None
        try:
            result = subprocess.run(argv, cwd=out, env=env, capture_output=True, text=True,
                                    encoding='utf-8', errors='replace', timeout=180)
            stdout, stderr, exit_code = result.stdout, result.stderr, result.returncode
        except subprocess.TimeoutExpired as error:
            def readable(value):
                return value.decode('utf-8', errors='replace') if isinstance(value, bytes) else (value or '')
            stdout, stderr, exit_code = readable(error.stdout), readable(error.stderr), None
            timed_out = True
        except OSError as error:
            stdout, stderr, exit_code = '', str(error), None
            launch_error = str(error)
        for suffix, value in [('stdout', stdout), ('stderr', stderr)]:
            (out / f'{label}.{suffix}.log').write_text(value, encoding='utf-8')
        invocations.append({'label': label, 'argv': argv, 'cwd': str(out), 'exit': exit_code,
                            'timed_out': timed_out, 'launch_error': launch_error})
        write_json('invocations.json', invocations)
        if exit_code != 0:
            raise RuntimeError(f'{label}: exit {exit_code}; diagnostics retained')
        return stdout

    adb = [args.adb, '-s', args.serial]

    def shell(label, words):
        return run(label, adb + ['shell', ' '.join(shlex.quote(str(word)) for word in words)])

    result = {'schema': 'BlackOpsARM.TASK-0014.string-table-gate.v1',
              'recorded_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'device_serial': args.serial, 'oracles': {}, 'platforms': {},
              'scope': 'Actual pointer-table expansion/Win32 packed table read, cursor/alias/reset/release; finite reader fixture; no string bytes or interning',
              'reader_boundary': 'Separate complete native decode processes precede header/table-only zone gate; engine asynchronous/refill reader remains unvalidated'}
    try:
        native = args.android_build.resolve() / 'bo1_string_table_tests'
        host_binaries = {target: args.host_build.resolve() / target / 'string_table.exe' for target in ['x86', 'x64']}
        for binary in [native, *host_binaries.values()]:
            if not binary.is_file():
                raise RuntimeError('Missing gate executable: ' + str(binary))
        for asset, leaf in ASSETS.items():
            oracle_path = ROOT / 'docs/port/evidence/arm64_inflate' / leaf
            oracle = json.loads(oracle_path.read_text(encoding='utf-8'))
            source = args.retail_root / 'zone/common' / f'{asset}.ff'
            ref = reference(source, oracle)
            ref.update({'oracle_path': oracle_path.relative_to(ROOT).as_posix(), 'oracle_sha256': sha(oracle_path)})
            result['oracles'][asset] = ref
            write_json('independent-oracles.json', result['oracles'])
        device = '/data/local/tmp/blackopsarm-phase1-string-table-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S%f')
        result['device_directory'] = device
        write_json('results.json', result)
        shell('device-create', ['mkdir', device])
        run('device-metadata', adb + ['shell', 'getprop ro.product.model; getprop ro.build.version.sdk; uname -a; cat /proc/meminfo'])
        run('device-probe-push', adb + ['push', native, device + '/probe'])
        shell('device-probe-chmod', ['chmod', '700', device + '/probe'])
        if shell('device-probe-sha', ['sha256sum', device + '/probe']).split()[0] != sha(native):
            raise RuntimeError('Device executable hash mismatch')
        for asset, ref in result['oracles'].items():
            run(asset + '-push', adb + ['push', ref['original_path'], f'{device}/{asset}.ff'])
            if shell(asset + '-device-sha', ['sha256sum', f'{device}/{asset}.ff']).split()[0] != ref['raw_sha256']:
                raise RuntimeError('Device original asset hash mismatch')
        for target in ['windows_x86', 'windows_x64', 'honor_arm64']:
            on_device = target == 'honor_arm64'
            binary = device + '/probe' if on_device else str(host_binaries[target.split('_')[1]])

            def execute(label, params):
                return shell(target + '-' + label, [binary] + params) if on_device else run(target + '-' + label, [binary] + params)

            params = ['gate']
            decodes = {}
            for asset, ref in result['oracles'].items():
                prefix = f'{device}/{asset}.prefix' if on_device else str(out / f'{target}-{asset}.prefix')
                source = f'{device}/{asset}.ff' if on_device else ref['original_path']
                print(target, asset, 'separate full decode and pointer-table validation', flush=True)
                decoded = json.loads(execute(asset + '-decode', ['decode', source, prefix]))
                if (decoded['decoded_bytes'] != ref['decoded_bytes'] or decoded['decoded_adler32'] != ref['decoded_adler32']
                        or decoded['prefix_hex'] != ref['prefix_hex'] or decoded['decoded_buffer_released'] is not True
                        or decoded['vector_capacity_bytes'] < ref['decoded_bytes']):
                    raise RuntimeError('Native decoded table prefix differs from independent full-stream oracle')
                digest = shell(target + '-' + asset + '-prefix-sha', ['sha256sum', prefix]).split()[0] if on_device else sha(prefix)
                if digest != ref['prefix_sha256']:
                    raise RuntimeError('Extracted prefix file hash mismatch')
                decodes[asset] = decoded
                params.extend([prefix, str(ref['decoded_bytes'])])
            rows = [json.loads(line) for line in execute('pointer-table', params).splitlines() if line.strip()]
            if not rows:
                raise RuntimeError('Pointer-table gate produced no records')
            fixtures, cycles, summaries = [], [], []
            for index, row in enumerate(rows):
                if not isinstance(row, dict):
                    raise RuntimeError('Gate record is not a JSON object')
                categories = [key for key in ['fixture', 'asset_index', 'schema'] if key in row]
                if len(categories) != 1:
                    raise RuntimeError('Unknown or ambiguous gate record: ' + str(row))
                category = categories[0]
                if category == 'fixture':
                    if cycles or summaries:
                        raise RuntimeError('Preflight fixture emitted after genuine cycles or summary')
                    fixtures.append(row)
                elif category == 'asset_index':
                    if summaries:
                        raise RuntimeError('Genuine cycle emitted after summary')
                    cycles.append(row)
                else:
                    if row['schema'] != 'BlackOpsARM.TASK-0014.string-table.v1' or index != len(rows) - 1:
                        raise RuntimeError('Unknown schema or nonterminal summary record')
                    summaries.append(row)
            if len(summaries) != 1:
                raise RuntimeError('Expected exactly one terminal gate summary')
            summary = summaries[0]
            pointer_bytes = 8 if target != 'windows_x86' else 4
            expected_statuses = ['negative_count', 'negative_count', 'ok', 'ok', 'native_bounds',
                                 'source_bounds', 'disk_bounds', 'native_bounds', 'ok', 'ok',
                                 'disk_bounds', 'native_bounds', 'disk_bounds', 'native_bounds',
                                 'disk_bounds', 'native_bounds',
                                 'helper_domain' if pointer_bytes == 8 else 'commit_overflow']
            if len(fixtures) != len(expected_statuses):
                raise RuntimeError('Missing or extra preflight fixture records')
            for number, (fixture, expected_status) in enumerate(zip(fixtures, expected_statuses), 1):
                if (fixture['fixture'] != f'preflight_{number}' or fixture.get('status') != expected_status
                        or fixture.get('no_mutation') is not True):
                    raise RuntimeError('Preflight status/identity or no-mutation verification failed')
            if (summary['genuine_cycles_passed'] != 12 or len(cycles) != 12 or summary['fixtures_passed'] < 20
                    or summary['fixtures_passed'] < len(fixtures) + 9
                    or summary['pointer_bytes'] != pointer_bytes or summary['logical_baseline_restored'] is not True):
                raise RuntimeError('Pointer-table fixture or lifecycle summary failed')
            keys = set()
            for row in cycles:
                asset_index = row['asset_index']
                if asset_index not in (0, 1) or row['round'] not in range(3) or row['alloc_type'] not in (0, 1):
                    raise RuntimeError('Unexpected genuine cycle identity')
                identity = (asset_index, row['round'], row['alloc_type'])
                if identity in keys:
                    raise RuntimeError('Duplicate genuine cycle')
                keys.add(identity)
                ref = result['oracles'][list(ASSETS)[asset_index]]
                disk_bytes = ref['table_disk_bytes']
                native_bytes = len(ref['table_tags']) * pointer_bytes
                for key, expected in [('string_count', ref['string_count']), ('table_disk_bytes', disk_bytes),
                                      ('table_native_bytes', native_bytes), ('source_consumed_bytes', disk_bytes),
                                      ('block4_disk_cursor', disk_bytes), ('block4_native_cursor', native_bytes)]:
                    if row[key] != expected:
                        raise RuntimeError(f'{target} {identity}: {key} differs from genuine allocation/read contract')
                for key in ['all_tags_match', 'all_aliases_match', 'exact_element_mappings', 'reset_and_release_pass']:
                    if row[key] is not True:
                        raise RuntimeError(f'{target} {identity}: {key} failed')
                if 'decoded_source_end' in row and row['decoded_source_end'] != 52 + disk_bytes:
                    raise RuntimeError('Decoded source end differs from exact table-prefix boundary')
            result['platforms'][target] = {'binary_sha256': sha(native if on_device else binary),
                                           'fresh_decodes': decodes, 'preflight_fixtures': fixtures,
                                           'unprinted_runtime_fixture_count': summary['fixtures_passed'] - len(fixtures),
                                           'cycles': cycles, 'summary': summary}
            write_json('results.json', result)
        x64 = {(r['asset_index'], r['round'], r['alloc_type']): r for r in result['platforms']['windows_x64']['cycles']}
        arm = {(r['asset_index'], r['round'], r['alloc_type']): r for r in result['platforms']['honor_arm64']['cycles']}
        if x64 != arm:
            raise RuntimeError('Windows x64 and HONOR native pointer-table contracts differ')
        result['native64_contract_matches'] = True
        result['all_tests_completed'] = True
        write_json('results.json', result)
        print('PASS genuine string pointer-table gate:', out, flush=True)
    except Exception as error:
        write_json('failure.json', {'error_type': type(error).__name__, 'error': str(error), 'traceback': traceback.format_exc()})
        write_json('results.json', result)
        raise


if __name__ == '__main__':
    main()
