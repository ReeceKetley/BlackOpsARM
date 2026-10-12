"""Genuine first bootstrap provider on Windows and authorized HONOR; no asset archive."""
import argparse
import datetime
import hashlib
import json
import subprocess
from pathlib import Path
from bootstrap_default_oracle import reference
from run_rawfile_payload_gate import exact_record, verify_binary_architecture, verify_source_manifest

ROOT = Path(__file__).resolve().parents[2]


def expected_rows(oracle, width):
    rows = []
    for cycle in range(2):
        for r in oracle['records']:
            rows.append({'bootstrap_record': {'cycle': cycle, 'kind': r['kind'],
                'source_start': r['source_start'], 'source_end': r['source_end'], 'disk_start': r['disk_start'],
                'native_start': r['native64_start'] if width == 8 else r['disk_start'],
                'disk_bytes': r['disk_bytes'], 'native_bytes': r['native64_bytes'] if width == 8 else r['disk_bytes'],
                'adler32': r['adler32']}})
        rows.append({'bootstrap_cycle': {'cycle': cycle, 'alloc_type': cycle, 'source_end': oracle['source_end'],
            'disk_end': oracle['block4_disk_end'], 'native_end': oracle['block4_native64_end'] if width == 8 else oracle['block4_disk_end'],
            'registered_assets': 1, 'techniques': 2, 'passes': 2, 'arguments': 3, 'null_techniques': 112, 'fixups': 18,
            'gpu_handles': 0, 'temp_overwrite_verified': True, 'entry_baseline_restored': True, 'owner_baseline_restored': True}})
    rows.append({'schema': 'BlackOpsARM.TASK39.bootstrap-default.v1', 'pointer_bytes': width, 'cycles': 2,
        'source_end': oracle['source_end'], 'renderer_enabled': False, 'actual_provider_registration': True,
        'integrated_engine_startup': False})
    return rows


def check_rows(actual, expected):
    if len(actual) != len(expected):
        raise RuntimeError('Exact complete bootstrap inventory differs')
    for row, want in zip(actual, expected):
        if 'schema' in want:
            exact_record(row, want, 'bootstrap terminal')
        else:
            key = next(iter(want))
            if set(row) != {key}:
                raise RuntimeError('Bootstrap wrapper/placement differs')
            exact_record(row[key], want[key], 'bootstrap graph/cycle')


