"""Collect shader57 regression evidence and the separate frozen56 Vulkan soak."""
import hashlib
import json
import re
from pathlib import Path

repo = Path(__file__).resolve().parents[2]
runs = repo/'build/run'
batch57_source_sha = '8de64664ef6092d36c0d6a15388fb64015918ba6e2b8782739af8689228d455a'
if hashlib.sha256((repo/'src/gfx_d3d/r_material_load_obj.cpp').read_bytes()).hexdigest() != batch57_source_sha:
    raise SystemExit('Batch57 source has changed; preserve these frozen reports and create a report for the new stage.')

def log(path):
    raw = path.read_bytes()
    encoding = 'utf-16' if raw[:2] in [b'\xff\xfe', b'\xfe\xff'] else 'utf-16-le' if b'\0' in raw[:100] else 'utf-8-sig'
    return raw.decode(encoding, errors='replace')

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def gate(name):
    run = runs/name
    console = log(run/'main/console_mp.log')
    emergency = run/'main/bo1_emergency.log'
    errors = log(emergency) if emergency.exists() else ''
    vk = run/'BO1Zombies_d3d9.log'
    exitfile = run/'exitcode.txt'
    saves = re.findall(r'^SaveGame check:.*', console, re.M)
    patterns = [r'^SaveGame check: script save IDs valid \(', r'^SaveGame check: segment 2 sections 1-3 read back identical \(',
                r'^SaveGame check: segment 1 weapons resolved \(', r'^SaveGame check: segment 4 header identical \(']
    row = dict(name=name, passCount=len(re.findall(r'^bo1_plan:.* end (?:recovery )?PASS\b', console, re.M)),
        failCount=len(re.findall(r'^bo1_plan:.* end (?:recovery )?FAIL\b', console, re.M)),
        complete=bool(re.search(r'^bo1_plan: time \d+ complete,', console, re.M)),
        completeLines=re.findall(r'^bo1_plan: time \d+ complete,.*', console, re.M),
        quitLines=re.findall(r'^bo1_plan:.*quit.*', console, re.M),
        exitCode=int(exitfile.read_text().strip()) if exitfile.exists() else None,
        crash='BO1_HEADLESS CRASH:' in errors,
        assertion=bool(re.search(r'BO1_HEADLESS ASSERT:|Bo1_NarrowFail', console+errors)),
        unknownSteps=re.findall(r'^bo1_plan: unknown step.*', console, re.M),
        saveChecks=saves, saveChecksValid=len(saves)==4 and all(sum(bool(re.match(p,s)) for s in saves)==1 for p in patterns),
        pap=re.findall(r'^bo1_plan:.* pap end PASS.*', console, re.M), glass=re.findall(r'^bo1_plan:.* glass end PASS.*', console, re.M),
        exeSha256=sha(run/'BO1Zombies.exe'), vulkanErrors=re.findall(r'^(?:err|error):.*', log(vk), re.M) if vk.exists() else [],
        captures=[p.name for p in (run/'frames').glob('*.ppm')])
    row['acceptedRun'] = (row['complete'] and not row['failCount'] and row['exitCode']==0 and row['saveChecksValid']
        and not row['crash'] and not row['assertion'] and not row['unknownSteps'] and not row['vulkanErrors'])
    if (run/'launch.json').exists():
        launch = json.loads(log(run/'launch.json'))
        row['backend']=launch['backend'];row['sound']=launch['sound'];row['god']=launch['god']
        row['dxvkSha256']=sha(run/'d3d9.dll');row['packageHashMatches']=row['dxvkSha256']==launch['package']['dllSha256'].lower()
        modules=run/'modules.json';row['modules']=json.loads(log(modules)) if modules.exists() else []
        required={'d3d9.dll','vulkan-1.dll','nvoglv64.dll','VkLayer_khronos_validation.dll'}
        row['vulkanModulesPresent']=required <= {m['ModuleName'] for m in row['modules']}
        row['acceptedRun'] &= row['packageHashMatches'] and row['vulkanModulesPresent']
    return row,console

