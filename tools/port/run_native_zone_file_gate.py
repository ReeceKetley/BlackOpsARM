"""Validate real native reader/owner rows against independent retail metadata."""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import subprocess
from startup_zone_oracle import inspect, RAW_SHA


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--serial', required=True)
    parser.add_argument('--optimization', choices=('O0', 'O3'), required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    build, output = (root / args.build).resolve(), (root / args.output).resolve()
    if not build.is_relative_to(root) or not output.is_relative_to(root) or output.exists():
        raise RuntimeError('Use worktree build and fresh evidence output')
    output.mkdir(parents=True)
    binary = build / 'bo1_native_zone_file_tests'
    binary_data = binary.read_bytes()
    if binary_data[:5] != b'\x7fELF\x02' or struct.unpack_from('<H', binary_data, 18)[0] != 183:
        raise RuntimeError('Expected native ARM64 ELF')
    retail = Path('D:/Games/Call of Duty - Black Ops/zone/Common/code_pre_gfx_mp.ff')
    oracle = inspect(retail)
    adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', args.serial]
    remote = '/data/local/tmp/blackopsarm-startup-owner41-final-' + args.optimization
    commands = []

    def run(arguments):
        completed = subprocess.run(adb + arguments, capture_output=True, text=True)
        commands.append({'args': arguments, 'exit': completed.returncode,
                         'stdout': completed.stdout, 'stderr': completed.stderr})
        (output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
        if completed.returncode:
            raise RuntimeError('Device operation failed; evidence retained')
        return completed.stdout

    run(['get-state'])
    run(['shell', 'mkdir', '-p', remote])
    run(['push', str(binary), remote + '/owner_tests'])
    sha = hashlib.sha256(binary_data).hexdigest()
    if run(['shell', 'sha256sum', remote + '/owner_tests']).split()[0] != sha:
        raise RuntimeError('Pushed executable differs')
    run(['push', str(retail), remote + '/code_pre_gfx_mp.ff'])
    if run(['shell', 'sha256sum', remote + '/code_pre_gfx_mp.ff']).split()[0] != RAW_SHA:
        raise RuntimeError('Pushed genuine source differs')
    run(['shell', 'chmod', '700', remote + '/owner_tests'])
    stdout = run(['shell', remote + '/owner_tests', remote + '/code_pre_gfx_mp.ff'])
    (output / 'test.log').write_text(stdout, encoding='utf-8')
    rows = [json.loads(line)['native_zone_file'] for line in stdout.splitlines()]
    if len(rows) != 4:
        raise RuntimeError('Expected four real low/high owner cycles')
    for index, row in enumerate(rows):
        if (row['cycle'] != index or row['decoded'] != oracle['decoded_bytes']
                or row['adler32'] != oracle['decoded_adler32'] or row['padding'] != 27
                or row['owner_capacity'] != 442368 or not 65536 < row['codec_peak'] < 8 * 1024 * 1024):
            raise RuntimeError('Runtime row differs from independent source/owner expectation')
    result = {'passed': True, 'serial': args.serial, 'optimization': args.optimization,
              'binary_sha256': sha, 'retail_sha256': RAW_SHA, 'rows': rows,
              'scope': 'Production reader/owner; no asset dispatch, engine startup or gameplay'}
    (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('Final HONOR native reader/owner passes: ' + args.optimization)


if __name__ == '__main__':
    main()
