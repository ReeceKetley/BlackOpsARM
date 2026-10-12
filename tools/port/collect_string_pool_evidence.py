"""Freeze real pool lifecycle, genuine temporary strings and all failed attempts."""
from pathlib import Path
import argparse,hashlib,json,re,shutil,struct,subprocess
ROOT=Path(__file__).resolve().parents[2]
LLVM=Path('D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64/bin')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for n in ['build','host','intern-host','run','normal-windows','narrow-regression','narrow-output','output']:ap.add_argument('--'+n,type=Path,required=True)
    a=ap.parse_args()
    for p in vars(a).values():
        if not p.resolve().is_relative_to(ROOT):raise RuntimeError('Only Android worktree outputs allowed')
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    m={'schema':'BlackOpsARM.TASK-0017.evidence.v1','files':{},'builds':{},'artifacts':[],'source_sha256':{}}
    def copy(p,name):shutil.copyfile(p,out/name);m['files'][name]={'source':p.resolve().relative_to(ROOT).as_posix(),'sha256':sha(p)}
    for pattern in ['string_pool_pass*','string_pool_optimized_pass*']:
        for directory in sorted(a.build.parent.glob(pattern)):
            for leaf in ['configure.log','build.log','compile_commands.json','device-tests.log','loader-source-preservation.json','pool-source-preservation.json','review-evidence.md']:
                if (directory/leaf).is_file():copy(directory/leaf,directory.name+'.'+leaf)
            log=(directory/'build.log').read_text(encoding='utf-8-sig');m['builds'][directory.name]={'compiler_errors':len(re.findall(r'^.+?:\d+:\d+: (?:fatal )?error:',log,re.M)),'linker_errors':len(re.findall(r'^ld.lld: error:',log,re.M)),'warnings':len(re.findall(r'^.+?:\d+:\d+: warning:',log,re.M))}
    for pattern in ['string_pool_host_pass*','intern_strings_host_pass*','string_pool_fixtures_run*','string_pool_genuine_run*','string_pool_optimized_run*']:
        for directory in sorted(a.build.parent.glob(pattern)):
            for p in sorted(directory.rglob('*')):
                if p.is_file() and (p.suffix in ['.log','.json','.map'] or p.name=='command.txt'):copy(p,directory.name+'.'+p.relative_to(directory).as_posix().replace('/','.'))
    for label,directory in [('normal',a.normal_windows),('narrow-run',a.narrow_regression),('narrow',a.narrow_output)]:
        for p in sorted(directory.rglob('*')):
            if p.is_file() and (p.suffix in ['.log','.json'] or p.name.endswith('command.txt')):copy(p,label+'.'+p.relative_to(directory).as_posix().replace('/','.'))
    for label,directory in [('debug',a.build),('optimized',a.build.parent/'string_pool_optimized_pass1')]:
        binaries=sorted(p for p in directory.glob('bo1_*tests') if p.is_file())
        for p in sorted(directory.rglob('*.o'))+binaries:
            data=p.read_bytes()
            if data[:6]!=b'\x7fELF\x02\x01' or struct.unpack_from('<H',data,18)[0]!=183:raise RuntimeError('Wrong native architecture')
            m['artifacts'].append({'path':p.resolve().relative_to(ROOT).as_posix(),'sha256':sha(p),'machine':'AArch64','configuration':label})
        m[label+'_aarch64_artifacts']=sum(x['configuration']==label for x in m['artifacts']);m[label+'_executables']=[p.name for p in binaries]
    symbols=subprocess.check_output([LLVM/'llvm-nm.exe','-C',a.build/'bo1_intern_strings_tests'],text=True)
    required=['Load_TempStringCustom(','Load_XStringCustom(','SL_GetString(','SL_ShutdownSystem(','MT_AllocIndex(','MT_FreeIndex(','DB_ExpandLoad_Ptr32Array(','Sys_EnterCriticalSection(']
    forbidden=['DB_ExpandLoad_XAssetArray(','Load_XAssetListCustom(','DB_InitZone(','Scr_ExecThread(','Scr_ClearScrVarDebugPub(']
    if any(s not in symbols for s in required) or any(s in symbols for s in forbidden):raise RuntimeError('Incorrect interning executable boundary')
    (out/'intern_strings_symbols.txt').write_text(symbols,encoding='utf-8');m['production_symbols_present']=required;m['later_symbols_absent']=forbidden
    for c in json.loads((a.build/'compile_commands.json').read_text(encoding='utf-8')):
        if '-DBO1_PORT_X64' in c['command']:raise RuntimeError('Android legacy selector enabled')
        p=Path(c['file']).resolve()
        if p.is_relative_to(ROOT):m['source_sha256'][p.relative_to(ROOT).as_posix()]=sha(p)
    for name in ['src/universal/critical_sections.h','src/platform/atomic32.h','src/platform/script_pool_services.h','src/clientscript/cscr_main.h','src/clientscript/cscr_memorytree.h','src/clientscript/cscr_stringlist.h','src/win32/win_common.h','tools/port/tests/string_pool_fixture.h','tools/port/run_string_pool_gate.py','tools/port/collect_string_pool_evidence.py','tools/port/verify_string_pool_source.py','tools/port/verify_raw_strings_source.py','tools/port/build_string_pool_host.ps1','tools/port/build_intern_strings_host.ps1']:m['source_sha256'][name]=sha(ROOT/name)
    m['host_fixture_provenance']='Full genuine run1 used Windows4 before test-only baseline-header extraction. Windows5 current-source rebuild/records match every prior fixture/release/summary; see rebuild-provenance.json. No current test SHA attributed to old Windows4 input.'
    m['scope']='Actual serial SL/MT lifecycle and genuine TempStringCustom; no full VM, threaded pool, async reader, payload dispatch or graphics'
    m['reader_boundary']='Borrowed finite decoded-source adapter with SHA-verified prefixes from prior complete native decode and independent full-stream oracle'
    m['diagnostics']='stringsUsed retained1 in collision fixture/13 in genuine file; MT/debug/hash ownership clears before reinit. avgAlloc diagnostic storage also retained; no RSS/leak claim.'
    m.update(final_debug_build=a.build.as_posix(),host_fixture_build=a.host.as_posix(),host_intern_build=a.intern_host.as_posix(),final_genuine_run=a.run.as_posix(),original_buffers_in_git=False)
    (out/'manifest.json').write_text(json.dumps(m,indent=2)+'\n',encoding='utf-8');print('Frozen',len(m['files']),'files;',m['debug_aarch64_artifacts'],'debug AArch64 artifacts;',m['optimized_aarch64_artifacts'],'optimized')
if __name__=='__main__':main()
