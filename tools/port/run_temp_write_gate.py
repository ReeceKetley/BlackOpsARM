"""Run isolated TEMP write/mapping checks on Windows x86/x64 and HONOR ARM64."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import sys
import time
import traceback
import uuid

sys.dont_write_bytecode = True
from run_script_string_services import verify_pe
ROOT = Path(__file__).resolve().parents[2]


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host-build',type=Path,required=True)
    parser.add_argument('--android-build',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--iterations',type=int,default=3)
    parser.add_argument('--adb',type=Path,default=Path('D:/Android/Sdk/platform-tools/adb.exe'))
    parser.add_argument('--serial',default='AKWJ9X5116G04618')
    args=parser.parse_args()
    output=args.output.resolve()
    if not output.is_relative_to(ROOT/'build-android-audit') or output.exists() or not 1<=args.iterations<=20:
        raise RuntimeError('Use fresh ignored Android output and1..20 iterations')
    output.mkdir(parents=True)
    result=dict(schema='BlackOpsARM.TASK-0028.temp-write-gate.v1',status='running',
                recorded_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),argv=[sys.executable,*sys.argv],
                runner_sha256=sha(__file__),device_serial=args.serial,platforms={},
                read_failure_model='bounded controlled C++ throw adapter',production_longjmp_recovery_certified=False,
                scope='Scratch-owned actual stream cursors/aliases only; no production zone-allocation claim, genuine assets or payload children')
    invocations=[]
    environment=dict(os.environ)
    for key in ('BO1_STREAM_TRACE','BO1_TRACE_ZONE','BO1_TRACE_ASSET','BO1_TRACE_STACK_OP'):
        environment.pop(key,None)
    environment['BO1_DIAG_LOG']=str(output/'diagnostics.log')
    def save():
        (output/'results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
        (output/'invocations.json').write_text(json.dumps(invocations,indent=2)+'\n',encoding='utf-8')
    def run(label,argv,expected_exit=0):
        words=[str(word) for word in argv]
        record=dict(label=label,argv=words,cwd=str(output),timeout_seconds=60,exit=None,timed_out=False)
        start=time.monotonic();stdout=stderr=b''
        try:
            completed=subprocess.run(words,cwd=output,env=environment,capture_output=True,timeout=60)
            stdout,stderr=completed.stdout,completed.stderr;record['exit']=completed.returncode
        except subprocess.TimeoutExpired as error:
            stdout,stderr=error.stdout or b'',error.stderr or b'';record['timed_out']=True
        except OSError as error:
            stderr=str(error).encode();record['launch_error']=str(error)
        record['elapsed_seconds']=time.monotonic()-start
        (output/(label+'.stdout.log')).write_bytes(stdout)
        (output/(label+'.stderr.log')).write_bytes(stderr)
        invocations.append(record);save()
        exit_ok=(record['exit']==expected_exit) if expected_exit is not None else (record['exit'] is not None and record['exit']!=0)
        if not exit_ok or record['timed_out']:
            raise RuntimeError(label+': invocation failed; diagnostics retained')
        return stdout.decode('utf-8',errors='replace')
    adb=[args.adb,'-s',args.serial]
    def shell(label,words):
        return run(label,adb+['shell',' '.join(shlex.quote(str(word)) for word in words)])
    try:
        binaries={'windows_x86':(args.host_build/'x86/temp_write.exe').resolve(),
                  'windows_x64':(args.host_build/'x64/temp_write.exe').resolve(),
                  'honor_arm64':(args.android_build/'bo1_temp_write_tests').resolve()}
        verify_pe(binaries['windows_x86'],0x14c);verify_pe(binaries['windows_x64'],0x8664)
        with binaries['honor_arm64'].open('rb') as stream:elf=stream.read(64)
        if elf[:6]!=b'\x7fELF\x02\x01' or struct.unpack_from('<H',elf,18)[0]!=183:
            raise RuntimeError('Android executable must be little-endian ELF64 AArch64')
        manifests={'windows_x86':args.host_build/'x86/source-manifest.json',
                   'windows_x64':args.host_build/'x64/source-manifest.json',
                   'honor_arm64':args.android_build/'source-manifest.json'}
        result['source_manifests']={}
        for target,path in manifests.items():
            hashes=json.loads(path.read_text(encoding='utf-8-sig'))
            for name,digest in hashes.items():
                source_path=Path(name) if Path(name).is_absolute() else ROOT/name
                if sha(source_path).upper()!=digest.upper():
                    raise RuntimeError(target+': source currency mismatch '+name)
            result['source_manifests'][target]={'path':str(path.resolve()),'sha256':sha(path),'files':hashes}
        if run('device-state',adb+['get-state']).strip()!='device':raise RuntimeError('HONOR not online')
        result['device_metadata']=run('device-metadata',adb+['shell','getprop ro.product.model; getprop ro.product.device; getprop ro.build.version.sdk; uname -a'])
        device='/data/local/tmp/blackopsarm-temp-write-'+uuid.uuid4().hex
        result['device_directory']=device
        shell('device-create',['mkdir',device])
        run('device-push',adb+['push',binaries['honor_arm64'],device+'/probe'])
        shell('device-chmod',['chmod','700',device+'/probe'])
        if shell('device-sha',['sha256sum',device+'/probe']).split()[0]!=sha(binaries['honor_arm64']):
            raise RuntimeError('Pushed executable digest differs')
        for target,binary in binaries.items():
            pointer=4 if target=='windows_x86' else 8
            entry=result['platforms'][target]=dict(executable=str(binary),sha256=sha(binary),runs=[])
            for iteration in range(args.iterations):
                label=f'{target}-{iteration}'
                text=shell(label,[device+'/probe']) if target=='honor_arm64' else run(label,[binary])
                payload=json.loads(text)
                expected=dict(schema='BlackOpsARM.TASK-0028.temp-write.v1',pointer_bytes=pointer,
                              native64=pointer==8,cycles=6,cases=18,short_reads=12,matrix_cases=78 if pointer==8 else 0,
                              controlled_fixtures=True,genuine_assets_loaded=False,payload_children_loaded=False)
                if payload!=expected:raise RuntimeError(target+': prerequisite payload differs')
                entry['runs'].append(payload);save()
        for target in ('windows_x64','honor_arm64'):
            label=target+'-opaque-controls'
            argv=(adb+['shell',shlex.quote(device+'/probe')+' --opaque-controls']) if target=='honor_arm64' else [binaries[target],'--opaque-controls']
            focused=json.loads(run(label,argv))
            expected={'schema':'BlackOpsARM.TASK-0028.opaque-controls.v1','pointer_bytes':8,
                      'opaque_retained_tests':6,'latest_tombstone_tests':1,'c_string_tests':2,
                      'cleanup_scope':'controlled_C++_throw_adapter_only','production_longjmp_recovery_certified':False}
            if focused!=expected:raise RuntimeError(target+': focused ABI controls differ')
            result['platforms'][target]['opaque_controls']=focused
            for mode in ('--invalid-alias','--invalid-slot','--external-alias'):
                label=target+mode
                argv=(adb+['shell',shlex.quote(device+'/probe')+' '+mode]) if target=='honor_arm64' else [binaries[target],mode]
                run(label,argv,expected_exit=None)
                diagnostic=(output/(label+'.stderr.log')).read_text(encoding='utf-8',errors='replace')
                if 'Unsupported TEMP alias off=' not in diagnostic or 'Unexpected invalid alias success' in diagnostic:
                    raise RuntimeError(target+': required invalid-TEMP-alias diagnostic missing')
                if target=='honor_arm64' and invocations[-1]['exit']!=134:
                    raise RuntimeError('Invalid Android alias must fail with SIGABRT134')
                if target=='windows_x64' and invocations[-1]['exit']!=3:
                    raise RuntimeError('Invalid Windows alias must fail with CRT abort3')
                result['platforms'][target].setdefault('invalid_alias_failures',{})[mode]={'expected_failure':True,'exit':invocations[-1]['exit'],'diagnostic':diagnostic}
        if result['platforms']['windows_x64']['runs']!=result['platforms']['honor_arm64']['runs']:
            raise RuntimeError('Native64 platform contracts differ')
        result.update(status='passed',all_tests_completed=True,native64_contract_matches=True)
    except Exception as error:
        result.update(status='failed',all_tests_completed=False,error=str(error),traceback=traceback.format_exc())
    save();print(json.dumps(dict(status=result['status'],output=str(output))))
    return 0 if result['status']=='passed' else 1


if __name__=='__main__':raise SystemExit(main())
