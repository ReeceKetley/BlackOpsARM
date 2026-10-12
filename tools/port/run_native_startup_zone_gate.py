"""Run the production startup loader library on HONOR with explicit test inputs."""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import subprocess
from check_startup_zone_runtime import validate
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
    binary = build / 'bo1_native_startup_zone_tests'
    data = binary.read_bytes()
    if data[:5] != b'\x7fELF\x02' or struct.unpack_from('<H', data, 18)[0] != 183:
        raise RuntimeError('Expected native ARM64 ELF')
    retail = Path('D:/Games/Call of Duty - Black Ops/zone/Common/code_pre_gfx_mp.ff')
    oracle = inspect(retail)
    adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', args.serial]
    remote = '/data/local/tmp/blackopsarm-startup-zone41-' + args.optimization
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
    run(['push', str(binary), remote + '/startup_tests'])
    sha = hashlib.sha256(data).hexdigest()
    if run(['shell', 'sha256sum', remote + '/startup_tests']).split()[0] != sha:
        raise RuntimeError('Pushed executable differs')
    run(['push', str(retail), remote + '/code_pre_gfx_mp.ff'])
    if run(['shell', 'sha256sum', remote + '/code_pre_gfx_mp.ff']).split()[0] != RAW_SHA:
        raise RuntimeError('Pushed genuine source differs')
    run(['shell', 'chmod', '700', remote + '/startup_tests'])
    stdout = run(['shell', remote + '/startup_tests', remote + '/code_pre_gfx_mp.ff'])
    (output / 'test.log').write_text(stdout, encoding='utf-8')
    result = validate(stdout, 8, oracle)
    result.update(serial=args.serial, optimization=args.optimization,
                  binary_sha256=sha, retail_sha256=RAW_SHA)
    (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('HONOR actual root/Ptr/Add loading of all MP71 assets passes: ' + args.optimization)


if __name__ == '__main__':
    main()
