"""TASK38 full root wrapper; all TASK36/37 prerequisites retained."""
from pathlib import Path
from techset_oracle import reference as techset_reference
tech_extra='''            prior=result['oracles']['zombie_pentagon'];oracle=techset_reference(prior)
            prefix=f'{device}/techset.decoded' if on_device else str(out/(target+'-techset.decoded'))
            source=f'{old_device}/zombie_pentagon.ff' if on_device else prior['original_path']
            decoded=json.loads(execute('techset-decode',['--decode-techset',source,prefix]))
            exact_record(decoded,{'decoded_bytes':prior['decoded_bytes'],'decoded_adler32':prior['decoded_adler32'],'prefix_bytes':66226,'buffer_released':True},'techset native decode')
            digest=shell(target+'-techset-sha',['sha256sum',prefix]).split()[0] if on_device else sha(prefix)
            if digest!=oracle['prefix_sha256']:raise RuntimeError('Native prefix differs from independent complete retail streaming provenance')
            rows=[json.loads(line) for line in execute('techset',['--techset',prefix]).splitlines() if line.strip()]
            if len(rows)!=5:raise RuntimeError('Techset exact record count')
            width=4 if target=='windows_x86' else 8
            for i,row in enumerate(rows[:4]):
                exact_record(row,{'techset_cycle':i,'alloc_type':i%2,'source_start':65678,'header_end':66206,'source_end':66226,'name_bytes':20,'name_adler32':1243743876,'null_techniques':130,'registered_assets':2,'free_entry_nodes':32752,'prior_queries':1594,'native_header_bytes':528 if width==4 else 1056,'child_native_end':65632 if width==4 else 98940,'actual_root_wrapper':True,'actual_find':True,'temp_overwrite_survived':True},'techset cycle')
            exact_record(rows[-1],{'schema':'BlackOpsARM.TASK38.techset.v1','cycles':4,'pointer_bytes':width,'next_type':5,'engine_startup_executed':False},'techset terminal')
            result.setdefault('techset',{})[target]={'oracle':oracle,'native_decode':decoded,'native_prefix_sha256':digest,'records':rows}
'''
base=Path(__file__).with_name('run_code_zone_gate.py');program=base.read_text(encoding='utf-8-sig')
program=program.replace("base=Path(__file__).with_name('run_registry_add_gate.py');", "whole_extra+=tech_extra\nbase=Path(__file__).with_name('run_registry_add_gate.py');")
program=program.replace('code_zone.exe','techset.exe').replace('bo1_code_zone_tests','bo1_techset_tests')
program=program.replace('TASK-0037.code-zone-gate.v1','TASK-0038.techset-gate.v1')
exec(compile(program,str(base),'exec'),globals())
