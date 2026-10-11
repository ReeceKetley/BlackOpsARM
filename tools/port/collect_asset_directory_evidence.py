"""Freeze bounded asset-directory tests, diagnostics and executable provenance."""
from pathlib import Path
import argparse,collections,hashlib,json,re,shutil,struct,subprocess
ROOT=Path(__file__).resolve().parents[2]
LLVM=Path('D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for n in ['build','host','run','windows-regression','output']:ap.add_argument('--'+n,type=Path,required=True)
    a=ap.parse_args()
    for p in vars(a).values():
        if not p.resolve().is_relative_to(ROOT):raise RuntimeError('Only Android worktree paths allowed')
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    m={'schema':'BlackOpsARM.TASK-0016.evidence.v1','files':{},'builds':{},'artifacts':[],'source_sha256':{}}
    def copy(p,name):shutil.copyfile(p,out/name);m['files'][name]={'source':p.resolve().relative_to(ROOT).as_posix(),'sha256':sha(p)}
    for directory in sorted(a.build.parent.glob('asset_directory_pass*')):
        for leaf in ['configure.log','build.log','compile_commands.json','device-tests.log','source-preservation.json','review-evidence.md']:
            if (directory/leaf).is_file():copy(directory/leaf,directory.name+'.'+leaf)
        log=(directory/'build.log').read_text(encoding='utf-8-sig');m['builds'][directory.name]={'compiler_errors':len(re.findall(r'^.+?:\d+:\d+: (?:fatal )?error:',log,re.M)),'linker_errors':len(re.findall(r'^ld.lld: error:',log,re.M)),'warnings':len(re.findall(r'^.+?:\d+:\d+: warning:',log,re.M))}
    for pattern in ['asset_directory_host_pass*','asset_directory_run*']:
        for directory in sorted(a.build.parent.glob(pattern)):
            for p in sorted(directory.rglob('*')):
                if p.is_file() and (p.suffix in ['.json','.log','.map'] or p.name=='command.txt'):copy(p,directory.name+'.'+p.relative_to(directory).as_posix().replace('/','.'))
    for p in sorted(a.windows_regression.rglob('*')):
        if p.is_file() and (p.suffix in ['.log','.json'] or p.name.endswith('command.txt')):copy(p,'windows.'+p.relative_to(a.windows_regression).as_posix().replace('/','.'))
    binaries=sorted(p for p in a.build.glob('bo1_*tests') if p.is_file())
    for p in sorted(a.build.rglob('*.o'))+binaries:
        b=p.read_bytes()
        if b[:6]!=b'\x7fELF\x02\x01' or struct.unpack_from('<H',b,18)[0]!=183:raise RuntimeError('Wrong native architecture')
        m['artifacts'].append({'path':p.resolve().relative_to(ROOT).as_posix(),'sha256':sha(p),'machine':'AArch64'})
    symbols=subprocess.check_output([LLVM/'llvm-nm.exe','-C',a.build/'bo1_asset_directory_tests'],text=True)
    required=['Load_XStringCustom(','DB_ExpandLoad_Ptr32Array(','DB_ExpandLoad_XAssetArray(','DB_InitStreams(','DB_MemAlloc(','DecodedSource::ReadCString(']
    forbidden=['Load_TempStringCustom(','SL_GetString(','Load_XAssetListCustom(','DB_InitZone(']
    if any(x not in symbols for x in required) or any(x in symbols for x in forbidden):raise RuntimeError('Incorrect asset-directory executable boundary')
    (out/'asset_directory_symbols.txt').write_text(symbols,encoding='utf-8');m['production_symbols_present']=required;m['later_symbols_absent']=forbidden
    for c in json.loads((a.build/'compile_commands.json').read_text(encoding='utf-8')):
        if '-DBO1_PORT_X64' in c['command']:raise RuntimeError('Android legacy flag enabled')
        p=Path(c['file']).resolve()
        if p.is_relative_to(ROOT):m['source_sha256'][p.relative_to(ROOT).as_posix()]=sha(p)
    for name in ['src/platform/fastfile_asset_directory.h','src/platform/atomic32.h','src/platform/android/script_string_services.h','tools/port/run_script_string_services.py','src/platform/fastfile_raw_string.h','src/platform/fastfile_script_table.h','src/database/db_stream.h','src/database/db_stream_load.h','tools/port/tests/frontend_zone_fixture.h','tools/port/asset_directory_oracle.py','tools/port/run_asset_directory_gate.py','tools/port/verify_raw_strings_source.py','tools/port/build_asset_directory_host.ps1','tools/port/collect_asset_directory_evidence.py']:m['source_sha256'][name]=sha(ROOT/name)
    for arch,machine in [('x86',0x14c),('x64',0x8664)]:
        p=a.host/arch/'asset_directory.exe';b=p.read_bytes();pe=struct.unpack_from('<I',b,0x3c)[0]
        if b[pe:pe+4]!=b'PE\0\0' or struct.unpack_from('<H',b,pe+4)[0]!=machine:raise RuntimeError('Wrong Windows architecture')
        m['artifacts'].append({'path':p.resolve().relative_to(ROOT).as_posix(),'sha256':sha(p),'machine':arch})
    m.update(aarch64_artifact_count=len(m['artifacts'])-2,native_test_executables=[p.name for p in binaries],final_build=a.build.as_posix(),final_host=a.host.as_posix(),final_run=a.run.as_posix(),reader_boundary='Production borrowed DecodedSource plus finite fixture callback, not full asynchronous/refill engine loader',scope='Actual raw strings and asset-directory table; no interning, offset conversion, dispatch/payloads or graphics',original_buffers_in_git=False)
    (out/'manifest.json').write_text(json.dumps(m,indent=2)+'\n',encoding='utf-8');print('Frozen',len(m['files']),'evidence files;',m['aarch64_artifact_count'],'AArch64 artifacts')
if __name__=='__main__':main()
