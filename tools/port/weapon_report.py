"""Compare saved Win32/native-x64 weapon catalogs and retain explicit runtime coverage."""
import argparse
import json
import re
from pathlib import Path

MAPS = ['zombie_pentagon','zombie_theater','zombie_cosmodrome','zombie_coast',
        'zombie_temple','zombie_moon','zombie_cod5_prototype','zombie_cod5_asylum',
        'zombie_cod5_sumpf','zombie_cod5_factory']

def fields(line):
    return dict(re.findall(r'(\w+)=([^\s]+)', line))

def catalog(run):
    path=run/'main/console_mp.log'
    if not path.exists(): return {}
    return {v['name']:v for line in path.read_text(encoding='latin1').splitlines()
            if line.startswith('bo1_sweep_catalog:') for v in [fields(line)]}

def exclusion(name, v):
    if 'lh_' in name or '_left' in name: return 'left-hand component (exercise paired weapon)'
    if name.startswith('explosive_bolt'): return 'crossbow projectile (exercise crossbow)'
    if name.startswith('sniper_explosive_bolt'): return 'Scavenger projectile (exercise sniper_explosive)'
    if name=='zombie_nesting_doll_single': return 'Matryoshka child projectile (exercise nesting doll)'
    if v['inv']=='2' and name!='claymore_zm': return 'scripted item/equipment, outside weapon activation sweep'
    if v['class'] in ['7','8'] or (v['type']=='0' and int(v['damage'])<=0): return 'non-player/dummy asset'
    if '_zm' not in name and not name.startswith('zombie_'): return 'non-Zombies asset'
    return None

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--catalog-prefix',default='weapcatalog33')
    parser.add_argument('--output',default='build/run/weapon_inventory33.json')
    parser.add_argument('--markdown',help='Optional reviewable Markdown report, relative to the repository')
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    runs=root/'build/run'
    report={'scope':MAPS,'maps':{},'differences':[],'missing_catalogs':[],
            'runtime_results':[], 'runtime_plan':{},'exclusions':{},
            'upgrade_mappings':{},'mapping_differences':[],'runs':{},
            'note':'Catalog comparison is checked asset metadata evidence. Runtime PASS measures activation, ADS/reload/switch where applicable; actor damage/death observations during a case are separate. Shared cases are exercised once per architecture; the Pack-a-Punch machine is not used for every grant. Dead Ops Arcade is a separate mode without Pack-a-Punch and is blocked by an unsupported server script function.'}
    seen=set()
    for mapname in MAPS:
        suffix='pentagon' if mapname=='zombie_pentagon' else mapname
        catalogs={arch:catalog(runs/f'{arch}_{args.catalog_prefix}_{suffix}') for arch in ['w32','x64hi']}
        for arch,cat in catalogs.items():
            if not cat: report['missing_catalogs'].append(f'{arch}:{mapname}')
        left,right=catalogs['w32'],catalogs['x64hi']
        for name in sorted(set(left)|set(right)):
            l={k:v for k,v in left.get(name,{}).items() if k not in ['def','variant','candidate']}
            r={k:v for k,v in right.get(name,{}).items() if k not in ['def','variant','candidate']}
            if l!=r: report['differences'].append({'map':mapname,'name':name,'win32':l,'x64':r})
        active=[]
        for name,v in left.items():
            reason=exclusion(name,v)
            if reason:
                report['exclusions'].setdefault(reason,[]).append(f'{mapname}:{name}')
                continue
            active.append(name)
            # Same-name assets with different numeric metadata need their own map run.
            signature=tuple((k,val) for k,val in v.items() if k not in ['map','w','parent','alt','dual','def','variant','candidate'])
            if signature not in seen:
                report['runtime_plan'].setdefault(mapname,[]).append(name)
                seen.add(signature)
        report['maps'][mapname]={'registered':len(left),'combat_variants':len(active),'names':active,
                                'matching':len(left)==len(right) and all(d['map']!=mapname for d in report['differences'])}
    for path in sorted(runs.glob('*weap*/main/console_mp.log')):
        lines=path.read_text(encoding='latin1').splitlines()
        runname=path.parents[1].name
        exitfile=path.parents[1]/'exitcode.txt'
        exitcode=exitfile.read_text().strip() if exitfile.exists() else 'pending'
        complete=[fields(line) for line in lines if line.startswith('bo1_sweep_complete:')]
        emergency=path.parent/'bo1_emergency.log'
        crashed=emergency.exists() and 'BO1_HEADLESS CRASH:' in emergency.read_text(encoding='latin1')
        report['runs'][runname]={'exit':exitcode,'complete':complete,
            'crash':crashed or any(line.startswith('BO1_HEADLESS CRASH:') for line in lines)}
        arch=runname.split('_')[0]
        if arch not in ['w32','x64hi']: continue
        for line in lines:
            if line.startswith('bo1_sweep_mapping:'):
                v=fields(line)
                if v.get('upgrade') and v.get('map') and v.get('base'):
                    report['upgrade_mappings'].setdefault(v['map'],{}).setdefault(arch,{})[v['base']]=v['upgrade']
            if line.startswith('bo1_sweep_result:'):
                if ' PASS ' not in line and ' FAIL ' not in line: continue
                v=fields(line); v['run']=runname
                v['status']='FAIL' if ' FAIL ' in line else 'PASS'
                v['exit']=exitcode
                report['runtime_results'].append(v)
    for mapname,archives in report['upgrade_mappings'].items():
        if 'w32' in archives and 'x64hi' in archives and archives['w32']!=archives['x64hi']:
            report['mapping_differences'].append(mapname)
    report['coverage']=[]
    for mapname,names in report['runtime_plan'].items():
        for name in names:
            case={'map':mapname,'name':name}
            for arch in ['w32','x64hi']:
                accepted=[v for v in report['runtime_results']
                          if v.get('map')==mapname and v.get('name')==name
                          and v['run'].startswith(arch+'_') and v['status']=='PASS'
                          and v['exit']=='0' and report['runs'][v['run']]['complete']
                          and not report['runs'][v['run']]['crash']]
                case[arch]=accepted[-1] if accepted else None
            report['coverage'].append(case)
    report['accepted_counts']={arch:sum(bool(c[arch]) for c in report['coverage']) for arch in ['w32','x64hi']}
    report['unique_names']=len({c['name'] for c in report['coverage']})
    report['upgraded_cases']=sum('upgraded' in c['name'] for c in report['coverage'])
    report['registered_observations']=sum(v['registered'] for v in report['maps'].values())
    output=root/args.output
    output.write_text(json.dumps(report,indent=2)+'\n')
    if args.markdown:
        md=['# Zombies weapon sweep - Windows x64', '',
            'Generated from saved catalogs and runtime logs. A case counts as accepted only when it passed in a completed run with exit 0 and no emergency crash.', '',
            f"Coverage: Win32 **{report['accepted_counts']['w32']}/{len(seen)}**, high-address x64 **{report['accepted_counts']['x64hi']}/{len(seen)}**.", '',
            f"Plan: {len(seen)} cases, {report['unique_names']} distinct asset names, including {report['upgraded_cases']} upgraded cases. Nacht's different Thundergun definition is a separate case.", '',
            f"Catalogs: {len(MAPS)*2-len(report['missing_catalogs'])}/20; checked metadata differences: {len(report['differences'])}; Pack-a-Punch mapping differences: {len(report['mapping_differences'])}.", '',
            '## Scope and limits', '',
            '- Ten installed round-based Zombies maps. Shared definitions are exercised once per architecture on Pentagon; map-exclusive or numerically different definitions are exercised on their respective map. Every map has a separate Win32/x64 catalog comparison.',
            '- Real rendered client, sound enabled, automated user commands, god mode. Each case lasts 18 game seconds and checks activation plus ADS, reload and switching where applicable. Dual weapons and alternate fire are separate cases where registered.',
            '- Upgraded weapons are granted directly with retail gold camo. This does not test the Pack-a-Punch machine for every base weapon. The earlier Pentagon route tested the machine separately.',
            '- Actor damage/death counters are observations during a case, not guaranteed weapon-specific attribution. These checks do not prove every special effect, revive effect, collision or multiplayer interaction correct. No valid visual screenshots were collected.',
            '- Dead Ops Arcade (zombietron) is a separate mode without Pack-a-Punch. It reaches an unsupported server GetDvarVector function on both architectures; gameplay is not covered. Its native flame-table loader failure was repaired.',
            '- Scripted items, perk bottles, non-player assets and internal projectile/left-hand components are excluded from independent cases; parent weapons exercise the components.', '',
            '## Cases', '', '| Map | Weapon asset | Win32 | High-address x64 |', '| --- | --- | --- | --- |']
        for c in report['coverage']:
            cells=[f"PASS (`{c[a]['run']}`)" if c[a] else '**PENDING**' for a in ['w32','x64hi']]
            md.append(f"| {c['map']} | `{c['name']}` | {cells[0]} | {cells[1]} |")
        md+=['', '## Retained failed cases', '']
        failed=[v for v in report['runtime_results'] if v['status']=='FAIL']
        md += [f"- `{v['run']}`: `{v['name']}`." for v in failed] or ['None recorded.']
        md+=['', '## Incomplete or crashed sweep runs', '']
        incomplete=[(n,v) for n,v in report['runs'].items() if 'sweep' in n and (not v['complete'] or v['exit']!='0' or v['crash'])]
        md += [f"- `{n}`: exit `{v['exit']}`, completed={bool(v['complete'])}, crash={v['crash']}." for n,v in incomplete] or ['None recorded.']
        (root/args.markdown).write_text('\n'.join(md)+'\n',encoding='utf8')
    print(f'catalogs={len(MAPS)*2-len(report["missing_catalogs"])} differences={len(report["differences"])} unique_tests={len(seen)}')
    for mapname,names in report['runtime_plan'].items(): print(f'{mapname}: {len(names)} '+','.join(names))
    print(f'report={output}')
    print(f"accepted={report['accepted_counts']}")

if __name__=='__main__': main()
