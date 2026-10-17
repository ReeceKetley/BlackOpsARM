"""Unattended map sweep on the tablet: start each Zombies map through the
launcher's debug auto-start hook, let it run (god mode), and collect errors,
crashes, missing assets, frame statistics and screenshots per map.

Usage: python -I tools/port/run_map_sweep.py --out DIR [--seconds 150] [--maps zombie_theater ...]
"""
import argparse, json, pathlib, re, subprocess, sys, time

ADB = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', 'AKWJ9X5116G04618']
PKG = 'com.blackopsarm.dev'
MAPS = ['zombie_theater', 'zombie_pentagon', 'zombie_cosmodrome', 'zombie_coast', 'zombie_temple', 'zombie_moon',
        'zombie_cod5_prototype', 'zombie_cod5_asylum', 'zombie_cod5_sumpf', 'zombie_cod5_factory']
CONSOLE = 'files/user/main/console_mp.log'
ENTRY = 'files/user/native-entry.log'


def adb(*args, timeout=60, check=False):
    return subprocess.run(ADB + list(args), capture_output=True, timeout=timeout, check=check)


def run_as(*args, timeout=60):
    return adb('shell', 'run-as', PKG, *args, timeout=timeout).stdout


def size_of(path):
    out = run_as('stat', '-c', '%s', path).decode(errors='replace').strip()
    return int(out) if out.isdigit() else 0


def tail_from(path, offset):
    return run_as('tail', '-c', '+%d' % (offset + 1), path, timeout=120)


def engine_pid():
    return adb('shell', 'pidof', PKG + ':engine').stdout.decode().strip()


def screenshot(target):
    adb('shell', 'screencap', '-p', '/data/local/tmp/sweep.png')
    adb('pull', '/data/local/tmp/sweep.png', str(target))


CONSOLE_PATTERNS = [
    ('engine_error', re.compile(r'Com_ERROR|ERROR:|\bError\b.*:|^Error ', re.I | re.M)),
    ('script_error', re.compile(r'script runtime error|Script runtime|unknown function|undefined|\^1', re.I)),
    ('missing', re.compile(r"couldn't (find|load|open)|could not (find|load|open)|missing|not found|failed to load", re.I)),
    ('warning', re.compile(r'WARNING', re.I)),
]
LOGCAT_PATTERNS = [
    ('fatal', re.compile(r'Fatal signal|F DEBUG|F libc|Abort message|FATAL|Com_ERROR on parallel|abort\(\)', re.I)),
    ('native_failure', re.compile(r'native_[a-z_]*_failed|query_issue_failed|failed=[1-9]|failed: ', re.I)),
    ('vulkan_error', re.compile(r'VkResult|VK_ERROR|device lost', re.I)),
]


def classify(text, patterns):
    hits = {}
    for line in text.splitlines():
        for name, pattern in patterns:
            if pattern.search(line):
                hits.setdefault(name, []).append(line.strip()[:300])
                break
    return hits


def normalise(line):
    line = re.sub(r'0x[0-9a-fA-F]+|[0-9]+', 'N', line.strip())
    return line[:200]


def unique_messages(text):
    # Distinct problem-looking lines with numbers folded away, with counts.
    counts = {}
    for line in text.splitlines():
        if re.search(r'error|could not|couldn.t|missing|not found|failed|unknown|invalid|assert|redundant|exception|warning', line, re.I):
            key = normalise(line)
            counts[key] = counts.get(key, 0) + 1
    return counts


