"""Collect actual startup services with original Windows branch parity."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
from check_startup_services import validate

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('original-host', 'native-host', 'arm-o0', 'arm-o3', 'output', 'serial'):
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use fresh worktree collection directory')
    output.mkdir(parents=True)
    commands, runs, controls = [], [], []

    def run(argv, expected=0, diagnostic=None):
        result = subprocess.run([str(x) for x in argv], capture_output=True, text=True, timeout=30)
        commands.append(dict(argv=[str(x) for x in argv], exit=result.returncode,
                             stdout=result.stdout, stderr=result.stderr))
        (output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
        text = result.stdout + result.stderr
        if result.returncode != expected or any(bad in text for bad in ('runtime error:', 'test failed:', 'Startup assertion', 'watchdog expired')):
            raise RuntimeError('Actual startup services command failed; evidence retained')
        if diagnostic and 'Startup service dependency unavailable: ' + diagnostic not in text:
            raise RuntimeError('Unavailable dependency diagnostic differs')
        return result.stdout

    adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', args.serial]
    if run(adb + ['get-state']).strip() != 'device':
        raise RuntimeError('Device unavailable')
    cases = []
    for policy, directory in (('original', args.original_host), ('native', args.native_host)):
        for arch, width in (('x86', 4), ('x64', 8)):
            cases.append((policy + '_' + arch, width, ROOT / directory / arch, 'native_startup_services.exe', False, policy == 'original'))
    for name, directory in (('arm64_O0', args.arm_o0), ('arm64_O3', args.arm_o3)):
        cases.append((name, 8, ROOT / directory, 'bo1_native_startup_services_tests', True, False))
    oracle, manifests = None, []
    modes = dict(expression='command expression integration', localization='localization token',
                 exec='player configuration name', time='engine millisecond clock', server='client command forwarding')
    for name, width, directory, filename, android, original in cases:
        directory = directory.resolve()
        if not directory.is_relative_to(ROOT) or read(directory / 'build-result.json')['completed'] is not True:
            raise RuntimeError('Current successful worktree build required')
        result = read(directory / 'build-result.json')
        if not android and result['original_windows_policy'] is not original:
            raise RuntimeError('Windows branch policy differs')
        manifest = read(directory / 'source-manifest.json')
        if any(digest(ROOT / path) != sha.lower() for path, sha in manifest.items()):
            raise RuntimeError('Selected build source is no longer current: ' + name)
        manifests.append(manifest)
        binary = directory / filename
        sha = digest(binary)
        invocation = [str(binary)]
        if android:
            remote = '/data/local/tmp/blackopsarm-startup-services46-' + output.name
            run(adb + ['shell', 'mkdir', '-p', remote])
            remote_binary = remote + '/' + name
            run(adb + ['push', binary, remote_binary])
            if run(adb + ['shell', 'sha256sum', remote_binary]).split()[0] != sha:
                raise RuntimeError('Device binary differs')
            run(adb + ['shell', 'chmod', '700', remote_binary])
            invocation = adb + ['shell', remote_binary]
        stdout = run(invocation)
        rows = validate(stdout, width)
        normalized = json.loads(json.dumps(rows))
        for row in normalized:
            if 'startup_services' in row:
                row['startup_services']['pointer_bytes'] = 0
        if oracle is None:
            if not original:
                raise RuntimeError('Original Windows branch must establish oracle first')
            oracle = normalized
        elif normalized != oracle:
            raise RuntimeError('Production state differs from original Windows branch oracle')
        runs.append(dict(name=name, binary_sha256=sha, records=len(rows), actual=rows))
        for mode, diagnostic in modes.items():
            fatal_stdout = run(invocation + [mode], 134 if android else 3, diagnostic)
            if '"startup_services"' in fatal_stdout:
                raise RuntimeError('Unavailable dependency emitted success summary')
            controls.append(dict(name=name, mode=mode, exit=134 if android else 3, diagnostic=diagnostic))
    for manifest in manifests:
        if any(digest(ROOT / path) != sha.lower() for path, sha in manifest.items()):
            raise RuntimeError('Source changed during startup validation')
    (output / 'result.json').write_text(json.dumps(dict(passed=True, runs=runs, controls=controls,
        original_windows_branch_parity=True, full_com_init=False,
        scope='Integrated production startup services; subset driver settings, explicit unavailable dependencies.'), indent=2) + '\n', encoding='utf-8')
    print('Six startup-service runs and30 exact unavailable controls pass')


if __name__ == '__main__':
    main()
