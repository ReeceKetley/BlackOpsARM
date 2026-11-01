"""Inspect loaded map constraint definitions in isolated native engine profiles.

This is a save-owner definition census, not a gameplay or playable-resume test.
The current engine and matching providers must already be staged at --remote.
Each engine exits through its own /quit; the observer never kills or restarts it.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess
import time

MAPS = ['zombie_theater', 'zombie_pentagon', 'zombie_cosmodrome', 'zombie_coast',
        'zombie_temple', 'zombie_moon', 'zombie_cod5_prototype',
        'zombie_cod5_asylum', 'zombie_cod5_sumpf', 'zombie_cod5_factory']
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--out', type=Path, required=True)
p.add_argument('--remote', required=True)
p.add_argument('--serial', default='AKWJ9X5116G04618')
p.add_argument('--maps', nargs='+', choices=MAPS, default=MAPS)
a = p.parse_args()
a.out.mkdir(parents=True, exist_ok=False)
adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', a.serial]
game = '/storage/emulated/0/Android/data/com.blackopsarm.dev/files/game'
identity = subprocess.check_output(adb+['shell', 'sha256sum', a.remote+'/engine'], text=True).split()[0]
assert re.fullmatch('[0-9a-f]{64}', identity)
results = []
for name in a.maps:
    folder = a.out/name
    folder.mkdir()
    profile = a.remote+'/profiles/'+name
    subprocess.run(adb+['shell', 'mkdir', '-p', profile], check=True, capture_output=True)
    command = 'cd '+shlex.quote(a.remote)+' && '+shlex.join([
        'env', 'LD_LIBRARY_PATH='+a.remote, 'BO1_ENGINE_USER_ROOT='+profile,
        'BO1_TRACE_STARTUP=0', 'BO1_TRACE_POSTINIT=0', 'BO1_ENGINE_WATCHDOG_SECONDS=120',
        'UBSAN_OPTIONS=print_stacktrace=1', './engine', game])
    result = {'map': name, 'engine_sha256': identity, 'profile': profile, 'command': command,
              'scope': 'loaded map definitions only; native graph support and gameplay not assessed',
              'test_aids': 'headless test client, god enabled, fighting and resupply disabled', 'status': 'running'}
    print(name, 'starting', flush=True)
    with (folder/'entry.log').open('xb') as output:
        process = subprocess.Popen(adb+['shell', '-T', command], stdin=subprocess.PIPE,
                                   stdout=output, stderr=subprocess.STDOUT)
        result['host_pid'] = process.pid
        started = time.monotonic()
        sent = queried = quit_sent = False
        while process.poll() is None:
            text = (folder/'entry.log').read_text(encoding='utf-8', errors='replace')
            if not sent and 'ENGINE ENTRY actual Com_Init returned' in text:
                commands = ['set developer 1', 'set developer_script 0', 'set QuitOnError 1',
                            'set bo1_testclient 1', 'set bo1_testclient_fight 0',
                            'set bo1_testclient_god 1', 'set bo1_testclient_resupply 0',
                            'set bo1_autoquit 120000', 'devmap '+name]
                process.stdin.write(('\n'.join(commands)+'\n').encode())
                process.stdin.flush()
                result['commands'] = ['/'+x for x in commands]
                sent = True
            if not queried and 'BO1_SERVER_SPAWN_COMPLETE' in text:
                process.stdin.write(b'bo1_resume_map_owners definitions\n')
                process.stdin.flush()
                queried = True
            if not quit_sent and 'Resume map definitions END: map '+name+' ' in text:
                process.stdin.write(b'quit\n')
                process.stdin.flush()
                quit_sent = True
            result.update(elapsed_seconds=round(time.monotonic()-started, 2),
                          query_sent=queried, quit_sent=quit_sent)
            (folder/'running.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
            time.sleep(.5)
        result['exit'] = process.wait()
        process.stdin.close()
    text = (folder/'entry.log').read_text(encoding='utf-8', errors='replace')
    guard = subprocess.run(adb+['shell', 'cat', profile+'/.blackopsarm-instance'], capture_output=True, text=True)
    result['guard'] = guard.stdout.strip()
    result['status'] = 'terminal'
    result['clean'] = result['exit'] == 0 and result['guard'].startswith('clean pid=')
    rows = [dict(zip(('index', 'kind', 'firstAttach', 'firstTarget', 'secondAttach', 'secondTarget',
                     'ropeSlot', 'handlePresent', 'flags', 'timeout'), map(int, m))) for m in re.findall(
        r'Resume map definition: index (\d+) kind (-?\d+) attach (-?\d+):(-?\d+)/(-?\d+):(-?\d+) ropeSlot (-?\d+) handlePresent (\d+) flags (-?\d+) timeout (-?\d+)', text)]
    end = re.findall(r'Resume map definitions END: map '+name+r' count (\d+) unknown (\d+)', text)
    result['definitions'] = rows
    result['counts'] = {str(kind): sum(x['kind'] == kind for x in rows) for kind in range(10)}
    result['complete_inventory'] = len(end) == 1 and [x['index'] for x in rows] == list(range(int(end[0][0])))
    result['summary_lines'] = [line for line in text.splitlines() if line.startswith('Resume map definitions:')]
    result['log_sha256'] = hashlib.sha256((folder/'entry.log').read_bytes()).hexdigest()
    (folder/'result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    results.append(result)
    (a.out/'summary.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
    print(name, 'exit', result['exit'], 'clean', result['clean'], 'complete', result['complete_inventory'],
          'counts', result['counts'], flush=True)
    assert result['clean'] and result['complete_inventory'], 'Inspect the same process/retained log; no automatic restart'