def cadence(logcat_text):
    fps = [float(m.group(1)) for m in re.finditer(r'native_frame_cadence .*?fps=([\d.]+)', logcat_text)]
    worst = [float(m.group(1)) for m in re.finditer(r'native_frame_cadence .*?max_ms=([\d.]+)', logcat_text)]
    spikes = len(re.findall(r'native_frame_spike', logcat_text))
    if not fps:
        return {'windows': 0, 'spike_frames': spikes}
    gameplay = fps[len(fps) // 4:]  # skip the load/spawn part
    return {'windows': len(fps), 'avg_fps': round(sum(gameplay) / len(gameplay), 1), 'min_window_fps': round(min(gameplay), 1),
            'worst_frame_ms': round(max(worst), 1) if worst else None, 'spike_frames': spikes}


def sweep_map(name, out, seconds):
    d = out / name
    d.mkdir(parents=True, exist_ok=True)
    adb('shell', 'am', 'force-stop', PKG)
    time.sleep(2)
    console_start, entry_start = size_of(CONSOLE), size_of(ENTRY)
    adb('logcat', '-c')
    logfile = open(d / 'logcat.txt', 'wb')
    cat = subprocess.Popen(ADB + ['logcat', '-v', 'time', 'BO1NativeVulkan:I', 'BlackOpsARM:I', 'SDL:I', 'DEBUG:I',
                                  'libc:F', 'AndroidRuntime:E', 'ActivityManager:I', '*:S'], stdout=logfile, stderr=subprocess.STDOUT)
    started = time.time()
    adb('shell', 'am', 'start', '-n', PKG + '/.LauncherActivity', '--es', 'autoMap', name)
    result = {'map': name, 'died': False, 'died_after_s': None}
    pid_seen = None
    shots = {int(seconds * .55): 'early', int(seconds * .95): 'late'}
    while time.time() - started < seconds:
        time.sleep(5)
        pid = engine_pid()
        if pid:
            pid_seen = pid
        elif pid_seen:
            result['died'] = True
            result['died_after_s'] = round(time.time() - started)
            break
        elapsed = int(time.time() - started)
        for at, label in list(shots.items()):
            if elapsed >= at:
                screenshot(d / f'{label}.png')
                del shots[at]
    result['engine_started'] = bool(pid_seen)
    if not result['died'] and pid_seen:
        screenshot(d / 'final.png')
    cat.terminate()
    logfile.close()
    console = tail_from(CONSOLE, console_start).decode(errors='replace')
    entry = tail_from(ENTRY, entry_start).decode(errors='replace')
    (d / 'console.txt').write_text(console, encoding='utf-8')
    (d / 'entry.txt').write_text(entry, encoding='utf-8')
    crash = adb('logcat', '-d', '-b', 'crash').stdout.decode(errors='replace')
    if pid_seen and pid_seen in crash:
        (d / 'crash.txt').write_text(crash, encoding='utf-8')
        result['crash_buffer'] = True
    logcat_text = (d / 'logcat.txt').read_text(encoding='utf-8', errors='replace')
    result['console'] = {k: len(v) for k, v in classify(console, CONSOLE_PATTERNS).items()}
    result['console_examples'] = {k: v[:6] for k, v in classify(console, CONSOLE_PATTERNS).items()}
    result['entry_fatal'] = [l for l in entry.splitlines() if re.search(r'fatal|Com_ERROR|ERROR', l, re.I)][:8]
    result['logcat'] = {k: len(v) for k, v in classify(logcat_text, LOGCAT_PATTERNS).items()}
    result['logcat_examples'] = {k: v[:6] for k, v in classify(logcat_text, LOGCAT_PATTERNS).items()}
    result['map_loaded'] = bool(re.search(r"(Loading|Loaded) fastfile '%s'" % name, console + entry)) or name in console
    result['frames'] = cadence(logcat_text)
    result['messages'] = unique_messages(console)
    (d / 'result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', required=True)
    parser.add_argument('--seconds', type=int, default=150)
    parser.add_argument('--maps', nargs='*', default=MAPS)
    a = parser.parse_args()
    out = pathlib.Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    adb('shell', 'svc', 'power', 'stayon', 'usb')
    results = []
    try:
        for name in a.maps:
            print(time.strftime('%H:%M:%S'), 'map', name, flush=True)
            r = sweep_map(name, out, a.seconds)
            results.append(r)
            print('   ', json.dumps({k: r[k] for k in ('engine_started', 'died', 'died_after_s', 'map_loaded', 'console', 'logcat', 'frames')}), flush=True)
    finally:
        adb('shell', 'am', 'force-stop', PKG)
        adb('shell', 'svc', 'power', 'stayon', 'false')
        (out / 'summary.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
        if results:
            base = set(results[0]['messages']) if results[0]['map'] == 'zombie_theater' else set()
            lines = ['# Map sweep', '', 'Messages below are ones Kino (baseline) did not print.', '']
            for r in results:
                new = sorted((k, v) for k, v in r['messages'].items() if k not in base)
                lines += ['## ' + r['map'], '',
                          'started=%s died=%s after=%ss loaded=%s frames=%s' % (r['engine_started'], r['died'], r['died_after_s'], r['map_loaded'], json.dumps(r['frames'])),
                          'logcat=%s' % json.dumps(r['logcat']), '']
                if r['map'] != 'zombie_theater':
                    lines += ['New messages (%d distinct):' % len(new)] + ['- x%d %s' % (v, k) for k, v in sorted(new, key=lambda kv: -kv[1])[:40]] + ['']
            (out / 'report.md').write_text(chr(10).join(lines), encoding='utf-8')
    print('done', flush=True)


if __name__ == '__main__':
    main()
