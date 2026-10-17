"""Deliver slash-prefixed test commands to an already running automatic Kino launch.

The engine consumes its private FIFO through the existing restricted console
dispatcher. Delivery is not execution: verify queried dvars or resulting game
state after the following simulation frame, before recording a benchmark.
"""
import argparse
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--adb', default='D:/Android/Sdk/platform-tools/adb.exe')
    parser.add_argument('--serial', default='AKWJ9X5116G04618')
    parser.add_argument('--package', default='com.blackopsarm.dev')
    parser.add_argument('--pid', required=True, type=int)
    parser.add_argument('commands', nargs='+')
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9_.:-]+', args.serial):
        parser.error('invalid device serial')
    if not re.fullmatch(r'[A-Za-z][A-Za-z0-9_]*(?:\.[A-Za-z][A-Za-z0-9_]*)+', args.package):
        parser.error('invalid package name')
    if args.pid <= 0:
        parser.error('a positive observed engine PID is required')
    for command in args.commands:
        if not command.startswith('/') or len(command) < 2 or any(ord(c) < 32 for c in command):
            parser.error('each command must start with / and contain no control characters')
    payload = ('\n'.join(args.commands) + '\n').encode('utf-8')
    if len(payload) > 4096 or any(len(c.encode('utf-8')) > 1023 for c in args.commands):
        parser.error('batch exceeds the parser frame budget or line limit')
    adb = [args.adb, '-s', args.serial]

    def read(*command):
        return subprocess.check_output(adb + ['shell', '-T', *command], timeout=6).decode().strip()

    def check_pid():
        if read('pidof', args.package + ':engine') != str(args.pid):
            raise RuntimeError('observed engine PID changed; commands were not approved for this process')

    check_pid()
    uid = read('run-as', args.package, 'id', '-u')
    metadata = read('run-as', args.package, 'stat', '-c', '%F:%a:%h:%u', 'files/user/native-console.fifo')
    if metadata != 'FIFO (named pipe):600:1:' + uid:
        raise RuntimeError('private FIFO type, mode, link count or owner mismatch: ' + metadata)
    # Android's shipped dd lacks O_NONBLOCK. A system timeout bounds both FIFO
    # open and write if the reader exits; never restart the engine during this
    # operation. Raw stdin carries commands without shell quoting/interpolation.
    result = subprocess.run(adb + ['shell', '-T', 'run-as', args.package,
        '/system/bin/timeout', '2', '/system/bin/tee', 'files/user/native-console.fifo'],
        input=payload, capture_output=True, timeout=6)
    if result.returncode or result.stdout.replace(b'\r\n', b'\n') != payload:
        raise RuntimeError('FIFO delivery failed; a partial batch may have reached the engine: '
            + repr((result.returncode, result.stdout, result.stderr)))
    check_pid()
    print(f'Delivered {len(args.commands)} slash commands to engine PID {args.pid}; verify execution before measuring.')


if __name__ == '__main__':
    main()