names=[f'{p}_shader57_gate' for p in ['w32','x64hi','x64low','x64guard']]+['x64hi_dxvk57_pane2_final']
rows=[gate(name)[0] for name in names if (runs/name).exists()]
(runs/'shader57_results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps([{k:r[k] for k in ['name','passCount','failCount','complete','exitCode','acceptedRun']} for r in rows],indent=2))

focused=dict(sourceStage='working-copy-through-shader57', sourceSha256={}, cases={}, builds={}, logs={},
    scope='Actual extracted native compiler/shader load functions with real Microsoft D3DX COM and controlled generation/cache/device/failure adapters. Warning diagnostic is a controlled real COM buffer. GPU results, when present, use actual device creation/draw/readback. Full loose-file/reflection/cache disk pipeline remains outside coverage.')
for name in ['src/gfx_d3d/r_material_load_obj.cpp','src/gfx_d3d/r_material.h','src/gfx_d3d/r_material_load_obj.h',
             'tools/port/check_material_shader.py','tools/port/check_material_shader.ps1','tools/port/test_material_shader.cpp','tools/port/run_material_shader_gpu.ps1']:
    focused['sourceSha256'][name]=sha(repo/name)
for suffix in ['','_baseline']:
    focused['cases'][suffix or 'current']=json.loads(log(runs/f'shader57{suffix}_cases.json'))
    for arch in ['x86','x64']:
        path=runs/f'test_material_shader{suffix}_{arch}.log';focused['logs'][path.name]=log(path)
for label in ['hi','build','build-x64','build-x64-guard','build-x64-hi-guard']:
    path=repo/f'tools/port/native_shader57_{label}.log';focused['builds'][path.name]=dict(sha256=sha(path),text=log(path))
full=repo/'tools/port/native_full57.log'
if full.exists():
    text=log(full)
    root_prefix=str(repo).replace('\\','/')+'/'
    sites=sorted(set((m[0].replace('\\','/').removeprefix(root_prefix),int(m[1]))
        for m in re.findall(r'([A-Z]:[^\r\n]+?)\((\d+),\d+\): warning C43(?:11|12|02):',text)))
    focused['fullAudit']=dict(log='tools/port/native_full57.log',sha256=sha(full),sites=sites,uniqueCount=len(sites),
        srcTlCount=sum(p.startswith(('src/','tl/')) for p,n in sites), errorCount=len(re.findall(r'error C\d+|error LNK|fatal error',text)))
focused['gpuResults']={p.name:json.loads(log(p)) for p in runs.glob('shader57_gpu*_results.json')}
(runs/'shader57_focused_results.json').write_text(json.dumps(focused,indent=2)+'\n')

name='x64hi_dxvk56_soak30'
if (runs/name).exists():
    row,console=gate(name)
    samples=runs/name/'soak-samples.jsonl'
    row['samples']=[json.loads(line) for line in log(samples).splitlines() if line.strip()] if samples.exists() else []
    completes=re.findall(r'^bo1_plan: time (\d+) complete,',console,re.M)
    quits=re.findall(r'^bo1_plan: time (\d+) quit (\d+) ms after complete',console,re.M)
    row['continuedGameMs']=int(quits[-1][1]) if quits else None
    row['roundNotifies']=re.findall(r'^bo1_feature:.* level notify .* round \d+.*',console,re.M)
    row['maxRound']=max([int(n) for n in re.findall(r'^bo1_feature:.* level notify .* round (\d+) ',console,re.M)]+[0])
    row['maxSpawned']=max([int(n) for n in re.findall(r'^bo1_actors: time \d+ spawned (\d+)',console,re.M)]+[0])
    row['privateBytesRange']=[min(s['privateBytes'] for s in row['samples']),max(s['privateBytes'] for s in row['samples'])] if row['samples'] else []
    row['samplesAfterRoute']=[s for s in row['samples'] if completes and re.search(r' time (\d+) ',s['client']) and int(re.search(r' time (\d+) ',s['client'])[1])>=int(completes[-1])]
    row['acceptedSoak']=row['acceptedRun'] and row['continuedGameMs'] is not None and row['continuedGameMs']>=1800000
    row['sourceStage']='frozen-material56-executable; later-shader57-source/builds-independent'
    (runs/'vulkan56_soak_results.json').write_text(json.dumps(row,indent=2)+'\n')
    print(json.dumps({k:row[k] for k in ['name','maxRound','maxSpawned','continuedGameMs','exitCode','acceptedSoak']}))
