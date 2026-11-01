"""Reopen a diagnostic archive in a new engine process without loading a map.

The preceding capture process must have exited. This checks persistent bytes,
not playable restoration. No snapshot is installed and no archive is written.
"""
import argparse
import json
from pathlib import Path
import shlex
import subprocess
import time

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--profile', required=True)
p.add_argument('--out', type=Path, required=True)
p.add_argument('--binary', default='engine-save-archive-v2')
p.add_argument('--serial', default='AKWJ9X5116G04618')
p.add_argument('--remote', default='/data/local/tmp/bo1-map-audit-20261018')
p.add_argument('--game-root', default='/storage/emulated/0/Android/data/com.blackopsarm.dev/files/game')
a = p.parse_args()
if not a.profile.startswith(a.remote + '/profiles/') or any(c in a.binary for c in '/\\\n\r'):
    p.error('use an isolated audit profile and a staged executable name')
adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', a.serial]
ps = subprocess.run(adb + ['shell', 'ps', '-A', '-o', 'PID,ARGS'], capture_output=True, text=True, check=True).stdout
if a.profile in ps:
    raise RuntimeError('The capture profile still has a live process; wait for its terminal result')
a.out.mkdir(parents=True, exist_ok=False)
sha = subprocess.run(adb + ['shell', 'sha256sum', a.remote + '/' + a.binary], capture_output=True, text=True, check=True).stdout.split()[0]
remote = shlex.join(['timeout', '50', 'env', 'LD_LIBRARY_PATH=' + a.remote,
    'BO1_ENGINE_USER_ROOT=' + a.profile, 'BO1_TRACE_STARTUP=0', 'BO1_TRACE_POSTINIT=0',
    a.remote + '/' + a.binary, a.game_root])
start = time.monotonic()
with (a.out / 'entry.log').open('xb') as log:
    process = subprocess.Popen(adb + ['shell', '-T', remote], stdin=subprocess.PIPE, stdout=log, stderr=subprocess.STDOUT)
    inspected = quit_sent = False
    while process.poll() is None:
        time.sleep(0.25)
        text = (a.out / 'entry.log').read_text(encoding='utf-8', errors='replace')
        if not inspected and 'ENGINE ENTRY actual Com_Init returned' in text:
            process.stdin.write(b'bo1_save_archive_inspect\n')
            process.stdin.flush()
            inspected = True
        if inspected and not quit_sent and ('Save archive:' in text or time.monotonic() - start > 35):
            process.stdin.write(b'quit\n')
            process.stdin.flush()
            quit_sent = True
    result = {'profile': a.profile, 'binary': a.binary, 'engine_sha256': sha,
        'exit_code': process.returncode, 'elapsed_seconds': round(time.monotonic() - start, 2),
        'map_loaded': 'BO1_GAME_INIT_COMPLETE' in text,
        'integrity_verified': 'Save archive: generation 2 integrity OK map zombie_theater' in text,
        'playable_restore_tested': False}
    (a.out / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result))
    raise SystemExit(0 if result['exit_code'] == 0 and result['integrity_verified'] and not result['map_loaded'] else 1)
