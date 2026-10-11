"""Reproduce TASK-0011 sequential fresh-decode and allocation-only measurements.

All host builds/output stay in BlackOpsARM; retail input is read-only. No cache
flush, device settings, APK installation, asset parsing, or artifact deletion.
"""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import math
import platform
import shlex
import statistics
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
ASSETS = {
    'code_pre_gfx': 'retained.inflate-genuine-oracle.json',
    'zombie_pentagon': 'retained.inflate-zombie-oracle.json',
}


def sha(path):
    with Path(path).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def stats(samples):
    result = {'count': len(samples)}
    for key in ['create_ns', 'allocate_ns', 'release_ns', 'total_phase_ns']:
        values = sorted(s[key] for s in samples)
        result[key] = {'median': statistics.median(values), 'p95_nearest_rank': values[math.ceil(.95 * len(values)) - 1],
                       'min': values[0], 'max': values[-1]}
    return result


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--host-build', type=Path, required=True)
    ap.add_argument('--android-build', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--serial', default='AKWJ9X5116G04618')
    ap.add_argument('--adb', type=Path, default=Path('D:/Android/Sdk/platform-tools/adb.exe'))
    ap.add_argument('--retail-root', type=Path, default=ROOT.parent/'BlackOps64/build/Release')
    ap.add_argument('--warm', type=int, default=31)
    ap.add_argument('--warmups', type=int, default=3)
    ap.add_argument('--cold', type=int, default=20)
    ap.add_argument('--decode-runs', type=int, default=3)
    args = ap.parse_args()
    out = args.output.resolve()
    if not out.is_relative_to(ROOT) or out.exists():
        raise RuntimeError('Use a fresh output directory inside BlackOpsARM')
    out.mkdir(parents=True)
    adb = [str(args.adb), '-s', args.serial]
    invocations = []

    def run(label, command, expected=0):
        p = subprocess.run([str(c) for c in command], cwd=ROOT, capture_output=True, text=True,
                           encoding='utf-8', errors='replace', timeout=180)
        (out/(label+'.stdout.log')).write_text(p.stdout, encoding='utf-8')
        (out/(label+'.stderr.log')).write_text(p.stderr, encoding='utf-8')
        invocations.append({'label': label, 'argv': [str(c) for c in command], 'exit': p.returncode,
                            'expected_exit': expected})
        (out/'invocations.json').write_text(json.dumps(invocations, indent=2)+'\n')
        if p.returncode != expected:
            raise RuntimeError(f'{label}: exit {p.returncode} expected {expected}; evidence retained')
        return p.stdout

    def shell(label, words, expected=0):
        return run(label, adb+['shell', ' '.join(shlex.quote(str(w)) for w in words)], expected)

    device = '/data/local/tmp/blackopsarm-phase1-checked-zone-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S%f')
    shell('device-create', ['mkdir', device])
    run('device-metadata', adb+['shell', 'getprop ro.product.model; getprop ro.build.version.sdk; uname -a; cat /proc/meminfo; cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor'])
    native = args.android_build.resolve()/'bo1_checked_zone_tests'
    run('device-probe-push', adb+['push', native, device+'/probe'])
    shell('device-chmod', ['chmod', '700', device+'/probe'])
    oracles = {}
    for asset, filename in ASSETS.items():
        oracle = json.loads((ROOT/'docs/port/evidence/arm64_inflate'/filename).read_text())
        source = args.retail_root/'zone/common'/f'{asset}.ff'
        if sha(source) != oracle['fileSHA256']:
            raise RuntimeError('Retail input differs from frozen independent oracle: '+asset)
        run(asset+'-push', adb+['push', source, f'{device}/{asset}.ff'])
        check = shell(asset+'-device-sha', ['sha256sum', f'{device}/{asset}.ff']).split()[0]
        if check != oracle['fileSHA256']:
            raise RuntimeError('Device input digest mismatch')
        oracles[asset] = {'original_path': str(source.resolve()), 'raw_sha256': check,
                          'oracle_path': 'docs/port/evidence/arm64_inflate/'+filename,
                          'oracle_sha256': sha(ROOT/'docs/port/evidence/arm64_inflate'/filename),
                          'fields': oracle['xFileFields'], 'header_hex': struct.pack('<9I', *oracle['xFileFields']).hex(),
                          'header_sha256': hashlib.sha256(struct.pack('<9I', *oracle['xFileFields'])).hexdigest(),
                          'decoded_bytes': oracle['decodedBytes'], 'decoded_adler32': int(oracle['decodedAdler32'], 16)}
    result = {'schema': 'BlackOpsARM.TASK-0011.measurements.v1',
              'recorded_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'host': platform.platform(), 'device_serial': args.serial, 'device_directory': device,
              'conditions': {'warm': f'{args.warmups} excluded warmups then {args.warm} repeated owner create/allocate/release samples in one process; Android remaps every time; Windows pool retained',
                             'cold': 'Each allocation sample in a fresh process; filesystem cache/CPU frequency/thermal state uncontrolled, not storage-cold',
                             'decode': 'Separate fresh processes, path streaming, one decoded vector; no zone allocation; checksum/full end/length validated',
                             'allocator': 'Only 36 bytes extracted by successful genuine decode; no decoded payload remains in allocation processes',
                             'timing': 'Monotonic CLOCK_MONOTONIC/QPC; /proc/PSAPI reads excluded; concurrent 1ms resident sampler enabled',
                             'quantiles': 'median (average middle pair if even); p95 nearest rank ceil(0.95*N)',
                             'memory': 'HWM is process-lifetime cumulative. Sampler may miss sub-ms peaks. Android mprotect accessible bytes are not physical commitment; Windows private commit/working set are distinct metrics.'},
              'oracles': oracles, 'platforms': {}}
    for target in ['windows_x86', 'windows_x64', 'honor_arm64']:
        on_device = target == 'honor_arm64'
        binary = device+'/probe' if on_device else str((args.host_build/target.split('_')[1]/'checked_zone.exe').resolve())
        def execute(label, params, expected=0):
            if on_device:
                return shell(target+'-'+label, [binary]+params, expected)
            return run(target+'-'+label, [binary]+params, expected)
        fixtures = execute('fixtures', ['fixtures'])
        summary = json.loads(fixtures.splitlines()[-1])
        if summary['fixtures_passed'] < 66 or summary['baseline'] != summary['final']:
            raise RuntimeError('Fixture cleanup failed')
        failure = execute('backend-failure', ['backend-failure'], 134 if on_device else 23)
        target_result = {'binary_sha256': sha(native if on_device else binary), 'fixtures': summary,
                         'real_backend_failure_exit': 134 if on_device else 23, 'assets': {}}
        for asset, provenance in oracles.items():
            print(target, asset, 'decode/validate/warm/cold', flush=True)
            header_path = f'{device}/{asset}.header' if on_device else str(out/f'{target}-{asset}.header')
            input_path = f'{device}/{asset}.ff' if on_device else provenance['original_path']
            decodes = []
            for i in range(args.decode_runs):
                decoded = json.loads(execute(f'{asset}-decode-{i}', ['decode', input_path, header_path]))
                if decoded['header_hex'] != provenance['header_hex'] or decoded['plan']['decoded_bytes'] != provenance['decoded_bytes'] or decoded['decoded_adler32'] != provenance['decoded_adler32']:
                    raise RuntimeError('Native decode differs from genuine independent oracle')
                decodes.append(decoded)
            header_hash = shell(target+'-'+asset+'-header-sha', ['sha256sum', header_path]).split()[0] if on_device else sha(header_path)
            if header_hash != provenance['header_sha256']:
                raise RuntimeError('Extracted header provenance mismatch')
            validation = execute(asset+'-genuine-validation', ['validate', header_path])
            if not json.loads(validation.splitlines()[-1])['genuine_production_parity']:
                raise RuntimeError('Production DB allocation parity failed')
            warm = json.loads(execute(asset+'-warm', ['alloc', header_path, str(args.warm), str(args.warmups)]))
            cold = [json.loads(execute(f'{asset}-cold-{i}', ['alloc', header_path, '1', '0'])) for i in range(args.cold)]
            cold_samples = [r['samples'][0] for r in cold]
            for sample in warm['samples']+cold_samples:
                if not sample['logical_ownership_restored'] or (not sample['raw_counters_match'] and not sample['pool_initialized_and_retained']):
                    raise RuntimeError('Allocation cleanup failed')
            target_result['assets'][asset] = {'decode_runs': decodes, 'warm': warm,
                'warm_statistics': stats(warm['samples']), 'cold_process_runs': cold,
                'cold_statistics': stats(cold_samples), 'validated_header_sha256': header_hash}
        result['platforms'][target] = target_result
        (out/'measurements.json').write_text(json.dumps(result, indent=2)+'\n')
    # Compare exact fields and all derived production sizes between native ABIs.
    for asset in ASSETS:
        arm = result['platforms']['honor_arm64']['assets'][asset]['warm']['plan']
        win = result['platforms']['windows_x64']['assets'][asset]['warm']['plan']
        if arm != win:
            raise RuntimeError('Windows x64/ARM64 logical contract mismatch')
    result['native64_plans_match'] = True
    (out/'measurements.json').write_text(json.dumps(result, indent=2)+'\n')
    print('PASS checked allocation measurements:', out, flush=True)


if __name__ == '__main__':
    main()
