"""Build/package one changed SDK engine and observe a fresh real launcher action.

Development helper: no asset import, test fixtures, provider changes or readiness
claims. Observer completion is not engine success. Each SDK/stage is unique.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import os
import sys
import xml.etree.ElementTree as ET
import zipfile

p = argparse.ArgumentParser()
p.add_argument('--sdk', type=int, required=True)
p.add_argument('--build', default='build-android-audit/native_engine_entry1')
p.add_argument('--stage', type=int, required=True)
p.add_argument('--asan', action='store_true', help='Build/stage/package the explicitly configured separate ASan diagnostic tree')
p.add_argument('--height', type=int, choices=(0,540,720,1080), default=0)
p.add_argument('--seconds', type=int, default=180)
a = p.parse_args()
root = Path(__file__).resolve().parents[2]
build = (root / a.build).resolve()
assert build.is_relative_to(root), 'Build must stay inside BlackOpsARM'
out = root / 'build-android-audit/native_registry_closure'
stage = root / f'build-android-audit/apk-native-stage{a.stage}'
apk = root / 'android/app/build/outputs/apk/debug/app-debug.apk'
adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', 'AKWJ9X5116G04618']
assert not stage.exists(), 'Preserve previous stage evidence; select a fresh number'
assert not (out / f'sdk-engine-shared-build{a.sdk}.log').exists()
if a.asan:
    cache = (build / 'CMakeCache.txt').read_text(encoding='utf-8')
    if not re.search(r'^BO1_NATIVE_ASAN:[^=]+=ON$', cache, re.M) or not re.search(r'^ANDROID_STL:[^=]+=c\+\+_shared$', cache, re.M):
        p.error('--asan requires a separate ASan/shared-STL configured build directory')


def logged(command, name, cwd=root, env=None):
    path = out / name
    with path.open('wb') as f:
        result = subprocess.run(command, cwd=cwd, env=env, stdout=f, stderr=subprocess.STDOUT)
    print('\n'.join(path.read_text(errors='replace').splitlines()[-6:]), flush=True)
    if result.returncode:
        raise RuntimeError(f'{name}: exit {result.returncode}')

def device(*args):
    result = subprocess.run(adb + list(args), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode:
        raise RuntimeError(result.stderr.decode(errors='replace'))
    return result.stdout

logged(['D:/Android/Sdk/cmake/3.22.1/bin/cmake.exe', '--build',
        str(build), '--target',
        'blackopsarm_engine', '--parallel', '4'], f'sdk-engine-shared-build{a.sdk}.log')
logged([sys.executable, str(root / 'tools/port/stage_native_engine_apk.py'),
        '--build', str(build),
        '--output', str(stage)] + (['--asan'] if a.asan else []), f'sdk-stage{a.stage}.log')
env = os.environ.copy()
env['JAVA_HOME'] = 'C:/Users/recox/scoop/apps/temurin21-jdk/current'
logged(['cmd.exe', '/c', 'gradlew.bat', ':app:assembleDebug',
        '-Pbo1NativePrebuiltDir=' + str(stage)] + (['-Pbo1NativeAsanWrapDir=' + str(stage / 'asan-resources')] if a.asan else []), f'sdk-apk-build{a.stage}.log',
       cwd=root / 'android', env=env)
rows = json.loads((stage / 'native-manifest.json').read_text())
checks = []
with zipfile.ZipFile(apk) as z:
    for row in rows:
        data = z.read('lib/arm64-v8a/' + row['library'])
        digest = hashlib.sha256(data).hexdigest()
        assert digest == row['sha256']
        checks.append({'library': row['library'], 'sha256': digest,
                       'bytes': len(data), 'build_id': row['build_id']})
wrap_verification = None
with zipfile.ZipFile(apk) as z:
    wrap_path = 'lib/arm64-v8a/wrap.sh'
    if a.asan:
        wrap_verification = json.loads((stage / 'asan-wrap-manifest.json').read_text())
        wrapper = z.read(wrap_path)
        assert hashlib.sha256(wrapper).hexdigest() == wrap_verification['sha256']
        assert len(wrapper) == wrap_verification['bytes']
    else:
        assert wrap_path not in z.namelist(), 'Ordinary APK unexpectedly contains diagnostic wrapping'
verification = {'apk_sha256': hashlib.sha256(apk.read_bytes()).hexdigest(), 'libraries': checks, 'asan_wrap': wrap_verification}
(out / f'sdk-package-verification{a.stage}.json').write_text(
    json.dumps(verification, indent=2) + '\n', encoding='utf-8')
print('PASS: all packaged providers match; engine BuildId', checks[0]['build_id'], flush=True)
installed = device('install', '-r', str(apk))
(out / f'sdk-install{a.sdk}.txt').write_bytes(installed)
assert b'Success' in installed
print(installed.decode().strip(), flush=True)
try:
    device('shell', 'svc', 'power', 'stayon', 'usb')
    device('shell', 'input', 'keyevent', 'KEYCODE_WAKEUP')
    device('shell', 'wm', 'dismiss-keyguard')
    device('shell', 'am', 'start', '-W', '-n', 'com.blackopsarm.dev/.AssetImportActivity', '--ez', 'nativeVulkanBackend', 'true', '--ez', 'nativeAutoKino', 'true', '--ei', 'nativeVulkanHeight', str(a.height))
    remote = f'/data/local/tmp/blackopsarm-ui{a.sdk}.xml'
    device('shell', 'uiautomator', 'dump', remote)
    local = out / f'sdk-importer-ui{a.sdk}.xml'
    device('pull', remote, str(local))
    nodes = list(ET.parse(local).iter('node'))
    for node in nodes:
        if node.get('text'):
            print(node.get('text'), node.get('bounds'), node.get('enabled'), flush=True)
    launch = [n for n in nodes if n.get('text') == 'LAUNCH GAME' and n.get('enabled') == 'true']
    assert len(launch) == 1, 'Launch must be observed enabled; no import fallback'
    bounds = [int(v) for v in re.findall(r'\d+', launch[0].get('bounds', ''))]
    assert len(bounds) == 4
    raw = device('shell', 'run-as', 'com.blackopsarm.dev', 'cat', 'files/user/native-entry.log')
    count = raw.count(b'ENGINE ENTRY SDK stdout/stderr captured')
    (out / f'sdk{a.sdk}-previous-markers.txt').write_text(str(count), encoding='utf-8')
    print('Previous launch markers', count, flush=True)
    device('shell', 'input', 'tap', str((bounds[0]+bounds[2])//2), str((bounds[1]+bounds[3])//2))
    result = subprocess.run([sys.executable, str(out / 'observe_sdk_launch.py'),
                             '--sdk', str(a.sdk), '--seconds', str(a.seconds),
                             '--previous-markers', str(count)], cwd=root)
    assert result.returncode == 0, 'Observer execution failed'
finally:
    device('shell', 'svc', 'power', 'stayon', 'false')
    print('USB stay-awake restored to original0.', flush=True)
