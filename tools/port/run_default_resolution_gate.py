"""Actual default services with genuine provider/map and SL/MT ownership."""
import argparse
import datetime
import hashlib
import json
import subprocess
from pathlib import Path
from bootstrap_default_oracle import reference as bootstrap_reference
from stringtable_payload_oracle import reference as table_reference
from techset_oracle import reference as stub_reference
from run_rawfile_payload_gate import exact_record, verify_binary_architecture, verify_source_manifest

ROOT = Path(__file__).resolve().parents[2]


def check_rows(rows, width):
    if len(rows) != 3:
        raise RuntimeError('Exactly two ownership cycles and one terminal required')
    for cycle in range(2):
        if not isinstance(rows[cycle], dict) or set(rows[cycle]) != {'default_resolution_cycle'}:
            raise RuntimeError('Exact ordered resolution wrapper required')
        exact_record(rows[cycle]['default_resolution_cycle'], {
            'cycle': cycle, 'provider_alloc_type': cycle, 'map_alloc_type': cycle,
            'provider_source_end': 10017, 'map_source_end': 66226, 'registered_assets': 3,
            'default_count_peak': 1, 'interned_name_id': 2, 'name_bytes': 19, 'name_adler32': 1186006616,
            'table_queries': 1594, 'provider_graph_shared': True, 'repeat_resolution_identity': True,
            'temp_overwrite_verified': True, 'pool_entry_owner_baselines_restored': True,
            'sl_mt_baseline_restored': True}, 'actual resolution cycle')
    exact_record(rows[2], {'schema': 'BlackOpsARM.TASK40.default-resolution.v1', 'pointer_bytes': width,
        'cycles': 2, 'actual_default_services': True, 'actual_sl_mt_name_ownership': True,
        'integrated_engine_startup': False}, 'actual resolution terminal')


