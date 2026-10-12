"""Collect exact native FastCriticalSection behavior and original Windows oracle."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('host-build', 'arm-o0', 'arm-o3', 'output', 'serial'):
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use fresh worktree evidence')
    output.mkdir(parents=True)
    commands, rows = [], []

    def run(arguments, expected=0, diagnostic=None):
        completed = subprocess.run([str(a) for a in arguments], capture_output=True, text=True, timeout=30)
        commands.append({'args': [str(a) for a in arguments], 'exit': completed.returncode,
                         'stdout': completed.stdout, 'stderr': completed.stderr})
        (output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
        combined = completed.stdout + completed.stderr
        if completed.returncode != expected or 'test assertion failed' in combined or 'watchdog expired' in combined or 'runtime error:' in combined:
            raise RuntimeError('Native lock operation failed; evidence retained')
        if diagnostic and diagnostic not in combined:
            raise RuntimeError('Actual release assertion diagnostic differs')
        return completed.stdout

    # Validate the separately extracted historical writer/releases, not the port.
    for arch, width in (('x86', 4), ('x64', 8)):
        directory = ROOT / 'build-android-audit/fast_critical_section_original_run2' / arch
        if json.loads((directory / 'run-result.json').read_text(encoding='utf-8-sig'))['exit'] != 0:
            raise RuntimeError('Original Windows oracle failed')
        actual = json.loads((directory / 'run.log').read_text(encoding='utf-8-sig'))
        expected = {'original_fast_critical_section': {'pointer_bytes': width, 'lock_bytes': 8,
                    'writer_updates': 3000, 'reader_checks': 3000, 'queued_reader': True,
                    'nested_readers': True, 'canaries': True, 'zero_final_counters': True, 'passed': True}}
        if actual != expected or any(type(actual['original_fast_critical_section'][key]) is not type(value)
                for key, value in expected['original_fast_critical_section'].items()):
            raise RuntimeError('Original Windows protocol differs from independent acceptance')
    adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', args.serial]
    run(adb + ['get-state'])
    cases = [('x86', 4, args.host_build + '/x86', 'native_fast_critical_section.exe'),
             ('x64', 8, args.host_build + '/x64', 'native_fast_critical_section.exe'),
             ('arm64_O0', 8, args.arm_o0, 'bo1_native_fast_critical_section_tests'),
             ('arm64_O3', 8, args.arm_o3, 'bo1_native_fast_critical_section_tests')]
    for architecture, width, directory, filename in cases:
        build = (ROOT / directory).resolve()
        if not build.is_relative_to(ROOT):
            raise RuntimeError('Build outside worktree')
        binary = build / filename
        manifest = json.loads((build / 'source-manifest.json').read_text(encoding='utf-8-sig'))
        if any(hashlib.sha256((ROOT / path).read_bytes()).hexdigest() != sha.lower() for path, sha in manifest.items()):
            raise RuntimeError('Build input no longer current')
        data = binary.read_bytes()
        sha = hashlib.sha256(data).hexdigest()
        arm = architecture.startswith('arm64')
        prefix = [binary]
        if arm:
            if data[:5] != b'\x7fELF\x02' or int.from_bytes(data[18:20], 'little') != 183:
                raise RuntimeError('Expected native ARM64 ELF')
            remote = '/data/local/tmp/blackopsarm-fcs45-' + architecture
            run(adb + ['shell', 'mkdir', '-p', remote])
            run(adb + ['push', binary, remote + '/lock_tests'])
            if run(adb + ['shell', 'sha256sum', remote + '/lock_tests']).split()[0] != sha:
                raise RuntimeError('Pushed native lock binary differs')
            run(adb + ['shell', 'chmod', '700', remote + '/lock_tests'])
            prefix = adb + ['shell', remote + '/lock_tests']
        stdout = run(prefix)
        actual = json.loads(stdout)
        expected = {'fast_critical_section': {'pointer_bytes': width, 'size': 8, 'alignment': 4,
                    'read_offset': 0, 'write_offset': 4, 'nested_reads': 3, 'simultaneous_readers': 2,
                    'blocked_writer': True, 'queued_reader': True, 'independent_sections': 2,
                    'writer_threads': 4, 'reader_threads': 6, 'total_writes': 10000,
                    'reader_checks': 12000, 'canaries': True, 'final_read_count': 0,
                    'final_write_count': 0, 'passed': True}}
        if actual != expected or any(type(actual['fast_critical_section'][key]) is not type(value)
                for key, value in expected['fast_critical_section'].items()):
            raise RuntimeError('Native protocol record differs')
        for counter in ('read', 'write'):
            for value in ('zero', 'negative_one', 'negative_min'):
                run(prefix + [counter + '_' + value], expected=134 if arm else 3,
                    diagnostic='critSect->' + counter + 'Count > 0')
        rows.append({'architecture': architecture, 'pointer_bytes': width, 'passed': True,
                     'negative_controls': 6, 'binary_sha256': sha})
    (output / 'result.json').write_text(json.dumps({'passed': True, 'runs': rows,
            'original_windows_oracle_passed': True, 'native_reader_admission': 'Contract test caller; actual production writer/releases execute',
            'dvar_init_executed': False}, indent=2) + '\n', encoding='utf-8')
    print('Native original writer/releases, fixed32 contention and24 signed-release controls pass Windows/HONOR O0/O3')


if __name__ == '__main__':
    main()
