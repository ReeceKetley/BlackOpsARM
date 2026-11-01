"""Run genuine ARM64 headless maps with isolated profiles and retained evidence.

Stage the current console engine/providers first. Each result records commands,
actual lifecycle/round/plan observations and failure excerpts; survival alone is
never classified as a gameplay pass. Reuse an output directory only to resume
after completed cases: existing case directories are never overwritten.
"""
import argparse
import json
import pathlib
import re
import shlex
import subprocess
import time

MAPS = ['zombie_theater', 'zombie_pentagon', 'zombie_cosmodrome', 'zombie_coast',
        'zombie_temple', 'zombie_moon', 'zombie_cod5_prototype',
        'zombie_cod5_asylum', 'zombie_cod5_sumpf', 'zombie_cod5_factory']


def write_json(path, value, required=True):
    temporary = path.with_name(path.name + '.tmp')
    temporary.write_text(json.dumps(value, indent=2), encoding='utf-8')
    for attempt in range(20):
        try:
            temporary.replace(path)
            return True
        except PermissionError:
            # Windows readers can temporarily deny delete-sharing. A progress
            # checkpoint must never orphan an otherwise healthy device run.
            if attempt < 19:
                time.sleep(0.05)
    if required:
        raise RuntimeError('Could not persist result: ' + str(path))
    print('Checkpoint temporarily locked: ' + str(path), flush=True)
    return False


