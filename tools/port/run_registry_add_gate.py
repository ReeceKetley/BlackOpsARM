"""TASK36 actual Add/full Link new-name registration in the genuine loader."""
from pathlib import Path
add_extra='''            add_rows=[row for row in rows if isinstance(row,dict) and 'registry_add_cycle' in row]
            def check_add(candidates):
                if len(candidates)!=12:raise RuntimeError('Exactly12 actual Add cycles required')
                for i,row in enumerate(candidates):
                    if set(row)!={'registry_add_cycle'}:raise RuntimeError('Extra registration keys')
                    exact_record(row['registry_add_cycle'],{'asset_index':i%2,'actual_add_calls':1 if i%2 else 2,
                        'engine_allocated_cloned_published':True,'returned_header_used_before_temp_pop':True,
                        'full_link_new_name_path':True,'fixture_unlink_before_free':True},'actual registration')
            check_add(add_rows)
            malformed=[add_rows[:-1],add_rows+[add_rows[-1]]]
            bad=json.loads(json.dumps(add_rows));bad[0]['registry_add_cycle']['actual_add_calls']=True;malformed.append(bad)
            bad=json.loads(json.dumps(add_rows));bad[0]['registry_add_cycle']['asset_index']=1;malformed.append(bad)
            for candidate in malformed:
                try:check_add(candidate)
                except RuntimeError:pass
                else:raise RuntimeError('Malformed Add records accepted')
            for i,row in enumerate(rows):
                if 'registry_add_cycle' in row and (i+1==len(rows) or 'registry_lookup_cycle' not in rows[i+1] or rows[i+1]['registry_lookup_cycle'].get('asset_index')!=row['registry_add_cycle']['asset_index']):raise RuntimeError('Add record must precede same-cycle lookup')
            fatal_label=target+'-registration-default-unavailable'
            try:execute('registration-default-unavailable',['--registration-default-unavailable'])
            except RuntimeError:
                invocation=invocations[-1]
                if invocation['label']!=fatal_label or invocation['exit']!=(134 if target=='honor_arm64' else 3) or invocation['timed_out'] or invocation['launch_error'] is not None:raise
            else:raise RuntimeError('Unavailable default registration unexpectedly succeeded')
            if (out/(fatal_label+'.stdout.log')).read_text() or (out/(fatal_label+'.stderr.log')).read_text()!='Explicit unsupported standalone registration dependency: DB_CreateDefaultEntry\\n':raise RuntimeError('Unexpected unavailable default output')
            result.setdefault('registration_parser_controls',{})[target]={'malformed_records_rejected':4,'actual_default_dependency_rejected':True}
            result.setdefault('registry_registration',{})[target]=[row['registry_add_cycle'] for row in add_rows]
            rows=[row for row in rows if row not in add_rows]
'''
base=Path(__file__).with_name('run_registry_lookup_gate.py');program=base.read_text(encoding='utf-8-sig')
program=program.replace("base=Path(__file__).with_name('run_entry_genuine_gate.py');", "lookup_extra=add_extra+lookup_extra\nbase=Path(__file__).with_name('run_entry_genuine_gate.py');")
program=program.replace("'fixture_installed_hash_membership':True", "'fixture_installed_hash_membership':False").replace("'registry_publication_executed':False", "'registry_publication_executed':True")
program=program.replace("program=base.read_text(encoding='utf-8-sig')", "program=base.read_text(encoding='utf-8-sig').replace(\"'registry_publication_executed':False\",\"'registry_publication_executed':True\")")
program=program.replace('registry_lookup.exe','registry_add.exe').replace('bo1_registry_lookup_tests','bo1_registry_add_tests')
program=program.replace('BlackOpsARM.TASK34.lookup-controls.v1','BlackOpsARM.TASK36.lookup-controls.v1').replace('BlackOpsARM.TASK-0034.registry-lookup.v1','BlackOpsARM.TASK-0036.registry-registration.v1').replace('BlackOpsARM.TASK-0034.registry-lookup-gate.v1','BlackOpsARM.TASK-0036.registry-registration-gate.v1')
program=program.replace("'run_registry_lookup_gate.py',", "'run_registry_add_gate.py', 'build_registry_add_host.ps1', 'build_registry_add_arm64.ps1', 'verify_registry_add_source.py', 'run_registry_lookup_gate.py',")
program=program.replace('Actual no-mod registry hash/name/find on genuine entry-owned headers with fixture-installed membership restored before release;', 'Actual DB_Add/full DB_Link new-name no-mod path allocates/clones/publishes genuine TEMP headers and returns persistent headers before normalization/Pop; fixture unlink before free;')
program=program.replace('PASS actual registry lookup on genuine entry-owned headers','PASS actual Add/full Link new-name genuine returned-header registration')
exec(compile(program,str(base),'exec'),globals())
