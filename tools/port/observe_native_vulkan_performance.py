"""Observe one existing engine PID; preserve logs, screenshots and frame samples.

Never launches/restarts the app. Screenshot evidence is valid only while the
same PID remains alive. CPU/GPU phase timings are not reported as whole-frame FPS.
"""
import argparse
from datetime import datetime
import json
from pathlib import Path
import re
import statistics
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument('--sdk', type=int, required=True)
parser.add_argument('--suffix', required=True)
parser.add_argument('--seconds', type=int, default=120)
parser.add_argument('--min-room-draws', type=int, default=900)
parser.add_argument('--serial', default='AKWJ9X5116G04618')
args = parser.parse_args()
if not re.fullmatch(r'[a-zA-Z0-9_-]+', args.suffix) or args.seconds <= 0:
    parser.error('Use a simple unique suffix and a positive duration')
root = Path(__file__).resolve().parents[2]
out = root / 'build-android-audit/native_registry_closure'
stem = out / f'sdk{args.sdk}-{args.suffix}'
assert not stem.with_suffix('.json').exists(), 'Preserve previous evidence'
assert not Path(str(stem) + '-logcat.txt').exists(), 'Preserve previous evidence'
adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', args.serial]
marker = b'ENGINE ENTRY SDK stdout/stderr captured'
expected = int((out / f'sdk{args.sdk}-previous-markers.txt').read_text()) + 1


def call(*command, check=True):
    result = subprocess.run(adb + list(command), capture_output=True)
    if check and result.returncode:
        raise RuntimeError(result.stderr.decode(errors='replace'))
    return result.stdout


def current_pid():
    return call('shell', 'pidof', 'com.blackopsarm.dev:engine', check=False).decode().strip()


entry_offset = None


def entry():
    global entry_offset
    if entry_offset is None:
        # Validate all historical markers once, before the observation clock.
        raw = call('exec-out', 'run-as', 'com.blackopsarm.dev', 'cat', 'files/user/native-entry.log')
        if raw.count(marker) != expected:
            raise RuntimeError('Launch marker changed; refusing to mix runs')
        entry_offset = raw.rfind(marker)
        return raw[entry_offset:]
    # Avoid repeatedly transferring the accumulated log (83 MB at SDK619).
    # A fixed byte offset retains this entire launch, including any later fatal.
    raw = call('exec-out', 'run-as', 'com.blackopsarm.dev', 'tail', '-c',
               '+' + str(entry_offset + 1), 'files/user/native-entry.log')
    if not raw.startswith(marker) or raw.count(marker) != 1:
        raise RuntimeError('Launch log rotated or marker changed; refusing to mix runs')
    return raw


pid = current_pid()
assert pid and pid.isdigit(), 'An existing engine PID is required'
entry()
initial_log = call('logcat', '-d', '--pid=' + pid, '-v', 'threadtime', '-s', 'BO1NativeVulkan:I', '*:S')
started = time.monotonic()
captures = []
reason = 'duration_complete'
Path(str(stem) + '-thermal-start.txt').write_bytes(call('shell', 'dumpsys', 'thermalservice'))
# Poll and flush so evidence is visible during observation. Exclude old ring
# entries; retain each new timestamped line once before rollover.
seen = set(initial_log.decode(errors='replace').splitlines())
with Path(str(stem) + '-logcat.txt').open('w', encoding='utf-8') as log:
    next_capture = 0
    while time.monotonic() - started < args.seconds:
        if current_pid() != pid:
            reason = 'pid_exited_or_changed'
            break
        segment = entry()
        Path(str(stem) + '-entry.log').write_bytes(segment)
        output = call('logcat', '-d', '--pid=' + pid, '-v', 'threadtime', '-s', 'BO1NativeVulkan:I', '*:S')
        for line in output.decode(errors='replace').splitlines():
            if line not in seen:
                log.write(line + '\n')
                seen.add(line)
        log.flush()
        elapsed = time.monotonic() - started
        if elapsed >= next_capture:
            png = call('exec-out', 'screencap', '-p')
            valid = current_pid() == pid and png.startswith(b'\x89PNG\r\n\x1a\n')
            path = Path(str(stem) + f'-screen{len(captures) + 1}.png')
            if valid:
                path.write_bytes(png)
            captures.append({'seconds': elapsed, 'same_pid': valid, 'path': str(path) if valid else None})
            print(f'SDK{args.sdk} PID{pid} elapsed={elapsed:.1f}s screenshot_same_pid={valid}', flush=True)
            next_capture = elapsed + 30
        time.sleep(5)

final_pid = current_pid()
Path(str(stem) + '-entry.log').write_bytes(entry())
Path(str(stem) + '-crash.txt').write_bytes(call('logcat', '-b', 'crash', '-d'))
Path(str(stem) + '-thermal-end.txt').write_bytes(call('shell', 'dumpsys', 'thermalservice'))
if final_pid != pid:
    reason = 'pid_exited_or_changed'
    Path(str(stem) + '-exit-info.txt').write_bytes(call('shell', 'dumpsys', 'activity', 'exit-info', 'com.blackopsarm.dev'))
samples = []
for line in Path(str(stem) + '-logcat.txt').read_text(errors='replace').splitlines():
    if 'native_scene_profile ' not in line:
        continue
    row = {key: float(value) for key, value in re.findall(r'(\w+)=(-?[\d.]+)', line)}
    row['log_time'] = line[:18]
    samples.append(row)
room = sorted((row for row in samples if row.get('draws', 0) >= args.min_room_draws), key=lambda row: row['frame'])
keys = ['draws', 'passes', 'begin_ms', 'record_ms', 'submit_ms', 'fence_wait_ms', 'gpu_ms', 'key_ms', 'program_create_ms']
summary = {key: {'mean': statistics.mean(row[key] for row in room),
                 'median': statistics.median(row[key] for row in room),
                 'min': min(row[key] for row in room), 'max': max(row[key] for row in room)} for key in keys} if room else {}
rates = []
for before, after in zip(room, room[1:]):
    delta = (datetime.strptime('2000-' + after['log_time'], '%Y-%m-%d %H:%M:%S.%f') -
             datetime.strptime('2000-' + before['log_time'], '%Y-%m-%d %H:%M:%S.%f')).total_seconds()
    if 'submit_clock_ms' in before and 'submit_clock_ms' in after:
        delta = (after['submit_clock_ms'] - before['submit_clock_ms']) / 1000
    if delta > 0 and after['frame'] - before['frame'] == 10:
        rates.append(10 / delta)
span_seconds = None
sustained_fps = None
if len(room) > 1 and all('submit_clock_ms' in row for row in room):
    span_seconds = (room[-1]['submit_clock_ms'] - room[0]['submit_clock_ms']) / 1000
    if span_seconds > 0:
        sustained_fps = (room[-1]['frame'] - room[0]['frame']) / span_seconds
result = {'sdk': args.sdk, 'min_room_draws': args.min_room_draws, 'pid': pid, 'final_pid': final_pid, 'reason': reason,
          'elapsed_seconds': time.monotonic() - started, 'expected_markers': expected,
          'sustained_scene_fps': sustained_fps, 'profile_span_seconds': span_seconds,
          'captures': captures, 'samples': samples, 'room_summary': summary,
          'scene_submission_rates': rates, 'gpu_scope': 'offscreen scene, excludes uploads/window output'}
stem.with_suffix('.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
print(json.dumps({key: result[key] for key in ['sdk', 'pid', 'final_pid', 'reason', 'sustained_scene_fps', 'profile_span_seconds', 'room_summary']}), flush=True)
if final_pid != pid:
    raise SystemExit(2)
