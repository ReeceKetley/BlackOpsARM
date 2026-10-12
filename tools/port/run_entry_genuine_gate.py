"""TASK33 genuine entry-owned-header adapter retaining strict TASK30/29 oracles."""
from pathlib import Path
entry_extra='''            entry_rows=[row for row in rows if isinstance(row,dict) and 'entry_owner_cycle' in row]
            pb=4 if target=='windows_x86' else 8
            def check_entry_records(candidates):
                if len(candidates)!=12:raise RuntimeError('Exactly12 entry-owned cycles required')
                for i,row in enumerate(candidates):
                    record=row['entry_owner_cycle']
                    if set(row)!={'entry_owner_cycle'}:raise RuntimeError('Extra entry cycle keys')
                    exact_record(record,{'asset_index':i%2,'owned_entries_before_release':1 if i%2 else 2,
                        'high_count_after_release':0,'high_water_retained':2,'entry_stride':16 if pb==4 else 24,
                        'free_entry_nodes':32752,'hash_buckets_empty':True,'entries_freed_before_zone':True,
                        'entry_header_pairs_survived_temp':True,'registry_publication_executed':False},'genuine entry cycle')
            check_entry_records(entry_rows)
            candidates=[entry_rows[:-1],entry_rows+[entry_rows[-1]]]
            bad=json.loads(json.dumps(entry_rows));bad[0]['entry_owner_cycle']['owned_entries_before_release']=True;candidates.append(bad)
            bad=json.loads(json.dumps(entry_rows));bad[0]['entry_owner_cycle']['asset_index']=1;candidates.append(bad)
            rejected=0
            for candidate in candidates:
                try:check_entry_records(candidate)
                except RuntimeError:rejected+=1
                else:raise RuntimeError('Malformed TASK33 record accepted')
            result.setdefault('entry_record_parser_controls',{})[target]={'rejected_malformed_records':rejected}
            for i,row in enumerate(rows):
                if 'entry_owner_cycle' in row and (i==0 or 'rawfile_owner_release' not in rows[i-1] or 'asset_index' not in rows[i+1]):
                    raise RuntimeError('Entry release evidence must follow raw release and precede zone completion')
            result.setdefault('entry_owned_genuine',{})[target]=[row['entry_owner_cycle'] for row in entry_rows]
            rows=[row for row in rows if row not in entry_rows]
            if rows[-1].get('schema')!='BlackOpsARM.TASK-0033.entry-owned-genuine.v1':raise RuntimeError('TASK33 terminal required')
            rows[-1]['schema']='BlackOpsARM.TASK-0030.rawfile-owner.v1'
'''.replace('else8','else 8').replace('else2','else 2').replace('else24','else 24')
base=Path(__file__).with_name('run_rawfile_owner_gate.py')
program=base.read_text(encoding='utf-8-sig')
program=program.replace('source = base.read_text(encoding=\'utf-8-sig\')',"source = base.read_text(encoding='utf-8-sig').replace('rawfile_payload.exe','entry_genuine.exe').replace('bo1_rawfile_payload_tests','bo1_entry_genuine_tests')")
program=program.replace('source = source[:position] + additional + source[position:]','additional = entry_extra + additional\nsource = source[:position] + additional + source[position:]')
program=program.replace('BlackOpsARM.TASK-0030.rawfile-owner-gate.v1','BlackOpsARM.TASK-0033.entry-owned-genuine-gate.v1')
program=program.replace("'run_rawfile_owner_gate.py', 'verify_rawfile_owner_source.py'", "'run_entry_genuine_gate.py', 'verify_entry_genuine_source.py', 'build_entry_genuine_host.ps1', 'build_entry_genuine_arm64.ps1', 'run_rawfile_owner_gate.py', 'verify_rawfile_owner_source.py'")
program=program.replace('Actual RawFile pool1024/2048 init/alloc/clone/free; complete genuine name17/buffer141 lifetime through TEMP overwrite; preserved StringTable queries;', 'Actual genuine RawFile/StringTable entry-owned headers via real allocation/clone/free before zone release; preserved children/TEMP/query behavior;')
program=program.replace('PASS actual RawFile pool/clone/free and genuine child lifetime across TEMP overwrite with all prerequisite validations','PASS actual entry-owned genuine headers and zone lifetime with all prerequisite validations')
exec(compile(program,str(base),'exec'),globals())
