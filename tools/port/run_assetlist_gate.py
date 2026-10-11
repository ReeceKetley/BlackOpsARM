"""Freeze independent prefix provenance and run genuine envelope-only gates."""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import os
import shlex
import struct
import subprocess
import zlib
from run_stream_init_gate import ASSETS, ROOT, sha

def reference(source, oracle):
    """Independent streaming system codec oracle; no full decoded buffer."""
    digest=hashlib.sha256(); adler=1; total=0; prefix=bytearray(); tail=bytearray()
    decoder=zlib.decompressobj()
    with source.open('rb') as f:
        raw=f.read(12)
        if raw[:8]!=b'IWffu100' or struct.unpack('<I',raw[8:])[0]!=473:
            raise RuntimeError('Unsupported original raw header')
        while chunk:=f.read(65536):
            if decoder.eof:
                tail.extend(chunk)
                continue
            output=decoder.decompress(chunk)
            digest.update(output); adler=zlib.adler32(output,adler); total+=len(output)
            if len(prefix)<52:prefix.extend(output[:52-len(prefix)])
            if decoder.eof:tail.extend(decoder.unused_data)
    if not decoder.eof or len(tail)>31 or any(tail):raise RuntimeError('Incomplete independent wrapped decode')
    if total!=oracle['decodedBytes'] or digest.hexdigest()!=oracle['decodedSHA256'] or adler!=int(oracle['decodedAdler32'],16):
        raise RuntimeError('Independent full decode differs from frozen oracle')
    if len(prefix)!=52 or list(struct.unpack('<9I',prefix[:36]))!=oracle['xFileFields']:
        raise RuntimeError('Independent prefix metadata mismatch')
    fields=struct.unpack('<iIiI',prefix[36:52])
    return {'prefix_hex':prefix.hex(),'prefix_sha256':hashlib.sha256(prefix).hexdigest(),
            'envelope_hex':prefix[36:52].hex(),'decoded_bytes':total,'decoded_sha256':digest.hexdigest(),
            'decoded_adler32':adler,'padding_bytes':len(tail),'string_count':fields[0],'strings_tag':fields[1],
            'asset_count':fields[2],'assets_tag':fields[3],'oracle_codec':'Python system zlib streaming independent full digest, no retained decoded buffer'}

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for name in ['android-build','host-build','output']:ap.add_argument('--'+name,type=Path,required=True)
    ap.add_argument('--serial',default='AKWJ9X5116G04618')
    ap.add_argument('--adb',type=Path,default=Path('D:/Android/Sdk/platform-tools/adb.exe'))
    ap.add_argument('--retail-root',type=Path,default=ROOT.parent/'BlackOps64/build/Release')
    args=ap.parse_args();out=args.output.resolve()
    if not out.is_relative_to(ROOT) or out.exists():raise RuntimeError('Fresh Android worktree output required')
    out.mkdir(parents=True);invocations=[];env=dict(os.environ)
    for key in ['BO1_STREAM_TRACE','BO1_TRACE_ZONE','BO1_TRACE_ASSET','BO1_TRACE_STACK_OP']:env.pop(key,None)
    env['BO1_DIAG_LOG']=str(out/'diagnostics.log')
    def run(label,argv):
        p=subprocess.run([str(a) for a in argv],cwd=out,env=env,capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=180)
        for stream in ['stdout','stderr']:(out/f'{label}.{stream}.log').write_text(getattr(p,stream),encoding='utf-8')
        invocations.append({'label':label,'argv':[str(a) for a in argv],'exit':p.returncode})
        (out/'invocations.json').write_text(json.dumps(invocations,indent=2)+'\n',encoding='utf-8')
        if p.returncode:raise RuntimeError(f'{label}: exit {p.returncode}; diagnostics retained')
        return p.stdout
    adb=[args.adb,'-s',args.serial]
    def shell(label,words):return run(label,adb+['shell',' '.join(shlex.quote(str(w)) for w in words)])
    device='/data/local/tmp/blackopsarm-phase1-assetlist-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S%f')
    shell('device-create',['mkdir',device])
    run('device-metadata',adb+['shell','getprop ro.product.model; getprop ro.build.version.sdk; uname -a; cat /proc/meminfo'])
    native=args.android_build.resolve()/'bo1_assetlist_tests'
    run('device-probe-push',adb+['push',native,device+'/probe']);shell('device-probe-chmod',['chmod','700',device+'/probe'])
    if shell('device-probe-sha',['sha256sum',device+'/probe']).split()[0]!=sha(native):raise RuntimeError('Device executable hash mismatch')
    result={'schema':'BlackOpsARM.TASK-0013.genuine-envelope.v1','recorded_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'device_serial':args.serial,'device_directory':device,'oracles':{},'platforms':{},
            'reader_boundary':'Actual native expander/Win32 raw envelope branch, finite checked fixture DB_LoadXFileData; engine async/refill reader not certified'}
    for asset,leaf in ASSETS.items():
        oracle_path=ROOT/'docs/port/evidence/arm64_inflate'/leaf;oracle=json.loads(oracle_path.read_text(encoding='utf-8'))
        source=args.retail_root/'zone/common'/f'{asset}.ff'
        if sha(source)!=oracle['fileSHA256']:raise RuntimeError('Original raw input mismatch')
        ref=reference(source,oracle);ref.update({'original_path':str(source.resolve()),'raw_sha256':sha(source),
                'oracle_path':oracle_path.relative_to(ROOT).as_posix(),'oracle_sha256':sha(oracle_path)})
        result['oracles'][asset]=ref
        run(asset+'-push',adb+['push',source,f'{device}/{asset}.ff'])
        if shell(asset+'-device-sha',['sha256sum',f'{device}/{asset}.ff']).split()[0]!=oracle['fileSHA256']:raise RuntimeError('Device raw input mismatch')
    (out/'independent-oracles.json').write_text(json.dumps(result['oracles'],indent=2)+'\n',encoding='utf-8')
    for target in ['windows_x86','windows_x64','honor_arm64']:
        on_device=target=='honor_arm64';binary=device+'/probe' if on_device else str((args.host_build/target.split('_')[1]/'assetlist.exe').resolve())
        def execute(label,params):return shell(target+'-'+label,[binary]+params) if on_device else run(target+'-'+label,[binary]+params)
        params=['gate'];decodes={}
        for asset,ref in result['oracles'].items():
            print(target,asset,'full native decode and envelope validation',flush=True)
            prefix=f'{device}/{asset}.prefix' if on_device else str(out/f'{target}-{asset}.prefix')
            source=f'{device}/{asset}.ff' if on_device else ref['original_path']
            dec=json.loads(execute(asset+'-decode',['decode',source,prefix]))
            if dec['prefix_hex']!=ref['prefix_hex'] or dec['decoded_bytes']!=ref['decoded_bytes'] or dec['decoded_adler32']!=ref['decoded_adler32'] or not dec['decoded_buffer_released']:
                raise RuntimeError('Native full decode/prefix differs from independent oracle')
            digest=shell(target+'-'+asset+'-prefix-sha',['sha256sum',prefix]).split()[0] if on_device else sha(prefix)
            if digest!=ref['prefix_sha256']:raise RuntimeError('Extracted prefix hash mismatch')
            decodes[asset]=dec;params += [prefix,str(ref['decoded_bytes'])]
        rows=[json.loads(line) for line in execute('envelope',params).splitlines()];summary=rows[-1]
        if summary['genuine_cycles_passed']!=12 or summary['fixtures_passed']<70 or not summary['logical_baseline_restored']:
            raise RuntimeError('Envelope lifecycle gate failed')
        for row in rows[:-1]:
            ref=result['oracles'][list(ASSETS)[row['asset_index']]]
            for key in ['string_count','strings_tag','asset_count','assets_tag']:
                if row[key]!=ref[key]:raise RuntimeError('Production fields differ from genuine disk bytes')
            for key,tag in [('native_strings_bits',ref['strings_tag']),('native_assets_bits',ref['assets_tag'])]:
                expected=tag if summary['pointer_bytes']==4 or tag not in [0xffffffff,0xfffffffe] else ((1<<64)-(1 if tag==0xffffffff else 2))
                if row[key]!=expected:raise RuntimeError('Pointer-tag widening mismatch')
            if row['read_calls']!=1 or row['consumed_bytes']!=16 or not all(row[k] for k in ['block_cursors_unchanged','persistent_alias_slots','reset_and_release_pass']):raise RuntimeError('Envelope consumption/cursor/lifetime mismatch')
        result['platforms'][target]={'binary_sha256':sha(native if on_device else binary),'fresh_decodes':decodes,'cycles':rows[:-1],'summary':summary}
        (out/'results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    for a,b in zip(result['platforms']['windows_x64']['cycles'],result['platforms']['honor_arm64']['cycles']):
        if a!=b:raise RuntimeError('Native64 production envelope contract mismatch')
    result['native64_contract_matches']=True
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('PASS genuine asset-list envelope:',out,flush=True)

if __name__=='__main__':main()
