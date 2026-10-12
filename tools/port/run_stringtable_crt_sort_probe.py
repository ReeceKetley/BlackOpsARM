"""Compare platform std::qsort order to unchanged original executable sort.

Windows hosts run by default; --android explicitly runs the device probe.
Only numeric, SHA-validated genuine StringTable metadata is transported.
Duplicate-row comparisons use an independent lowercase-digest model, not
actual engine query execution. No FastFile allocation descriptors are made.
"""
import sys
sys.dont_write_bytecode=True
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import struct
import subprocess
import traceback

ROOT=Path(__file__).resolve().parents[2]
ASSETS=['code_pre_gfx','zombie_pentagon']


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def query_model(cells, indices, columns):
    result=[]
    for cell in cells:
        begin,end=0,len(indices)
        while begin<end:
            midpoint=(begin+end)//2
            if cells[indices[midpoint]]['signed_hash']>=cell['signed_hash']:end=midpoint
            else:begin=midpoint+1
        row=-1
        for position in range(begin,len(indices)):
            candidate=cells[indices[position]]
            if candidate['signed_hash']!=cell['signed_hash']:break
            if (candidate['index']%columns==cell['index']%columns
                    and candidate['string']['lowercase_sha256']==cell['string']['lowercase_sha256']):
                row=candidate['index']//columns;break
        result.append({'index':cell['index'],'hash':cell['signed_hash'],'first_hash_index':begin,'row':row})
    return result


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--build',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--lookup-results',type=Path,default=ROOT/'build-android-audit/stringtable_lookup_run1/results.json')
    ap.add_argument('--original-results',type=Path,default=ROOT/'build-android-audit/original_stringtable_reference_run3/results.json')
    ap.add_argument('--android',action='store_true')
    ap.add_argument('--serial',default='AKWJ9X5116G04618')
    ap.add_argument('--adb',type=Path,default=Path('D:/Android/Sdk/platform-tools/adb.exe'))
    args=ap.parse_args();output=args.output.resolve()
    if not output.is_relative_to(ROOT/'build-android-audit') or output.exists():raise RuntimeError('Use fresh ignored Android output')
    output.mkdir(parents=True)
    report={'schema':'BlackOpsARM.CRT-sort-research.v1','scope':'Standalone actual std::qsort; metadata-only duplicate-row model, no actual engine execution',
            'lookup_results_path':str(args.lookup_results.resolve()),'lookup_results_sha256':sha(args.lookup_results),
            'original_results_path':str(args.original_results.resolve()),'original_results_sha256':sha(args.original_results),
            'runner_source_sha256':sha(__file__),'probe_source_sha256':sha(ROOT/'tools/port/tests/stringtable_crt_sort_probe.cpp'),
            'platforms':{},'inputs':{},'android_executed':args.android}
    commands=[]

    def save(name,value):
        (output/name).write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8')

    def run(label,argv):
        argv=[str(word) for word in argv]
        try:
            completed=subprocess.run(argv,cwd=output,capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=45)
            stdout,stderr,code=completed.stdout,completed.stderr,completed.returncode
        except Exception as error:
            commands.append({'label':label,'argv':argv,'failure':str(error)})
            save('commands.json',commands);raise
        (output/(label+'.stdout.log')).write_text(stdout,encoding='utf-8')
        (output/(label+'.stderr.log')).write_text(stderr,encoding='utf-8')
        commands.append({'label':label,'argv':argv,'exit':code});save('commands.json',commands)
        if code:raise RuntimeError(label+' failed; logs retained')
        return stdout

    try:
        lookup=json.loads(args.lookup_results.read_text());original=json.loads(args.original_results.read_text())
        if (lookup['schema']!='BlackOpsARM.TASK-0023.stringtable-lookup-gate.v1' or lookup.get('all_tests_completed') is not True
                or original['schema']!='BlackOpsARM.TASK-0024.original-stringtable-reference.v1' or original['status']!='passed'
                or original['lookup_results_sha256']!=sha(args.lookup_results)):
            raise RuntimeError('Original/reference source provenance not verified')
        for asset in ASSETS:
            oracle=lookup['oracles'][asset];ref=original['assets'][asset]
            if any(ref[key]!=oracle[key] for key in ['raw_sha256','decoded_sha256','prefix_bytes','prefix_sha256']):
                raise RuntimeError('Genuine metadata digest differs from original reference')
            cells=oracle['stringtable_cells'];indices=oracle['stringtable_indices']['values'];columns=ref['column_count']
            packed=struct.pack('<'+'h'*len(indices),*indices)
            if (indices!=ref['original_indices'] or hashlib.sha256(packed).hexdigest()!=ref['original_indices_sha256']
                    or len(cells)!=ref['cell_count']):raise RuntimeError('Genuine numeric index/count mismatch')
            payload=b'STQP'+struct.pack('<II',columns,len(cells))+struct.pack('<'+'i'*len(cells),*(cell['signed_hash'] for cell in cells))+packed
            path=output/(asset+'.numeric');path.write_bytes(payload)
            expected=query_model(cells,ref['normalized_indices'],columns)
            original_queries=[{'index':row['index'],'hash':row['hash'],'first_hash_index':row['first_hash_index'],'row':row['source_row_model']}
                              for row in ref['normalized_queries']['query_records']]
            if expected!=original_queries:raise RuntimeError('Independent query model disagrees with original executable reference')
            report['inputs'][asset]={'path':str(path),'sha256':sha(path),'format':'STQP + uint32 columns/count + int32 hashes + int16 original indices, little-endian',
                                     'genuine_metadata':True,'not_fastfile_allocation_descriptors':True,'raw_sha256':oracle['raw_sha256'],
                                     'decoded_sha256':oracle['decoded_sha256'],'column_count':columns,'cell_count':len(cells),
                                     'normalized_original_sha256':ref['normalized_indices_sha256']}
        device='/data/local/tmp/blackopsarm-crt-sort-'+output.name
        adb=[str(args.adb),'-s',args.serial]
        native=args.build.resolve()/'arm64/crt_sort'
        android_commands=[adb+['shell','mkdir '+shlex.quote(device)],adb+['push',str(native),device+'/probe'],
                          adb+['shell','chmod 700 '+shlex.quote(device+'/probe')]]
        for asset in ASSETS:
            android_commands.extend([adb+['push',str(output/(asset+'.numeric')),device+'/'+asset+'.numeric'],
                                     adb+['shell','sha256sum '+shlex.quote(device+'/'+asset+'.numeric')],
                                     adb+['shell',shlex.quote(device+'/probe')+' '+shlex.quote(device+'/'+asset+'.numeric')]])
        save('android-commands.json',android_commands)
        report['android_device_directory']=device
        if args.android:
            run('android-create',android_commands[0]);run('android-push-probe',android_commands[1]);run('android-chmod',android_commands[2])
            actual=run('android-probe-sha',adb+['shell','sha256sum '+shlex.quote(device+'/probe')]).split()[0]
            if actual!=sha(native):raise RuntimeError('Device probe digest mismatch')
        for target in ['windows_x86','windows_x64']+(['honor_arm64'] if args.android else []):
            platform={};on_device=target=='honor_arm64'
            binary=native if on_device else args.build.resolve()/target.split('_')[1]/'crt_sort.exe'
            for asset in ASSETS:
                metadata=output/(asset+'.numeric')
                if on_device:
                    run(target+'-'+asset+'-push',adb+['push',str(metadata),device+'/'+asset+'.numeric'])
                    if run(target+'-'+asset+'-sha',adb+['shell','sha256sum '+shlex.quote(device+'/'+asset+'.numeric')]).split()[0]!=sha(metadata):raise RuntimeError('Device metadata digest mismatch')
                    argv=adb+['shell',shlex.quote(device+'/probe')+' '+shlex.quote(device+'/'+asset+'.numeric')]
                else:argv=[binary,metadata]
                row=json.loads(run(target+'-'+asset,argv));ref=original['assets'][asset];oracle=lookup['oracles'][asset]
                keys={'schema','pointer_bytes','column_count','cell_count','array_address','array_above_u32','element_bytes','signed_hash_compare','equal_key_returns_zero','actual_std_qsort','sorted_indices'}
                if (set(row)!=keys or row['schema']!='BlackOpsARM.CRT-sort-probe.v1' or row['pointer_bytes']!=(4 if target=='windows_x86' else 8)
                        or row['column_count']!=ref['column_count'] or row['cell_count']!=ref['cell_count'] or row['element_bytes']!=2
                        or any(row[key] is not True for key in ['signed_hash_compare','equal_key_returns_zero','actual_std_qsort'])
                        or type(row['array_address']) is not int or row['array_above_u32'] is not (row['array_address']>0xffffffff)
                        or sorted(row['sorted_indices'])!=list(range(ref['cell_count']))):raise RuntimeError('Actual CRT probe contract failed')
                indices=row['sorted_indices'];cells=oracle['stringtable_cells'];columns=ref['column_count']
                if any((cells[a]['signed_hash'],a%columns)>(cells[b]['signed_hash'],b%columns) for a,b in zip(indices,indices[1:])):raise RuntimeError('CRT output not ordered by original comparator')
                actual_queries=query_model(cells,indices,columns);original_queries=query_model(cells,ref['normalized_indices'],columns)
                row.update({'sorted_indices_sha256':hashlib.sha256(struct.pack('<'+'h'*len(indices),*indices)).hexdigest(),
                            'normalized_order_matches_original':indices==ref['normalized_indices'],
                            'positions_differing_from_original':sum(a!=b for a,b in zip(indices,ref['normalized_indices'])),
                            'query_model_records':actual_queries,'query_model_row_misses':sum(query['row']<0 for query in actual_queries),
                            'query_model_rows_match_original':all(a['row']==b['row'] for a,b in zip(actual_queries,original_queries)),
                            'query_model_row_differences':[{'index':a['index'],'platform_row':a['row'],'original_row':b['row']}
                                                         for a,b in zip(actual_queries,original_queries) if a['row']!=b['row']]})
                platform[asset]=row
            report['platforms'][target]={'binary_sha256':sha(binary),'assets':platform};save('results.json',report)
        report['host_tests_completed']=True;report['all_tests_completed']=args.android
        report['android_pending']=not args.android
        report['conclusion']='Actual CRT order/query-model comparison measured; differing ties are retained evidence, not probe failure.'
        save('results.json',report);print('PASS standalone CRT research; Android pending:',not args.android,output)
    except Exception as error:
        save('failure.json',{'error_type':type(error).__name__,'error':str(error),'traceback':traceback.format_exc()});save('results.json',report);raise


if __name__=='__main__':main()