def main():
    ap = argparse.ArgumentParser()
    for name in ['host-build', 'android-build', 'output']:
        ap.add_argument('--' + name, type=Path, required=True)
    ap.add_argument('--game-root', type=Path, default=Path('D:/Games/Call of Duty - Black Ops'))
    args = ap.parse_args(); out = args.output.resolve(); out.mkdir(parents=True, exist_ok=False)
    result = {'schema': 'BlackOpsARM.TASK40.default-resolution-gate.v1', 'status': 'running',
              'integrated_engine_startup': False, 'invocations': [], 'platforms': {},
              'collector_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}

    def save():
        (out / 'results.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')

    def run(label, argv, expected=0):
        try:
            c = subprocess.run([str(v) for v in argv], cwd=ROOT, capture_output=True, text=True, timeout=180)
        except (subprocess.TimeoutExpired, OSError) as error:
            result['invocations'].append({'label': label, 'argv': [str(v) for v in argv], 'exit': None, 'error': str(error)}); save(); raise
        (out / (label + '.stdout.log')).write_text(c.stdout, encoding='utf-8')
        (out / (label + '.stderr.log')).write_text(c.stderr, encoding='utf-8')
        result['invocations'].append({'label': label, 'argv': [str(v) for v in argv], 'exit': c.returncode}); save()
        if c.returncode != expected or any(term in c.stderr for term in ['runtime error:', 'Assertion ', 'FAIL ']):
            raise RuntimeError(label + ': unexpected exit/assertion/sanitizer diagnostic ' + str(c.returncode))
        return c

    try:
        provider = bootstrap_reference(args.game_root / 'zone/common/code_post_gfx_mp.ff')
        frozen = json.loads((ROOT / 'docs/port/evidence/arm64_inflate/retained.inflate-zombie-oracle.json').read_text(encoding='utf-8-sig'))
        table = table_reference(args.game_root / 'zone/common/zombie_pentagon.ff', frozen)
        stub = stub_reference(table)
        result['oracles'] = {'provider': provider, 'map_table': table, 'map_stub': stub}
        adb, serial = Path('D:/Android/Sdk/platform-tools/adb.exe'), 'AKWJ9X5116G04618'
        devices = run('adb-devices', [adb, 'devices', '-l']).stdout
        if not any(line.split()[:2] == [serial, 'device'] for line in devices.splitlines()):
            raise RuntimeError('Authorized HONOR not currently online')
        device = '/data/local/tmp/blackopsarm-default-resolution-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S%f')
        result['device_directory'] = device

        def shell(label, words, expected=0):
            return run(label, [adb, '-s', serial, 'shell', *words], expected)

        shell('device-create', ['mkdir', '-p', device])
        run('device-provider-push', [adb, '-s', serial, 'push', provider['original_path'], device + '/provider.ff'])
        if shell('device-provider-sha', ['sha256sum', device + '/provider.ff']).stdout.split()[0] != provider['raw_sha256']:
            raise RuntimeError('Copied provider SHA differs')
        reuse = ROOT / 'build-android-audit/bootstrap_default_prereq_O3_root_run1/results.json'
        map_source = device + '/map.ff'
        if reuse.is_file():
            old = json.loads(reuse.read_text())
            map_source = old.get('reused_original_device_directory', old['device_directory']) + '/zombie_pentagon.ff'
            result['reused_map_source'] = map_source
        else:
            run('device-map-push', [adb, '-s', serial, 'push', table['original_path'], map_source])
        if shell('device-map-sha', ['sha256sum', map_source]).stdout.split()[0] != table['raw_sha256']:
            raise RuntimeError('Authorized retail map SHA differs')
        for target, binary, width, arch in [
            ('windows_x86', args.host_build / 'x86/default_resolution.exe', 4, 'x86'),
            ('windows_x64', args.host_build / 'x64/default_resolution.exe', 8, 'x64'),
            ('honor_arm64', args.android_build / 'bo1_default_resolution_tests', 8, 'arm64')]:
            verify_binary_architecture(binary, arch)
            manifest = verify_source_manifest(binary.parent / 'source-manifest.json')
            digest = hashlib.sha256(binary.read_bytes()).hexdigest(); native = target == 'honor_arm64'
            platform = result['platforms'][target] = {'binary_sha256': digest, 'manifest': manifest}
            executable = binary.resolve()
            if native:
                run('device-binary-push', [adb, '-s', serial, 'push', executable, device + '/probe']); shell('device-chmod', ['chmod', '700', device + '/probe'])
                if shell('device-binary-sha', ['sha256sum', device + '/probe']).stdout.split()[0] != digest:
                    raise RuntimeError('Pushed executable SHA differs')
                executable = device + '/probe'

            def invoke(label, words, expected=0):
                argv = [executable, *words]
                return shell(target + '-' + label, argv, expected) if native else run(target + '-' + label, argv, expected)

            pp = device + '/provider.decoded' if native else out / (target + '-provider.decoded')
            mp = device + '/map.decoded' if native else out / (target + '-map.decoded')
            decoded = json.loads(invoke('provider-decode', ['--decode-bootstrap', 'mp', device + '/provider.ff' if native else provider['original_path'], str(pp)]).stdout)
            exact_record(decoded, {'schema': 'BlackOpsARM.TASK39.decode.v1', 'decoded_bytes': provider['decoded_bytes'], 'decoded_adler32': provider['decoded_adler32'], 'prefix_bytes': 10017, 'buffer_released': True}, 'provider decoder')
            map_decoded = json.loads(invoke('map-decode', ['--decode-resolution-map', map_source if native else table['original_path'], str(mp)]).stdout)
            exact_record(map_decoded, {'decoded_bytes': table['decoded_bytes'], 'decoded_adler32': table['decoded_adler32'], 'prefix_bytes': 66226, 'buffer_released': True}, 'map decoder')
            prefix_hashes = {}
            for label, prefix, expected in [('provider', pp, provider['prefix_sha256']), ('map', mp, stub['prefix_sha256'])]:
                prefix_hashes[label] = shell('device-' + label + '-prefix-sha', ['sha256sum', str(prefix)]).stdout.split()[0] if native else hashlib.sha256(prefix.read_bytes()).hexdigest()
                if prefix_hashes[label] != expected:
                    raise RuntimeError('Independent/native complete prefix SHA differs')
            rows = [json.loads(line) for line in invoke('runtime', ['--default-resolution', str(pp), str(mp)]).stdout.splitlines() if line.strip()]; check_rows(rows, width)
            malformed = [rows[:-1], rows + rows[-1:]]
            for key, value in [('registered_assets', True), ('interned_name_id', 0), ('provider_graph_shared', 1), ('sl_mt_baseline_restored', False)]:
                bad = json.loads(json.dumps(rows)); bad[0]['default_resolution_cycle'][key] = value; malformed.append(bad)
            for bad in malformed:
                try:
                    check_rows(bad, width)
                except RuntimeError:
                    pass
                else:
                    raise RuntimeError('Malformed ownership record accepted')
            refusal = invoke('missing-default', ['--missing-default'], 134 if native else 3)
            before = [json.loads(line) for line in refusal.stdout.splitlines() if line.strip()]
            if len(before) != 1 or set(before[0]) != {'missing_default_before'}:
                raise RuntimeError('Exact missing-provider pre-fatal observation required')
            exact_record(before[0]['missing_default_before'], {'registered_assets': 0, 'default_count': 0, 'sl_allocations': 0}, 'before genuine missing default fatal')
            if refusal.stderr != "Could not load default asset 'default' for asset type 'techset'.\nTried to load asset 'missing-default-control'.":
                raise RuntimeError('Exact production missing-provider error required')
            platform.update(native_provider_decode=decoded, native_map_decode=map_decoded, native_prefix_sha256=prefix_hashes,
                            records=rows, malformed_records_rejected=len(malformed), actual_missing_provider_fatal_verified=True); save()
        result['status'] = 'passed'; save()
        print('PASS genuine retained-provider default resolution and SL/MT ownership on Windows x86/x64 and HONOR')
    except Exception as error:
        result['status'], result['failure'] = 'failed', str(error); save(); raise


if __name__ == '__main__':
    main()
