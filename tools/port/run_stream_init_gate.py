"""Reproduce the genuine decoded-header -> DB_InitStreams boundary, sequentially.

Decode occurs in separate processes with the existing TASK-0011 probe; only its
validated 36-byte header reaches the new cursor/ownership probe. No APK/assets
are packaged, and previous device/scratch/evidence directories are retained.
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

ROOT = Path(__file__).resolve().parents[2]
ASSETS = {'code_pre_gfx': 'retained.inflate-genuine-oracle.json',
          'zombie_pentagon': 'retained.inflate-zombie-oracle.json'}

def sha(path):
    with Path(path).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--android-build', type=Path, required=True)
    ap.add_argument('--host-build', type=Path, required=True)
    ap.add_argument('--host-decoder-build', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--adb', type=Path, default=Path('D:/Android/Sdk/platform-tools/adb.exe'))
    ap.add_argument('--serial', default='AKWJ9X5116G04618')
    ap.add_argument('--retail-root', type=Path, default=ROOT.parent/'BlackOps64/build/Release')
    args = ap.parse_args()
    out = args.output.resolve()
    if not out.is_relative_to(ROOT) or out.exists():
        raise RuntimeError('Use a fresh output directory inside BlackOpsARM')
    out.mkdir(parents=True)
    commands = []
    env = dict(os.environ)
    for key in ['BO1_STREAM_TRACE', 'BO1_TRACE_ZONE', 'BO1_TRACE_ASSET', 'BO1_TRACE_STACK_OP']:
        env.pop(key, None)
    env['BO1_DIAG_LOG'] = str(out/'diagnostics.log')
    def run(label, argv):
        p = subprocess.run([str(a) for a in argv], cwd=out, env=env, capture_output=True,
                           text=True, encoding='utf-8', errors='replace', timeout=180)
        (out/(label+'.stdout.log')).write_text(p.stdout, encoding='utf-8')
        (out/(label+'.stderr.log')).write_text(p.stderr, encoding='utf-8')
        commands.append({'label': label, 'argv': [str(a) for a in argv], 'exit': p.returncode})
        (out/'invocations.json').write_text(json.dumps(commands, indent=2)+'\n', encoding='utf-8')
        if p.returncode:
            raise RuntimeError(f'{label}: exit {p.returncode}; diagnostics preserved')
        return p.stdout
    adb = [args.adb, '-s', args.serial]
    def shell(label, words):
        return run(label, adb+['shell', ' '.join(shlex.quote(str(w)) for w in words)])
    device = '/data/local/tmp/blackopsarm-phase1-stream-init-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S%f')
    shell('device-create', ['mkdir', device])
    run('device-metadata', adb+['shell', 'getprop ro.product.model; getprop ro.build.version.sdk; uname -a; cat /proc/meminfo'])
    native = args.android_build.resolve()/'bo1_stream_init_tests'
    native_decoder = args.android_build.resolve()/'bo1_checked_zone_tests'
    for label, path in [('probe', native), ('decoder', native_decoder)]:
        run('device-'+label+'-push', adb+['push', path, device+'/'+label])
        shell('device-'+label+'-chmod', ['chmod', '700', device+'/'+label])
        if shell('device-'+label+'-sha', ['sha256sum', device+'/'+label]).split()[0] != sha(path):
            raise RuntimeError('Device executable hash mismatch')
    oracles = {}
    for asset, leaf in ASSETS.items():
        oracle_path = ROOT/'docs/port/evidence/arm64_inflate'/leaf
        oracle = json.loads(oracle_path.read_text(encoding='utf-8'))
        source = args.retail_root/'zone/common'/f'{asset}.ff'
        if sha(source) != oracle['fileSHA256']:
            raise RuntimeError('Original retail input differs: '+asset)
        run(asset+'-push', adb+['push', source, f'{device}/{asset}.ff'])
        if shell(asset+'-device-sha', ['sha256sum', f'{device}/{asset}.ff']).split()[0] != oracle['fileSHA256']:
            raise RuntimeError('Device retail input hash mismatch')
        header = struct.pack('<9I', *oracle['xFileFields'])
        oracles[asset] = {'raw_sha256': sha(source), 'original_path': str(source.resolve()),
                         'oracle_path': oracle_path.relative_to(ROOT).as_posix(), 'oracle_sha256': sha(oracle_path),
                         'header_hex': header.hex(), 'header_sha256': hashlib.sha256(header).hexdigest(),
                         'decoded_bytes': oracle['decodedBytes'], 'decoded_adler32': int(oracle['decodedAdler32'], 16)}
    result = {'schema': 'BlackOpsARM.TASK-0012.gate.v1', 'recorded_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'device_serial': args.serial, 'device_directory': device, 'oracles': oracles,
              'scope': 'Init/cursor reset/detach/free/release only; synthetic cursor arithmetic reads/writes no payload; separate full decode processes',
              'platforms': {}}
    for target in ['windows_x86', 'windows_x64', 'honor_arm64']:
        on_device = target == 'honor_arm64'
        probe = device+'/probe' if on_device else str((args.host_build/target.split('_')[1]/'stream_init.exe').resolve())
        decoder = device+'/decoder' if on_device else str((args.host_decoder_build/target.split('_')[1]/'checked_zone.exe').resolve())
        def execute(label, binary, params):
            return shell(target+'-'+label, [binary]+params) if on_device else run(target+'-'+label, [binary]+params)
        headers, decodes = [], {}
        for asset, provenance in oracles.items():
            header = f'{device}/{asset}.header' if on_device else str(out/f'{target}-{asset}.header')
            source = f'{device}/{asset}.ff' if on_device else provenance['original_path']
            print(target, asset, 'separate full decode and header validation', flush=True)
            decoded = json.loads(execute(asset+'-decode', decoder, ['decode', source, header]))
            if decoded['header_hex'] != provenance['header_hex'] or decoded['plan']['decoded_bytes'] != provenance['decoded_bytes'] or decoded['decoded_adler32'] != provenance['decoded_adler32']:
                raise RuntimeError('Fresh full decode differs from independent genuine oracle')
            digest = shell(target+'-'+asset+'-header-sha', ['sha256sum', header]).split()[0] if on_device else sha(header)
            if digest != provenance['header_sha256']:
                raise RuntimeError('Extracted decoded header hash mismatch')
            headers += [header, str(provenance['decoded_bytes'])]
            decodes[asset] = decoded
        rows = [json.loads(line) for line in execute('stream-init', probe, headers).splitlines()]
        summary = rows[-1]
        if summary['cycles_passed'] != 12 or summary['init_calls'] != 61 or summary['preflight_rejections'] != 6 or summary['partial_failures'] != 6 or summary['baseline'] != summary['final']:
            raise RuntimeError('Lifecycle/rejection/cleanup gate failed')
        if any(not r[k] for r in rows[:-1] for k in ['seven_cursors_pass', 'reset_pass', 'detached_before_release', 'logical_baseline_restored']):
            raise RuntimeError('A genuine allocation cycle failed')
        checked_fixture = json.loads(execute('checked-fixtures', decoder, ['fixtures']).splitlines()[-1])
        if checked_fixture['fixtures_passed'] != (67 if on_device else 66) or checked_fixture['baseline'] != checked_fixture['final']:
            raise RuntimeError('Checked allocator regression failed')
        for row in rows[:-1]:
            asset = list(ASSETS)[row['asset_index']]
            if row['disk_sizes'] != [b['disk_bytes'] for b in decodes[asset]['plan']['blocks']]:
                raise RuntimeError('Block provenance mismatch')
            if row['logical_sizes'] != [b['logical_bytes'] for b in decodes[asset]['plan']['blocks']]:
                raise RuntimeError('Production logical size mismatch')
        result['platforms'][target] = {'probe_sha256': sha(native if on_device else probe),
            'decoder_sha256': sha(native_decoder if on_device else decoder), 'fresh_decodes': decodes,
            'checked_allocator_fixtures': checked_fixture, 'cycles': rows[:-1], 'summary': summary}
        (out/'results.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    for a, b in zip(result['platforms']['windows_x64']['cycles'], result['platforms']['honor_arm64']['cycles']):
        if a['logical_sizes'] != b['logical_sizes'] or a['disk_sizes'] != b['disk_sizes']:
            raise RuntimeError('Native64 allocation/stream contract mismatch')
    result['native64_logical_contract_matches'] = True
    (out/'results.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print('PASS genuine stream init gate:', out, flush=True)

if __name__ == '__main__':
    main()