def observations(text):
    lines = text.splitlines()
    rounds = [int(m.group(1)) for m in re.finditer(r'bo1_feature:.*level notify start_of_round round (\d+)', text)]
    epochs = []
    for line in lines:
        if 'BO1_GAME_INIT_COMPLETE' in line:
            epochs.append({'init': line, 'player_active': False, 'highest_round_started': 0,
                           'plan_complete': False, 'plan_failed': False})
        if epochs and re.search(r'bo1_testclient:.*state 5 connected 1.*all_players_are_connected 1', line):
            epochs[-1]['player_active'] = True
        match = re.search(r'bo1_feature:.*level notify start_of_round round (\d+)', line)
        if epochs and match:
            epochs[-1]['highest_round_started'] = max(epochs[-1]['highest_round_started'], int(match.group(1)))
        if epochs and re.search(r'bo1_plan: time \d+ complete,', line):
            epochs[-1]['plan_complete'] = True
        if epochs and re.search(r'bo1_plan:.*(?:end(?: recovery)? FAIL|unknown step)', line):
            epochs[-1]['plan_failed'] = True
    return {
        'init_returned': 'ENGINE ENTRY actual Com_Init returned' in text,
        'server_spawned': 'BO1_SERVER_SPAWN_COMPLETE' in text,
        'map_epochs': epochs,
        'round_events': [s for s in lines if 'bo1_feature:' in s and re.search(
            r'notify (start_of_round|end_of_round|between_round_over|dog_round|thief_round)', s)],
        'life_events': [s for s in lines if 'bo1_feature:' in s and re.search(
            r'notify (fake_death|pre_end_game|end_game|player_downed|player_revived|_zombie_game_over)', s)],
        'plan_results': [s for s in lines if 'bo1_plan:' in s and re.search(r'end(?: recovery)? (PASS|FAIL)|complete', s)],
        'actor_summaries': [s for s in lines if re.search(r'bo1_actor|bo1_fight', s)][-12:],
        'highest_round_started': max(rounds, default=0),
        'actor_deaths': len(re.findall(r'bo1_effect:.*actor \d+ death mod ', text)),
        'player_active': bool(re.search(r'bo1_testclient:.*state 5 connected 1.*all_players_are_connected 1', text)),
        'fatal': bool(re.search(r'runtime error:|script runtime error|Native engine fatal|ASSERT |Fatal signal|Abort message', text, re.I)),
        'failures': [s for s in lines if re.search(
            r'runtime error:|script runtime error|Com_ERROR|ASSERT|Fatal signal|Abort message|ERROR:|Sys_Error', s, re.I)][:80],
        'autoquit': 'bo1_autoquit:' in text,
    }


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--out', required=True, type=pathlib.Path)
    p.add_argument('--serial', default='AKWJ9X5116G04618')
    p.add_argument('--remote', default='/data/local/tmp/bo1-map-audit-20261018')
    p.add_argument('--binary', default='engine')
    p.add_argument('--game-root', default='/storage/emulated/0/Android/data/com.blackopsarm.dev/files/game')
    p.add_argument('--maps', nargs='+', choices=MAPS, default=MAPS)
    p.add_argument('--seconds', type=int, default=240, help='Autoquit milliseconds are measured from actual init return')
    p.add_argument('--min-round', type=int, default=3, help='Required natural round reached for this bounded baseline')
    p.add_argument('--required-map-inits', type=int, default=1, help='Require this many actual initialized map epochs and active players')
    p.add_argument('--command', action='append', default=[], help='Seconds after init return:/console command')
    p.add_argument('--plan', help='Explicit feature plan; default is a baseline observation plan (Moon includes travel/PES)')
    p.add_argument('--set', action='append', default=[], help='Additional name=value test settings')
    a = p.parse_args()
    if not 60 <= a.seconds <= 3600:
        p.error('seconds must be 60..3600')
    if a.plan is not None and any(c in a.plan for c in '\n\r\0"'):
        p.error('plan must be one command argument')
    if not re.fullmatch(r'[A-Za-z0-9_.-]+', a.binary) or a.binary in ('.', '..'):
        p.error('binary must be one staged executable name')
    if a.required_map_inits < 1:
        p.error('required-map-inits must be positive')
    schedule = []
    for item in a.command:
        delay, sep, command = item.partition(':')
        try:
            delay = float(delay)
        except ValueError:
            p.error('scheduled command requires seconds:/command')
        if not sep or not 0 <= delay < a.seconds or not command.startswith('/') or any(c in command for c in '\n\r\0'):
            p.error('scheduled command must be a slash command within the run interval')
        schedule.append((delay, command))
    schedule.sort()
    a.out.mkdir(parents=True, exist_ok=True)
    adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', a.serial]
    identity = subprocess.run(adb + ['shell', shlex.join(['sha256sum', a.remote + '/' + a.binary])],
                              capture_output=True, text=True, check=True)
    engine_sha256 = identity.stdout.split()[0]
    if not re.fullmatch(r'[0-9a-f]{64}', engine_sha256):
        raise RuntimeError('Could not verify staged engine identity')
    settings = {'developer': '1', 'developer_script': '0', 'QuitOnError': '1',
                'bo1_testclient': '1', 'bo1_testclient_fight': '1', 'bo1_testclient_god': '1',
                'bo1_testclient_resupply': '1',
                'bo1_autoquit': str(a.seconds * 1000)}
    for item in a.set:
        key, sep, value = item.partition('=')
        if not sep or not re.fullmatch(r'[A-Za-z0-9_]+', key) or any(c in value for c in '\n\r\0"'):
            p.error('Each setting requires a valid name=value')
        settings[key] = value
    results = []
    for name in a.maps:
        settings['bo1_testclient_plan'] = a.plan if a.plan is not None else (
            'm_to_moon m_pes m_pes_on observe110' if name == 'zombie_moon' else 'observe110')
        d = a.out / name
        if (d / 'result.json').exists():
            previous = json.loads((d / 'result.json').read_text(encoding='utf-8'))
            if previous.get('engine_sha256') != engine_sha256:
                p.error('Existing result has a different/unverified engine; use a fresh output directory')
            print(name, 'already recorded; preserving evidence', flush=True)
            results.append(previous)
            continue
        d.mkdir(exist_ok=False)
        user = a.remote + '/profiles/' + a.out.name + '-' + name
        subprocess.run(adb + ['shell', 'mkdir', '-p', user], check=True, capture_output=True)
        commands = ['/set ' + k + ' "' + v + '"' for k, v in settings.items()]
        commands += ['/devmap ' + name]
        environment = ['env', 'LD_LIBRARY_PATH=' + a.remote,
                       'BO1_ENGINE_USER_ROOT=' + user, 'BO1_TRACE_STARTUP=0',
                       'BO1_TRACE_POSTINIT=0', 'BO1_ENGINE_WATCHDOG_SECONDS=120',
                       'UBSAN_OPTIONS=print_stacktrace=1']
        remote = 'cd ' + shlex.quote(a.remote) + ' && ' + shlex.join(
            ['timeout', str(a.seconds + 180)] + environment + ['./' + a.binary, a.game_root])
        remote += '; audit_status=$?; printf "%s\\n" "$audit_status" > ' + shlex.quote(user + '/exit-status') + '; exit "$audit_status"'
        start = time.monotonic()
        result = {'map': name, 'profile': user, 'commands': commands, 'remote': remote,
                  'engine_sha256': engine_sha256,
                  'test_aids': {k: v for k, v in settings.items() if k.startswith('bo1_testclient')},
                  'scheduled_commands': schedule, 'sent_scheduled_commands': [], 'status': 'running'}
        write_json(d / 'running.json', result, required=False)
        print(time.strftime('%H:%M:%S'), name, 'starting', flush=True)
        with (d / 'entry.log').open('xb') as log:
            process = subprocess.Popen(adb + ['shell', '-T', remote], stdin=subprocess.PIPE,
                                       stdout=log, stderr=subprocess.STDOUT)
            sent = False
            schedule_index = 0
            while process.poll() is None:
                time.sleep(1)
                text = (d / 'entry.log').read_text(encoding='utf-8', errors='replace')
                if not sent and 'ENGINE ENTRY actual Com_Init returned' in text:
                    # The CLI stdin transport accepts engine command tokens;
                    # the rendered slash FIFO strips '/' itself. Keep slash
                    # notation in evidence and adapt only this transport.
                    process.stdin.write(('\n'.join(c[1:] for c in commands) + '\n').encode())
                    process.stdin.flush()
                    result['commands_sent_seconds'] = round(time.monotonic() - start, 1)
                    sent = True
                if sent:
                    after_init = time.monotonic() - start - result['commands_sent_seconds']
                    while schedule_index < len(schedule) and after_init >= schedule[schedule_index][0]:
                        delay, command = schedule[schedule_index]
                        try:
                            process.stdin.write((command[1:] + '\n').encode())
                            process.stdin.flush()
                        except (BrokenPipeError, OSError):
                            result['schedule_write_failed'] = True
                            schedule_index = len(schedule)
                            break
                        result['sent_scheduled_commands'].append({'command': command, 'scheduled_seconds': delay,
                                                                  'sent_after_init_seconds': round(after_init, 1)})
                        schedule_index += 1
                result.update(elapsed_seconds=round(time.monotonic() - start, 1),
                              host_pid=process.pid, observed=observations(text))
                write_json(d / 'running.json', result, required=False)
            result['exit_code'] = process.wait()
            process.stdin.close()
        text = (d / 'entry.log').read_text(encoding='utf-8', errors='replace')
        result.update(status='terminal', elapsed_seconds=round(time.monotonic() - start, 1), observed=observations(text))
        guard = subprocess.run(adb + ['shell', 'cat', user + '/.blackopsarm-instance'], capture_output=True, text=True)
        result['guard'] = guard.stdout.strip()
        status = subprocess.run(adb + ['shell', 'cat', user + '/exit-status'], capture_output=True, text=True)
        result['remote_exit_status'] = int(status.stdout.strip()) if status.stdout.strip().lstrip('-').isdigit() else None
        result['exit_status_agrees'] = result['remote_exit_status'] == result['exit_code']
        result['abnormal_exit'] = result['exit_code'] != 0 or result['remote_exit_status'] != 0
        if result['abnormal_exit']:
            crash = subprocess.run(adb + ['logcat', '-d', '-b', 'crash'], capture_output=True, text=True).stdout
            pid = re.search(r'pid=(\d+)', result['guard'])
            if pid and re.search(r'pid: ' + pid[1] + r'\b|pid ' + pid[1] + r'\b', crash):
                (d / 'crash-buffer.txt').write_text(crash, encoding='utf-8')
                result['crash_buffer_retained'] = True
        result['clean_shutdown'] = (result['exit_code'] == 0 and result['remote_exit_status'] == 0
                                    and result['guard'].startswith('clean pid='))
        result['lifecycle_gate_met'] = (result['clean_shutdown'] and result['observed']['server_spawned']
                                        and result['observed']['player_active'] and not result['observed']['fatal'])
        epochs = result['observed']['map_epochs']
        result['map_epoch_gate_met'] = len(epochs) >= a.required_map_inits and all(e['player_active'] for e in epochs)
        result['required_map_inits'] = a.required_map_inits
        result['round_gate_met'] = bool(epochs) and all(e['highest_round_started'] >= a.min_round for e in epochs)
        result['required_natural_round'] = a.min_round
        result['plan_gate_met'] = (bool(epochs) and all(e['plan_complete'] and not e['plan_failed'] for e in epochs)
            and not any('FAIL' in s for s in result['observed']['plan_results'])
            and 'bo1_plan: unknown step' not in text)
        result['schedule_gate_met'] = len(result['sent_scheduled_commands']) == len(schedule)
        write_json(d / 'result.json', result)
        results.append(result)
        print(name, json.dumps(result), flush=True)
    write_json(a.out / 'summary.json', results)
    gates = ('lifecycle_gate_met', 'round_gate_met', 'plan_gate_met', 'map_epoch_gate_met', 'schedule_gate_met')
    return 0 if all(all(r.get(g) for g in gates) for r in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
