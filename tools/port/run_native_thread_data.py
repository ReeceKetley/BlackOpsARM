"""Independent exact-layout and negative-control validation of real T5 TLS bindings."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
from thread_data_layout_oracle import EXPECTED

ROOT = Path(__file__).resolve().parents[2]
CONTROLS = {
    'unregistered_get': 'unregistered thread (-1)', 'unregistered_set': 'unregistered thread (-1)',
    'unregistered_context': 'unregistered thread (-1)', 'invalid_role_negative': 'unsupported role (-1)',
    'invalid_role_count': 'unsupported role (15)', 'unsupported_server': 'unsupported role (10)',
    'slot_negative_get': 'invalid TLS slot (-1)', 'slot_count_get': 'invalid TLS slot (5)',
    'slot_negative_set': 'invalid TLS slot (-1)', 'slot_count_set': 'invalid TLS slot (5)',
    'role_switch': 'thread role switch (1)', 'main_retire': 'main retirement unsupported (0)',
    'duplicate_main': 'role already claimed (0)', 'duplicate_database': 'role already claimed (13)',
    'exit_without_retirement': 'worker exited without retirement (13)',
    'retired_get': 'retired thread (-1)', 'retired_rebind': 'retired role cannot rebind (13)',
    'retired_role_reuse': 'role already claimed (13)',
}


def validate(stdout, width):
    rows = [json.loads(line) for line in stdout.splitlines()]
    model = EXPECTED[width]
    keys = {'cmdargs_bytes': 'cmd_args_bytes', 'trace_bytes': 'trace_info_bytes',
            'checkcount_bytes': 'trace_count_bytes', 'cmd_item_offset': 'item_offset',
            'cmd_argv_offset': 'argv_offset', 'cmd_text_offset': 'text_offset',
            'cmd_argvpool_offset': 'argv_pool_offset', 'cmd_usedtext_offset': 'used_text_offset',
            'trace_partitions_offset': 'partition_offset', 'trace_brush_offset': 'brush_offset'}
    layout = {'record': 'layout', 'pointer_bytes': width, 'va_bytes': 4100, 'va_index_offset': 4096}
    layout.update({key: model[value] for key, value in keys.items()})
    verified = {'record': 'verified', 'live_roles': 3, 'canonical_slots': 12, 'native_jumps': 3,
                'cmd_resets': 2, 'retired_workers': 2, 'pass': True}
    if rows != [layout, verified] or any(type(row[k]) is not type(value) for row, expected in zip(rows, [layout, verified]) for k, value in expected.items()):
        raise RuntimeError('Native thread records differ from independent historical layout/model')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('host-build', 'arm-o0', 'arm-o3', 'output', 'serial'):
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use fresh worktree output')
    output.mkdir(parents=True)
    commands, rows = [], []

    def run(arguments, expected=0, diagnostic=None):
        completed = subprocess.run([str(a) for a in arguments], capture_output=True, text=True, timeout=30)
        commands.append({'args': [str(a) for a in arguments], 'exit': completed.returncode,
                         'stdout': completed.stdout, 'stderr': completed.stderr})
        (output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
        combined = completed.stdout + completed.stderr
        if completed.returncode != expected or 'Thread data test assertion failed' in combined or 'runtime error:' in combined:
            raise RuntimeError('Native thread operation failed; evidence retained')
        if diagnostic and ('Native thread services: ' + diagnostic) not in combined:
            raise RuntimeError('Negative control diagnostic differs')
        return completed.stdout

    adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', args.serial]
    run(adb + ['get-state'])
    cases = [('x86', 4, args.host_build + '/x86', 'native_thread_data.exe'),
             ('x64', 8, args.host_build + '/x64', 'native_thread_data.exe'),
             ('arm64_O0', 8, args.arm_o0, 'bo1_native_thread_data_tests'),
             ('arm64_O3', 8, args.arm_o3, 'bo1_native_thread_data_tests')]
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
                raise RuntimeError('Expected native ARM64 executable')
            remote = '/data/local/tmp/blackopsarm-thread44-' + architecture
            run(adb + ['shell', 'mkdir', '-p', remote])
            run(adb + ['push', binary, remote + '/thread_tests'])
            if run(adb + ['shell', 'sha256sum', remote + '/thread_tests']).split()[0] != sha:
                raise RuntimeError('Pushed thread executable differs')
            run(adb + ['shell', 'chmod', '700', remote + '/thread_tests'])
            prefix = adb + ['shell', remote + '/thread_tests']
        stdout = run(prefix)
        validate(stdout, width)
        for mode, diagnostic in CONTROLS.items():
            expected = 134 if arm else 3221226505 if mode == 'exit_without_retirement' else 3
            run(prefix + [mode], expected=expected, diagnostic=diagnostic)
        rows.append({'architecture': architecture, 'pointer_bytes': width, 'passed': True,
                     'negative_controls': len(CONTROLS), 'binary_sha256': sha})
    (output / 'result.json').write_text(json.dumps({'passed': True, 'runs': rows,
            'independent_layout_baseline': 'f117f5d', 'engine_init_executed': False,
            'scope': 'Actual Com_InitThreadData/Cmd_ResetArgs plus native one-shot TLS, not Com_Error recovery or startup.'}, indent=2) + '\n', encoding='utf-8')
    print('Genuine thread bindings, original layouts and72 exact negative controls pass Windows/HONOR O0/O3')


if __name__ == '__main__':
    main()
