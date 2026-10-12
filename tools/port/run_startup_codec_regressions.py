"""Run the original codec suite plus allocation boundaries on actual HONOR."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--serial', required=True)
    parser.add_argument('--optimization', choices=('O0', 'O3'), required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    output = (root / args.output).resolve()
    if not output.is_relative_to(root) or output.exists():
        raise RuntimeError('Use fresh worktree evidence directory')
    output.mkdir(parents=True)
    build = (root / args.build).resolve()
    if not build.is_relative_to(root):
        raise RuntimeError('Build must belong to this worktree')
    binary = build / 'audit' / 'bo1_inflate_tests'
    fixtures = build / 'audit' / 'inflate-fixtures'
    adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', args.serial]
    remote = '/data/local/tmp/blackopsarm-startup-codec41-' + args.optimization
    commands = []

    def run(arguments):
        completed = subprocess.run(adb + arguments, capture_output=True, text=True)
        commands.append({'args': arguments, 'exit': completed.returncode,
                         'stdout': completed.stdout, 'stderr': completed.stderr})
        (output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
        if completed.returncode:
            raise RuntimeError('ADB command failed; evidence retained')
        return completed.stdout

    run(['get-state'])
    run(['shell', 'mkdir', '-p', remote])
    run(['push', str(binary), remote + '/inflate_tests'])
    digest = hashlib.sha256(binary.read_bytes()).hexdigest()
    if run(['shell', 'sha256sum', remote + '/inflate_tests']).split()[0] != digest:
        raise RuntimeError('Pushed executable differs')
    run(['push', str(fixtures), remote + '/inflate-fixtures'])
    retail = Path('D:/Games/Call of Duty - Black Ops/zone/Common')
    sources = {}
    for name in ('code_pre_gfx_mp.ff', 'code_pre_gfx.ff'):
        source = retail / name
        sha = hashlib.sha256(source.read_bytes()).hexdigest()
        run(['push', str(source), remote + '/' + name])
        if run(['shell', 'sha256sum', remote + '/' + name]).split()[0] != sha:
            raise RuntimeError('Pushed retail file differs')
        sources[name] = sha
    run(['shell', 'chmod', '700', remote + '/inflate_tests'])
    stdout = run(['shell', remote + '/inflate_tests', remote + '/code_pre_gfx_mp.ff', remote + '/code_pre_gfx.ff'])
    (output / 'test.log').write_text(stdout, encoding='utf-8')
    if 'PASS bounded inflate:' not in stdout or stdout.count('GENUINE status=Ok') != 2:
        raise RuntimeError('Codec suite coverage incomplete')
    result = {'passed': True, 'serial': args.serial, 'optimization': args.optimization,
              'binary_sha256': digest, 'retail_sha256': sources,
              'tests': 'Original codec controls plus chunk1/7/32768 peak and peak-minus-one boundaries'}
    (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('HONOR codec and working-allocation boundaries passed: ' + args.optimization)


if __name__ == '__main__':
    main()
