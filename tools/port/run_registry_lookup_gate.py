"""TASK34 adapter retaining all genuine decode/entry ownership contracts."""
from pathlib import Path
lookup_extra='''            lookup_rows=[row for row in rows if isinstance(row,dict) and 'registry_lookup_cycle' in row]
            pb=4 if target=='windows_x86' else 8
            def check_lookup_records(candidates):
                if len(candidates)!=12:raise RuntimeError('Exactly12 genuine lookup cycles required')
                for i,row in enumerate(candidates):
                    if set(row)!={'registry_lookup_cycle'}:raise RuntimeError('Extra lookup keys')
                    exact_record(row['registry_lookup_cycle'],{'asset_index':i%2,'actual_name_queries':3 if i%2 else 6,
                        'fixture_installed_hash_membership':True,'actual_find_header_identity':True,
                        'ascii_case_variants_match':True,'miss_returns_null':True,'entry_bytes_restored':True,
                        'hash_membership_removed_before_free':True,'registry_publication_executed':False},'genuine lookup')
            check_lookup_records(lookup_rows)
            candidates=[lookup_rows[:-1],lookup_rows+[lookup_rows[-1]]]
            bad=json.loads(json.dumps(lookup_rows));bad[0]['registry_lookup_cycle']['actual_name_queries']=True;candidates.append(bad)
            bad=json.loads(json.dumps(lookup_rows));bad[0]['registry_lookup_cycle']['asset_index']=1;candidates.append(bad)
            rejected=0
            for candidate in candidates:
                try:check_lookup_records(candidate)
                except RuntimeError:rejected+=1
                else:raise RuntimeError('Malformed lookup record accepted')
            for i,row in enumerate(rows):
                if 'registry_lookup_cycle' in row and 'rawfile_owner_release' not in rows[i+1]:raise RuntimeError('Lookup verification must precede release')
            result.setdefault('registry_lookup',{})[target]={'cycles':[row['registry_lookup_cycle'] for row in lookup_rows],'malformed_records_rejected':rejected}
            control_text=execute('lookup-controls',['--lookup-controls'])
            controls=[json.loads(line) for line in control_text.splitlines() if line.strip()]
            if len(controls)!=1:raise RuntimeError('One lookup control summary required')
            exact_record(controls[0],{'schema':'BlackOpsARM.TASK34.lookup-controls.v1','pointer_bytes':pb,'hash_vectors':27,
                'bucket_collision_verified':True,'type_filter_before_header_access':True,'first_primary_match':True,
                'override_not_primary':True,'slash_hash_equal_lookup_miss':True,'deferred_count_hit_bypasses_endpoint':True,
                'ownership_baseline_restored':True,'registry_publication_executed':False},'lookup controls')
            fatal_label=target+'-deferred-unavailable'
            try:execute('deferred-unavailable',['--mod-deferred-unavailable'])
            except RuntimeError:
                invocation=invocations[-1]
                if invocation['label']!=fatal_label or invocation['exit']!=(134 if target=='honor_arm64' else 3) or invocation['timed_out'] or invocation['launch_error'] is not None:raise
            else:raise RuntimeError('Unsupported deferred endpoint unexpectedly returned success')
            if (out/(fatal_label+'.stdout.log')).read_text():raise RuntimeError('Unexpected deferred endpoint stdout')
            diagnostic=(out/(target+'-deferred-unavailable.stderr.log')).read_text()
            if diagnostic!='Explicit unsupported deferred mod-zone linking in standalone lookup gate\\n':raise RuntimeError('Missing/extra deferred unavailable diagnostics')
            result['registry_lookup'][target]['controls']=controls[0]
            rows=[row for row in rows if row not in lookup_rows]
            if rows[-1].get('schema')!='BlackOpsARM.TASK-0034.registry-lookup.v1':raise RuntimeError('TASK34 terminal required')
            rows[-1]['schema']='BlackOpsARM.TASK-0033.entry-owned-genuine.v1'
'''
base=Path(__file__).with_name('run_entry_genuine_gate.py');program=base.read_text(encoding='utf-8-sig')
program=program.replace('additional = entry_extra + additional','additional = lookup_extra + entry_extra + additional').replace('entry_genuine.exe','registry_lookup.exe').replace('bo1_entry_genuine_tests','bo1_registry_lookup_tests')
program=program.replace('BlackOpsARM.TASK-0033.entry-owned-genuine-gate.v1','BlackOpsARM.TASK-0034.registry-lookup-gate.v1')
program=program.replace("'run_entry_genuine_gate.py', 'verify_entry_genuine_source.py'", "'run_registry_lookup_gate.py', 'build_registry_lookup_host.ps1', 'build_registry_lookup_arm64.ps1', 'run_entry_genuine_gate.py', 'verify_entry_genuine_source.py'")
program=program.replace('Actual genuine RawFile/StringTable entry-owned headers via real allocation/clone/free before zone release; preserved children/TEMP/query behavior;', 'Actual no-mod registry hash/name/find on genuine entry-owned headers with fixture-installed membership restored before release;')
program=program.replace('PASS actual entry-owned genuine headers and zone lifetime with all prerequisite validations','PASS actual registry lookup on genuine entry-owned headers with all prerequisite validations')
exec(compile(program,str(base),'exec'),globals())
