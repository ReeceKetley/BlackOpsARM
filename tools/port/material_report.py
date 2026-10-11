"""Report retained material56 runtime gates and focused checks without relaunching."""
import hashlib
import json
import re
from pathlib import Path

repo = Path(__file__).resolve().parents[2]
runs = repo / 'build/run'
# This report describes frozen batch56 evidence. Later source must not be
# recorded as though those old executables tested it.
batch56_source_sha = 'fe30b3c0febd90cf5081d41f8c8ac8c5ca31f481845593162177f6941532d905'
if hashlib.sha256((repo / 'src/gfx_d3d/r_material_load_obj.cpp').read_bytes()).hexdigest() != batch56_source_sha:
    raise SystemExit('Batch56 source has changed; preserve its reports and use shader_report.py for batch57.')
names = [f'{arch}_material56_gate' for arch in ['w32', 'x64hi', 'x64low', 'x64guard']]
names += ['x64hi_dxvk56_pane2_final']
results = []
for name in names:
    run = runs / name
    if not run.exists():
        continue
    console = (run / 'main/console_mp.log').read_text(errors='replace')
    emergency = run / 'main/bo1_emergency.log'
    vk = run / 'BO1Zombies_d3d9.log'
    exitfile = run / 'exitcode.txt'
    saves = re.findall(r'^SaveGame check:.*', console, re.M)
    saves_valid = all(sum(bool(re.match(pattern, line)) for line in saves) == 1 for pattern in [
        r'^SaveGame check: script save IDs valid \(',
        r'^SaveGame check: segment 2 sections 1-3 read back identical \(',
        r'^SaveGame check: segment 1 weapons resolved \(',
        r'^SaveGame check: segment 4 header identical \('])
    row = dict(name=name, sourceStage='working-copy-through-material56',
        passCount=len(re.findall(r'^bo1_plan:.* end (?:recovery )?PASS\b', console, re.M)),
        failCount=len(re.findall(r'^bo1_plan:.* end (?:recovery )?FAIL\b', console, re.M)),
        complete=bool(re.search(r'^bo1_plan: time \d+ complete,', console, re.M)),
        exitCode=int(exitfile.read_text().strip()) if exitfile.exists() else None,
        crash=emergency.exists() and 'BO1_HEADLESS CRASH:' in emergency.read_text(errors='replace'),
        assertion=bool(re.search(r'BO1_HEADLESS ASSERT:|Bo1_NarrowFail', console)) or
            (emergency.exists() and 'BO1_HEADLESS ASSERT:' in emergency.read_text(errors='replace')),
        saveChecks=saves,
        saveChecksValid=saves_valid,
        pap=re.findall(r'^bo1_plan:.* pap end PASS.*', console, re.M),
        glass=re.findall(r'^bo1_plan:.* glass end PASS.*', console, re.M),
        completeLines=re.findall(r'^bo1_plan: time \d+ complete,.*', console, re.M),
        quitLines=re.findall(r'^bo1_plan:.*quit.*', console, re.M),
        unknownSteps=re.findall(r'^bo1_plan: unknown step.*', console, re.M),
        exeSha256=hashlib.sha256((run / 'BO1Zombies.exe').read_bytes()).hexdigest(),
        vulkanErrors=re.findall(r'^(?:err|error):.*', vk.read_text(errors='replace'), re.M) if vk.exists() else [],
        captures=[p.name for p in (run / 'frames').glob('*')])
    if (run / 'launch.json').exists():
        launch = json.loads((run / 'launch.json').read_text(encoding='utf-8-sig'))
        row['backend'] = launch['backend']
        row['sound'] = launch['sound']
        row['god'] = launch['god']
        row['dxvkSha256'] = hashlib.sha256((run / 'd3d9.dll').read_bytes()).hexdigest()
        row['packageHashMatches'] = row['dxvkSha256'] == launch['package']['dllSha256'].lower()
        modules = run / 'modules.json'
        row['modules'] = json.loads(modules.read_text(encoding='utf-8-sig')) if modules.exists() else []
    row['acceptedRun'] = (row['complete'] and row['failCount'] == 0 and row['exitCode'] == 0
        and not row['crash'] and not row['assertion'] and len(saves) == 4 and saves_valid
        and not row['unknownSteps'] and not row['vulkanErrors'])
    if 'backend' in row:
        required = {'d3d9.dll', 'vulkan-1.dll', 'nvoglv64.dll', 'VkLayer_khronos_validation.dll'}
        row['vulkanModulesPresent'] = required <= {m['ModuleName'] for m in row['modules']}
        row['acceptedRun'] &= row['packageHashMatches'] and row['vulkanModulesPresent']
    results.append(row)
(runs / 'material56_results.json').write_text(json.dumps(results, indent=2) + '\n')
print(json.dumps([{k: r[k] for k in ['name', 'passCount', 'failCount', 'complete', 'exitCode', 'acceptedRun']} for r in results], indent=2))

focused = dict(sourceStage='working-copy-through-material56',
    scope='Production native allocations, technique/pass/argument layout, scratch capacity, state remapping and deduplication. File parsing, shader reflection/linkage, COM compilation and full loose-material loading are controlled helpers or outside scope.',
    sourceSha256={}, logs={}, cases={}, builds={})
for path in ['src/gfx_d3d/r_material_load_obj.cpp', 'src/gfx_d3d/r_material.h', 'src/gfx_d3d/r_material_load_obj.h',
             'tools/port/check_material_native.py', 'tools/port/check_material_native.ps1', 'tools/port/test_material_native.cpp']:
    focused['sourceSha256'][path] = hashlib.sha256((repo / path).read_bytes()).hexdigest()
for suffix in ['', '_baseline']:
    focused['cases'][suffix or 'current'] = json.loads((runs / f'material56{suffix}_cases.json').read_text(encoding='utf-8-sig'))
    for arch in ['x86', 'x64']:
        path = runs / f'test_material_native{suffix}_{arch}.log'
        raw = path.read_bytes()
        encoding = 'utf-16' if raw[:2] in [b'\xff\xfe', b'\xfe\xff'] else 'utf-8'
        focused['logs'][path.name] = raw.decode(encoding, errors='replace')
for label in ['hi', 'build', 'build-x64', 'build-x64-guard', 'build-x64-hi-guard']:
    path = repo / f'tools/port/native_material56_{label}.log'
    raw = path.read_bytes()
    encoding = 'utf-16' if raw[:2] in [b'\xff\xfe', b'\xfe\xff'] else 'utf-16-le' if b'\0' in raw[:100] else 'utf-8'
    focused['builds'][path.name] = dict(sha256=hashlib.sha256(raw).hexdigest(), text=raw.decode(encoding, errors='replace'))
(runs / 'material56_focused_results.json').write_text(json.dumps(focused, indent=2) + '\n')
