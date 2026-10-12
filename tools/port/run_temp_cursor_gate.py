"""Run isolated TEMP cursor/alias checks on Windows x86/x64 and HONOR ARM64."""
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
    parser.add_argument('--historical-android-build',type=Path)
    parser.add_argument('--historical-source-json',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--iterations',type=int,default=3)
    parser.add_argument('--adb',type=Path,default=Path('D:/Android/Sdk/platform-tools/adb.exe'))
    parser.add_argument('--serial',default='AKWJ9X5116G04618')
    args=parser.parse_args()
    output=args.output.resolve()
    if not output.is_relative_to(ROOT/'build-android-audit') or output.exists() or not 1<=args.iterations<=20:
        raise RuntimeError('Use fresh ignored Android output and1..20 iterations')
    output.mkdir(parents=True)
    result=dict(schema='BlackOpsARM.TASK-0027.temp-cursor-gate.v1',status='running',
                recorded_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),argv=[sys.executable,*sys.argv],
                runner_sha256=sha(__file__),device_serial=args.serial,platforms={},
                scope='Scratch-owned actual stream cursors/aliases only; no production zone-allocation claim, assets or conflicting-write map implementation')
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
        if record['exit']!=expected_exit or record['timed_out']:
            raise RuntimeError(label+': invocation failed; diagnostics retained')
        return stdout.decode('utf-8',errors='replace')
    adb=[args.adb,'-s',args.serial]
    def shell(label,words):
        return run(label,adb+['shell',' '.join(shlex.quote(str(word)) for word in words)])
    try:
        binaries={'windows_x86':(args.host_build/'x86/temp_cursor.exe').resolve(),
                  'windows_x64':(args.host_build/'x64/temp_cursor.exe').resolve(),
                  'honor_arm64':(args.android_build/'bo1_temp_cursor_tests').resolve()}
        verify_pe(binaries['windows_x86'],0x14c);verify_pe(binaries['windows_x64'],0x8664)
        with binaries['honor_arm64'].open('rb') as stream:elf=stream.read(64)
        if elf[:6]!=b'\x7fELF\x02\x01' or struct.unpack_from('<H',elf,18)[0]!=183:
            raise RuntimeError('Android executable must be little-endian ELF64 AArch64')
        if run('device-state',adb+['get-state']).strip()!='device':raise RuntimeError('HONOR not online')
        result['device_metadata']=run('device-metadata',adb+['shell','getprop ro.product.model; getprop ro.product.device; getprop ro.build.version.sdk; uname -a'])
        device='/data/local/tmp/blackopsarm-temp-cursor-'+uuid.uuid4().hex
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
                expected=dict(schema='BlackOpsARM.TASK-0027.temp-cursor.v1',pointer_bytes=pointer,
                              native64=pointer==8,cycles=8,cases=40,alias_checks=112 if pointer==8 else 0,
                              paired_restore=True,insert_pointer=True,no_write_maps_retained=True,reset=True,
                              production_zone_allocation_claimed=False,asset2_loaded=False)
                if payload!=expected:raise RuntimeError(target+': prerequisite payload differs')
                entry['runs'].append(payload);save()
        if result['platforms']['windows_x64']['runs']!=result['platforms']['honor_arm64']['runs']:
            raise RuntimeError('Native64 platform contracts differ')
        if args.historical_android_build:
            if not args.historical_source_json:
                raise RuntimeError('Historical control requires frozen source provenance')
            historical=(args.historical_android_build/'bo1_temp_cursor_tests').resolve()
            with historical.open('rb') as stream:header=stream.read(64)
            if header[:6]!=b'\x7fELF\x02\x01' or struct.unpack_from('<H',header,18)[0]!=183:
                raise RuntimeError('Historical control must be ELF64 AArch64')
            run('historical-push',adb+['push',historical,device+'/historical'])
            shell('historical-chmod',['chmod','700',device+'/historical'])
            if shell('historical-sha',['sha256sum',device+'/historical']).split()[0]!=sha(historical):
                raise RuntimeError('Historical executable SHA mismatch')
            original_source=json.loads(args.historical_source_json.read_text(encoding='utf-8'))
            if not original_source['commit'].startswith('9a8534d'):
                raise RuntimeError('Historical source revision differs')
            text=run('historical-negative',adb+['shell',shlex.quote(device+'/historical')],expected_exit=1)
            diagnostic=(output/'historical-negative.stderr.log').read_text(encoding='utf-8')
            if text.strip() or diagnostic.strip()!='FAIL TEMP cursor: paired shadow cursor':
                raise RuntimeError('Historical control did not expose the expected shadow-restore defect')
            result['historical_negative_control']=dict(expected_failure=True,exit=1,
                diagnostic=diagnostic.strip(),binary_sha256=sha(historical),source=original_source,
                source_proof_sha256=sha(args.historical_source_json),current_test_failure=False)
        result.update(status='passed',all_tests_completed=True,native64_contract_matches=True)
    except Exception as error:
        result.update(status='failed',all_tests_completed=False,error=str(error),traceback=traceback.format_exc())
    save();print(json.dumps(dict(status=result['status'],output=str(output))))
    return 0 if result['status']=='passed' else 1


if __name__=='__main__':raise SystemExit(main())
