"""Run real x86/x64 prerequisites in a fresh host builder directory.

Results stay beside x86/x64 binaries for run_script_string_services.py. Existing
compile logs are preserved; any pre-existing runtime evidence causes refusal.
"""
from pathlib import Path
import argparse
import json
import subprocess
import sys
import traceback

sys.dont_write_bytecode = True
from run_script_string_services import ROOT, digest, validate_payload, verify_pe


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host-build', type=Path, required=True)
    parser.add_argument('--output', type=Path, help='Optional; must equal --host-build so references resolve beside binaries')
    args = parser.parse_args()
    build = args.host_build.resolve()
    output = args.output.resolve() if args.output else build
    if build == ROOT or not build.is_relative_to(ROOT) or output != build or not build.is_dir():
        raise RuntimeError('Use an existing host builder directory inside BlackOpsARM; output must be the same directory')
    cases = [('passing', [], None), ('invalid-lock', ['--invalid-lock'], 'invalid lock'),
             ('conflicting-server', ['--conflicting-server'], 'server thread already registered')]
    planned = ['results.json', 'host-invocations.json', 'host-executables.json', 'host-run-error.log']
    planned += [f'{arch}/{case}.{stream}.log' for arch in ['x86', 'x64']
                for case, _, _ in cases for stream in ['stdout', 'stderr']]
    if any((output / name).exists() for name in planned):
        raise RuntimeError('Runtime evidence already exists; build in a fresh directory and preserve earlier results')
    binaries = []
    for arch, pointer_bytes, machine in [('x86', 4, 0x14c), ('x64', 8, 0x8664)]:
        binary = build / arch / 'script_string_services.exe'
        verify_pe(binary, machine)
        binaries.append({'arch': arch, 'pointer_bytes': pointer_bytes, 'machine': machine,
                         'path': str(binary), 'sha256': digest(binary)})
    results, invocations = [], []

    def save(name, value):
        (output / name).write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')

    save('host-executables.json', binaries)
    try:
        for executable in binaries:
            arch = executable['arch']
            for case, parameters, diagnostic in cases:
                argv = [executable['path'], *parameters]
                invocation = {'arch': arch, 'case': case, 'argv': argv, 'cwd': str(output),
                              'timeout_seconds': 60, 'exit': None, 'timed_out': False, 'launch_error': None}
                stdout, stderr = b'', b''
                try:
                    process = subprocess.run(argv, cwd=output, capture_output=True, timeout=60)
                    stdout, stderr = process.stdout, process.stderr
                    invocation['exit'] = process.returncode
                except subprocess.TimeoutExpired as error:
                    stdout, stderr = error.stdout or b'', error.stderr or b''
                    invocation['timed_out'] = True
                except OSError as error:
                    stderr = str(error).encode('utf-8')
                    invocation['launch_error'] = str(error)
                (output / arch / (case + '.stdout.log')).write_bytes(stdout)
                (output / arch / (case + '.stderr.log')).write_bytes(stderr)
                invocations.append(invocation)
                save('host-invocations.json', invocations)
                entry = {'arch': arch, 'case': case, 'exit': invocation['exit'],
                         'stdout': stdout.decode('utf-8', errors='replace'),
                         'stderr': stderr.decode('utf-8', errors='replace')}
                results.append(entry)
                save('results.json', results)
                if invocation['exit'] is None:
                    raise RuntimeError(arch + ' ' + case + ': launch/timeout failure; evidence retained')
                if diagnostic is None:
                    if invocation['exit'] != 0:
                        raise RuntimeError(arch + ': normal prerequisite failed')
                    validate_payload(json.loads(entry['stdout']), executable['pointer_bytes'])
                elif invocation['exit'] != 3 or 'Script pool service failure: ' + diagnostic not in entry['stderr']:
                    raise RuntimeError(arch + ' ' + case + ': expected explicit abort exit3 and exact diagnostic')
        print(json.dumps({'status': 'passed', 'host_build': str(build), 'cases': len(results), 'pool_certified': False}))
    except Exception:
        (output / 'host-run-error.log').write_text(traceback.format_exc(), encoding='utf-8')
        raise


if __name__ == '__main__':
    main()