def main():
    ap = argparse.ArgumentParser()
    for name in ['host-build', 'android-build', 'output']:
        ap.add_argument('--' + name, type=Path, required=True)
    ap.add_argument('--game-root', type=Path, default=Path('D:/Games/Call of Duty - Black Ops'))
    args = ap.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    result = {'schema': 'BlackOpsARM.TASK39.bootstrap-default-gate.v1', 'status': 'running',
              'integrated_engine_startup': False, 'invocations': [], 'platforms': {},
              'collector_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}

    def save():
        (out / 'results.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')

    def run(label, argv, expected=0):
        try:
            c = subprocess.run([str(v) for v in argv], cwd=ROOT, capture_output=True, text=True, timeout=180)
        except (subprocess.TimeoutExpired, OSError) as error:
            result['invocations'].append({'label': label, 'argv': [str(v) for v in argv], 'exit': None, 'error': str(error)})
            save()
            raise
        (out / (label + '.stdout.log')).write_text(c.stdout, encoding='utf-8')
        (out / (label + '.stderr.log')).write_text(c.stderr, encoding='utf-8')
        result['invocations'].append({'label': label, 'argv': [str(v) for v in argv], 'exit': c.returncode})
        save()
        if c.returncode != expected or any(term in c.stderr for term in ['runtime error:', 'Assertion ', 'FAIL ']):
            raise RuntimeError(label + ': unexpected exit/assertion/sanitizer diagnostic ' + str(c.returncode))
        return c

    try:
        oracles = {label: reference(args.game_root / 'zone/common' / (name + '.ff'))
                   for label, name in [('mp', 'code_post_gfx_mp'), ('sp', 'code_post_gfx')]}
        result['oracles'] = oracles
        adb, serial = Path('D:/Android/Sdk/platform-tools/adb.exe'), 'AKWJ9X5116G04618'
        devices = run('adb-devices', [adb, 'devices', '-l']).stdout
        if not any(line.split()[:2] == [serial, 'device'] for line in devices.splitlines()):
            raise RuntimeError('Authorized HONOR not currently online')
        device = '/data/local/tmp/blackopsarm-bootstrap-default-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S%f')
        result['device_directory'] = device

        def shell(label, words, expected=0):
            return run(label, [adb, '-s', serial, 'shell', *words], expected)

        shell('device-create', ['mkdir', '-p', device])
        for label, oracle in oracles.items():
            run('device-retail-push-' + label, [adb, '-s', serial, 'push', oracle['original_path'], device + '/' + label + '.ff'])
            if shell('device-retail-sha-' + label, ['sha256sum', device + '/' + label + '.ff']).stdout.split()[0] != oracle['raw_sha256']:
                raise RuntimeError('Copied retail compressed SHA differs')
        targets = [('windows_x86', args.host_build / 'x86/bootstrap_default.exe', 4, 'x86'),
                   ('windows_x64', args.host_build / 'x64/bootstrap_default.exe', 8, 'x64'),
                   ('honor_arm64', args.android_build / 'bo1_bootstrap_default_tests', 8, 'arm64')]
        for target, binary, width, arch in targets:
            verify_binary_architecture(binary, arch)
            manifest = verify_source_manifest(binary.parent / 'source-manifest.json')
            native = target == 'honor_arm64'
            digest = hashlib.sha256(binary.read_bytes()).hexdigest()
            platform = result['platforms'][target] = {'binary_sha256': digest, 'manifest': manifest, 'zones': {}}
            executable = binary.resolve()
            if native:
                run('device-probe-push', [adb, '-s', serial, 'push', executable, device + '/probe'])
                shell('device-chmod', ['chmod', '700', device + '/probe'])
                if shell('device-probe-sha', ['sha256sum', device + '/probe']).stdout.split()[0] != digest:
                    raise RuntimeError('Pushed executable SHA differs')
                executable = device + '/probe'
            for label, oracle in oracles.items():
                prefix = device + '/' + label + '.decoded' if native else out / (target + '-' + label + '.decoded')
                source = device + '/' + label + '.ff' if native else oracle['original_path']
                def invoke(suffix, words, expected=0):
                    argv = [executable, *words]
                    return shell(target + '-' + label + '-' + suffix, argv, expected) if native else run(target + '-' + label + '-' + suffix, argv, expected)
                decoded = json.loads(invoke('decode', ['--decode-bootstrap', label, source, str(prefix)]).stdout)
                exact_record(decoded, {'schema': 'BlackOpsARM.TASK39.decode.v1', 'decoded_bytes': oracle['decoded_bytes'],
                    'decoded_adler32': oracle['decoded_adler32'], 'prefix_bytes': oracle['source_end'], 'buffer_released': True}, 'native decoder')
                prefix_sha = shell('device-prefix-sha-' + label, ['sha256sum', prefix]).stdout.split()[0] if native else hashlib.sha256(prefix.read_bytes()).hexdigest()
                if prefix_sha != oracle['prefix_sha256']:
                    raise RuntimeError('Native complete first-provider prefix differs')
                rows = [json.loads(line) for line in invoke('runtime', ['--bootstrap', label, str(prefix)]).stdout.splitlines() if line.strip()]
                expected = expected_rows(oracle, width)
                check_rows(rows, expected)
                malformed = [rows[:-1], rows + rows[-1:]]
                for key, value in [('native_bytes', True), ('source_start', -1)]:
                    bad = json.loads(json.dumps(rows)); bad[0]['bootstrap_record'][key] = value; malformed.append(bad)
                for key, value in [('registered_assets', True), ('temp_overwrite_verified', 1)]:
                    bad = json.loads(json.dumps(rows)); bad[len(oracle['records'])]['bootstrap_cycle'][key] = value; malformed.append(bad)
                for bad in malformed:
                    try:
                        check_rows(bad, expected)
                    except RuntimeError:
                        pass
                    else:
                        raise RuntimeError('Malformed bootstrap records accepted')
                if label == 'mp':
                    refusal = invoke('renderer-refusal', ['--bootstrap-renderer-on', label, str(prefix)], 134 if native else 3)
                    if refusal.stderr.count('Material graph fixture: renderer-enabled GPU operation unavailable') != 1:
                        raise RuntimeError('Missing explicit renderer-enabled refusal')
                platform['zones'][label] = {'native_decode': decoded, 'native_prefix_sha256': prefix_sha,
                                            'records': rows, 'malformed_records_rejected': len(malformed)}
                save()
        result['status'] = 'passed'
        save()
        print('PASS actual genuine bootstrap default graph registration on Windows x86/x64 and HONOR')
    except Exception as error:
        result['status'], result['failure'] = 'failed', str(error)
        save()
        raise


if __name__ == '__main__':
    main()
