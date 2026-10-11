"""Reproduce real script-pool platform prerequisites, independently of SL/MT.

Uses a fresh worktree/device directory; preserves failed invocation output.
The two deliberate native aborts are required results, not successful probes.
"""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import re
import shlex
import struct
import subprocess
import traceback
import uuid

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def validate_payload(payload, pointer_bytes):
    expected = {'schema': 'BlackOpsARM.script-services.v1',
                'pointer_bytes': pointer_bytes, 'atomic_bytes': 4,
                'atomic_updates': 80000, 'locked_updates': 80000,
                'recursive_sections': 2, 'independent_sections': True,
                'contention_pass': True, 'server_identity_pass': True,
                'pool_certified': False}
    for key, value in expected.items():
        if payload.get(key) != value or type(payload.get(key)) is not type(value):
            raise RuntimeError('Unexpected prerequisite result: ' + key)


def verify_pe(path, expected):
    with Path(path).open('rb') as stream:
        first = stream.read(64)
        if len(first) != 64 or first[:2] != b'MZ':
            raise RuntimeError('Invalid Windows reference executable')
        stream.seek(struct.unpack_from('<I', first, 60)[0])
        header = stream.read(6)
    if len(header) != 6 or header[:4] != b'PE\0\0' or struct.unpack_from('<H', header, 4)[0] != expected:
        raise RuntimeError('Wrong Windows reference executable machine')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--android-build', type=Path, required=True)
    parser.add_argument('--host-results', type=Path,
                        default=ROOT / 'build-android-audit/script_services_host_pass2/results.json')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--serial', default='AKWJ9X5116G04618')
    parser.add_argument('--adb', type=Path, default=Path('D:/Android/Sdk/platform-tools/adb.exe'))
    args = parser.parse_args()
    output = args.output.resolve()
    if output == ROOT or not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use a fresh output directory inside BlackOpsARM')
    output.mkdir(parents=True)
    invocations = []
    result = {'schema': 'BlackOpsARM.script-services-device.v1',
              'recorded_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'serial': args.serial, 'status': 'running',
              'scope': 'Real platform atomics, recursive locks and server identity; actual SL/MT pool remains unvalidated'}

    def save(name, value):
        (output / name).write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')

    def run(label, argv, allow_nonzero=False):
        argv = [str(word) for word in argv]
        invocation = {'label': label, 'argv': argv, 'cwd': str(output), 'timeout_seconds': 60,
                      'exit': None, 'timed_out': False, 'launch_error': None}
        stdout, stderr = b'', b''
        try:
            completed = subprocess.run(argv, cwd=output, capture_output=True, timeout=60)
            stdout, stderr = completed.stdout, completed.stderr
            invocation['exit'] = completed.returncode
        except subprocess.TimeoutExpired as error:
            stdout, stderr = error.stdout or b'', error.stderr or b''
            invocation['timed_out'] = True
        except OSError as error:
            stderr = str(error).encode('utf-8')
            invocation['launch_error'] = str(error)
        (output / (label + '.stdout.log')).write_bytes(stdout)
        (output / (label + '.stderr.log')).write_bytes(stderr)
        invocations.append(invocation)
        save('invocations.json', invocations)
        if invocation['exit'] is None or (invocation['exit'] != 0 and not allow_nonzero):
            raise RuntimeError(label + ': failed; full command/output retained')
        return stdout.decode('utf-8', errors='replace'), stderr.decode('utf-8', errors='replace'), invocation['exit']

    adb = [args.adb, '-s', args.serial]

    def shell(label, words):
        return run(label, adb + ['shell', shlex.join([str(word) for word in words])])[0]

    try:
        save('results.json', result)
        native = args.android_build.resolve() / 'bo1_script_string_services_tests'
        with native.open('rb') as stream:
            header = stream.read(20)
        if (len(header) != 20 or header[:6] != b'\x7fELF\x02\x01'
                or struct.unpack_from('<H', header, 18)[0] != 183):
            raise RuntimeError('Probe must be a little-endian ELF64 AArch64 executable')
        result['native_executable'] = {'path': str(native), 'sha256': digest(native), 'elf_machine': 183}
        host_path = args.host_results.resolve()
        host_bytes = host_path.read_bytes()
        host = json.loads(host_bytes)
        if len(host) != 6:
            raise RuntimeError('Expected all six Windows reference invocations')
        references = []
        for arch, pointer_bytes, machine in [('x86', 4, 0x14c), ('x64', 8, 0x8664)]:
            binary = host_path.parent / arch / 'script_string_services.exe'
            verify_pe(binary, machine)
            for case in ['passing', 'invalid-lock', 'conflicting-server']:
                matches = [item for item in host if item.get('arch') == arch and item.get('case') == case]
                if len(matches) != 1:
                    raise RuntimeError('Missing/duplicate host prerequisite case')
                item = matches[0]
                if case == 'passing':
                    if item['exit'] != 0:
                        raise RuntimeError('Host passing prerequisite failed')
                    validate_payload(json.loads(item['stdout']), pointer_bytes)
                else:
                    diagnostic = 'invalid lock' if case == 'invalid-lock' else 'server thread already registered'
                    if item['exit'] != 3 or 'Script pool service failure: ' + diagnostic not in item['stderr']:
                        raise RuntimeError('Host fatal prerequisite lacks exact expected abort evidence')
            references.append({'arch': arch, 'machine': machine, 'path': str(binary), 'sha256': digest(binary)})
        (output / 'host-results.snapshot.json').write_bytes(host_bytes)
        result['host_reference'] = {'path': str(host_path), 'sha256': digest(host_path), 'executables': references}
        state = run('device-state', adb + ['get-state'])[0].strip()
        if state != 'device':
            raise RuntimeError('Selected Android device is not authorized/online')
        model = shell('device-model', ['getprop', 'ro.product.model']).strip()
        abi = shell('device-abi', ['getprop', 'ro.product.cpu.abilist']).strip()
        if 'arm64-v8a' not in abi.split(','):
            raise RuntimeError('Selected device does not advertise ARM64')
        result['device'] = {'model': model, 'abi_list': abi,
                            'api': shell('device-api', ['getprop', 'ro.build.version.sdk']).strip()}
        folder = '/data/local/tmp/blackopsarm-script-services-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S%f') + '-' + uuid.uuid4().hex[:8]
        result['device_directory'] = folder
        save('results.json', result)
        shell('device-create', ['mkdir', folder])
        remote = folder + '/probe'
        run('device-push', adb + ['push', native, remote])
        shell('device-chmod', ['chmod', '700', remote])
        remote_sha = shell('device-executable-sha', ['sha256sum', remote]).split()[0].lower()
        if remote_sha != result['native_executable']['sha256']:
            raise RuntimeError('Pushed executable SHA differs from validated local ELF')
        result['cases'] = []
        for case, arguments, diagnostic in [('passing', [], None), ('invalid-lock', ['--invalid-lock'], 'invalid lock'),
                                            ('conflicting-server', ['--conflicting-server'], 'server thread already registered')]:
            command = shlex.join([remote, *arguments]) + '; bo1_services_rc=$?; printf \'\\nBO1_SCRIPT_SERVICES_EXIT:%s\\n\' "$bo1_services_rc"; exit "$bo1_services_rc"'
            stdout, stderr, code = run(case, adb + ['shell', command], allow_nonzero=True)
            markers = re.findall(r'^BO1_SCRIPT_SERVICES_EXIT:(\d+)\s*$', stdout, re.MULTILINE)
            if len(markers) != 1 or int(markers[0]) != code:
                raise RuntimeError(case + ': no confirmed matching native exit; transport/shell failure is not test evidence')
            entry = {'case': case, 'native_exit': code, 'diagnostic_expected': diagnostic}
            if diagnostic is None:
                payloads = [json.loads(line) for line in stdout.splitlines() if line.startswith('{')]
                if code != 0 or len(payloads) != 1:
                    raise RuntimeError('Native prerequisite normal invocation failed')
                validate_payload(payloads[0], 8)
                entry['payload'] = payloads[0]
            elif code != 134 or 'Script pool service failure: ' + diagnostic not in stdout + stderr:
                raise RuntimeError(case + ': expected explicit native SIGABRT/134 diagnostic')
            result['cases'].append(entry)
            save('results.json', result)
        result['status'] = 'passed'
        save('results.json', result)
        print(json.dumps({'status': 'passed', 'output': str(output), 'device_directory': folder, 'pool_certified': False}))
    except Exception as error:
        result['status'] = 'failed'
        result['error'] = str(error)
        save('results.json', result)
        (output / 'runner-error.log').write_text(traceback.format_exc(), encoding='utf-8')
        raise


if __name__ == '__main__':
    main()
