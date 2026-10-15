"""Build/package one changed SDK and test native Vulkan presentation only.

Development helper: existing assets reused; only changed providers and an
explicit captured-image fixture staged. No Com_Init or game-frame claim.
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
p.add_argument('--stage', type=int, required=True)
p.add_argument('--seconds', type=int, default=180)
p.add_argument('--fixture',required=True,help='NVR1 captured image for material preview; not retail game assets')
a = p.parse_args()
root = Path(__file__).resolve().parents[2]
out = root / 'build-android-audit/native_registry_closure'
stage = root / f'build-android-audit/apk-native-stage{a.stage}'
apk = root / 'android/app/build/outputs/apk/debug/app-debug.apk'
adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', 'AKWJ9X5116G04618']
assert not stage.exists(), 'Preserve previous stage evidence; select a fresh number'
assert not (out / f'sdk-engine-shared-build{a.sdk}.log').exists()

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
        str(root / 'build-android-audit/native_engine_entry1'), '--target',
        'blackopsarm_engine', '--parallel', '4'], f'sdk-engine-shared-build{a.sdk}.log')
logged([sys.executable, str(root / 'tools/port/stage_native_engine_apk.py'),
        '--build', str(root / 'build-android-audit/native_engine_entry1'),
        '--output', str(stage)], f'sdk-stage{a.stage}.log')
env = os.environ.copy()
env['JAVA_HOME'] = 'C:/Users/recox/scoop/apps/temurin21-jdk/current'
logged(['cmd.exe', '/c', 'gradlew.bat', ':app:assembleDebug',
        '-Pbo1NativePrebuiltDir=' + str(stage)], f'sdk-apk-build{a.stage}.log',
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
verification = {'apk_sha256': hashlib.sha256(apk.read_bytes()).hexdigest(), 'libraries': checks}
(out / f'sdk-package-verification{a.stage}.json').write_text(
    json.dumps(verification, indent=2) + '\n', encoding='utf-8')
print('PASS: all packaged providers match; engine BuildId', checks[0]['build_id'], flush=True)
installed = device('install', '-r', str(apk))
(out / f'sdk-install{a.sdk}.txt').write_bytes(installed)
assert b'Success' in installed
print(installed.decode().strip(), flush=True)
capture=(root/a.fixture).resolve().read_bytes()
assert capture[:4] in (b'NVR1',b'NVB3')
existingCapture=subprocess.run(adb+['shell','run-as','com.blackopsarm.dev','sha256sum','files/user/native-vulkan-preview.bin'],capture_output=True)
if existingCapture.returncode!=0 or existingCapture.stdout.decode().split()[0]!=hashlib.sha256(capture).hexdigest():
    subprocess.run(adb+['exec-in',"run-as com.blackopsarm.dev sh -c 'cat > files/user/native-vulkan-preview.bin'"],input=capture,check=True)
# exec-in can finish locally before the remote writer has closed its file.
# Poll the actual checksum; never launch with an incomplete capture.
import time
for captureCheck in range(20):
    deviceCapture=device('shell','run-as','com.blackopsarm.dev','sha256sum','files/user/native-vulkan-preview.bin').decode().split()[0]
    if deviceCapture==hashlib.sha256(capture).hexdigest():break
    time.sleep(0.1)
assert deviceCapture==hashlib.sha256(capture).hexdigest(),'Capture transfer must be byte-exact before launch'
(out/f'sdk-native-vulkan{a.sdk}-fixture.json').write_text(json.dumps({'sha256':hashlib.sha256(capture).hexdigest(),'bytes':len(capture),'source':a.fixture}),encoding='utf-8')
try:
    device('shell','svc','power','stayon','usb')
    device('shell','input','keyevent','KEYCODE_WAKEUP')
    device('shell','wm','dismiss-keyguard')
    device('shell','am','start','-W','-n','com.blackopsarm.dev/.AssetImportActivity','--ez','nativeVulkanPresentation','true')
    remote=f'/data/local/tmp/native-vulkan-ui{a.sdk}.xml'
    device('shell','uiautomator','dump',remote)
    local=out/f'sdk-native-vulkan{a.sdk}-ui.xml'
    device('pull',remote,str(local))
    nodes=list(ET.parse(local).iter('node'))
    launch=[n for n in nodes if n.get('text')=='LAUNCH GAME' and n.get('enabled')=='true']
    assert len(launch)==1,'Existing assets must be ready; never reimport'
    bounds=[int(v) for v in re.findall(r'\d+',launch[0].get('bounds',''))]
    device('shell','input','tap',str((bounds[0]+bounds[2])//2),str((bounds[1]+bounds[3])//2))
    import time
    time.sleep(10)
    raw=device('logcat','-d','-s','BO1NativeVulkan:I','*:S')
    (out/f'sdk-native-vulkan{a.sdk}-presentation.log').write_bytes(raw)
    pid=device('shell','pidof','com.blackopsarm.dev:engine').decode().strip()
    current=b'\n'.join(line for line in raw.splitlines() if re.search(rb'\s'+pid.encode()+rb'\s',line))
    print(current.decode(errors='replace'),flush=True)
    assert b'PRESENTATION_ONLY frames=' in current and b'frame failed' not in current and b'open failed' not in current
    screen=device('exec-out','screencap','-p')
    (out/f'sdk-native-vulkan{a.sdk}-screen.png').write_bytes(screen)
    print('Native presentation process remains live:',pid,flush=True)
finally:
    device('shell','svc','power','stayon','false')
