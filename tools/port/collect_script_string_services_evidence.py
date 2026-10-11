"""Archive real synchronization prerequisites, separate from pool certification."""
from pathlib import Path
import argparse,hashlib,json,shutil
ROOT=Path(__file__).resolve().parents[2]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for n in ['build','host','run','output']:ap.add_argument('--'+n,type=Path,required=True)
    a=ap.parse_args()
    for p in vars(a).values():
        if not p.resolve().is_relative_to(ROOT):raise RuntimeError('Only Android paths allowed')
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);m={'schema':'BlackOpsARM.script-services-evidence.v1','files':{},'source_sha256':{},'pool_certified':False}
    for label,directory in [('host',a.host),('honor',a.run)]:
        for p in sorted(directory.rglob('*')):
            if p.is_file() and (p.suffix in ['.log','.json','.map'] or p.name=='command.txt'):
                name=label+'.'+p.relative_to(directory).as_posix().replace('/','.');shutil.copyfile(p,out/name);m['files'][name]={'source':p.resolve().relative_to(ROOT).as_posix(),'sha256':sha(p)}
    commands=[c for c in json.loads((a.build/'compile_commands.json').read_text(encoding='utf-8')) if 'script_string_services' in c['file']]
    (out/'compile_commands.json').write_text(json.dumps(commands,indent=2)+'\n',encoding='utf-8')
    for n in ['src/platform/atomic32.h','src/platform/android/script_string_services.h','src/platform/android/script_string_services.cpp','tools/port/tests/script_string_services_tests.cpp','tools/port/build_script_string_services_host.ps1','tools/port/run_script_string_services.py','tools/port/collect_script_string_services_evidence.py','tools/port/build_script_string_services_arm64.ps1']:m['source_sha256'][n]=sha(ROOT/n)
    for leaf in ['configure.log','build.log']:
        source=a.build/leaf;name='native.'+leaf;shutil.copyfile(source,out/name);m['files'][name]={'source':source.resolve().relative_to(ROOT).as_posix(),'sha256':sha(source)}
    result=json.loads((a.run/'results.json').read_text(encoding='utf-8'));assert result['status']=='passed' and result['cases'][0]['payload']['pool_certified'] is False
    m['device_results']=result;m['limitations']='Process lifetime locks and immutable server identity; no reset/unregister. Full SL/MT pool and wider threading model unvalidated.'
    (out/'manifest.json').write_text(json.dumps(m,indent=2)+'\n',encoding='utf-8');print('Frozen real service prerequisites:',len(m['files']),'files')
if __name__=='__main__':main()
