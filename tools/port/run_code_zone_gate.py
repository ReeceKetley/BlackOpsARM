"""TASK37 all genuine code zone assets, retaining TASK36 prerequisites."""
from pathlib import Path
from code_zone_oracle import inspect_code_zone
whole_extra='''            code_ref=result['oracles']['code_pre_gfx']
            oracle=inspect_code_zone(code_ref,4 if target=='windows_x86' else 8)
            full=f'{device}/code-zone.decoded' if on_device else str(out/(target+'-code-zone.decoded'))
            source=f'{old_device}/code_pre_gfx.ff' if on_device else code_ref['original_path']
            decoded=json.loads(execute('code-zone-decode',['--decode-code-zone',source,full]))
            exact_record(decoded,{'schema':'BlackOpsARM.TASK37.decode.v1','decoded_bytes':49593,'decoded_adler32':oracle['decoded_adler32'],'buffer_released':True},'whole code decode')
            digest=shell(target+'-code-zone-sha',['sha256sum',full]).split()[0] if on_device else sha(full)
            if digest!=oracle['decoded_sha256']:raise RuntimeError('Native full code scratch SHA differs')
            whole=[json.loads(line) for line in execute('code-zone',['--code-zone',full]).splitlines() if line.strip()]
            raw_rows=[row for row in whole if 'code_zone_rawfile' in row]
            cycle_rows=[row for row in whole if 'code_zone_cycle' in row]
            releases=[row for row in whole if 'rawfile_owner_release' in row]
            terminals=[row for row in whole if 'schema' in row]
            if len(raw_rows)!=144 or len(cycle_rows)!=4 or len(releases)!=4 or len(terminals)!=1 or len(whole)!=153:raise RuntimeError('Whole zone record inventory differs')
            def check_raw(candidates):
                if len(candidates)!=144:raise RuntimeError('Exactly144 genuine RawFile records required')
                for i,row in enumerate(candidates):
                    if set(row)!={'code_zone_rawfile'}:raise RuntimeError('Unexpected RawFile record fields')
                    expected=dict(oracle['rawfiles'][i%36]);expected['cycle']=i//36
                    exact_record(row['code_zone_rawfile'],expected,'whole-zone rawfile')
            check_raw(raw_rows)
            malformed=[raw_rows[:-1],raw_rows+[raw_rows[-1]]]
            bad=json.loads(json.dumps(raw_rows));bad[0]['code_zone_rawfile']['length']=True;malformed.append(bad)
            bad=json.loads(json.dumps(raw_rows));bad[0]['code_zone_rawfile']['directory_index']=2;malformed.append(bad)
            for candidate in malformed:
                try:check_raw(candidate)
                except RuntimeError:pass
                else:raise RuntimeError('Malformed whole zone records accepted')
            for i,row in enumerate(cycle_rows):
                if set(row)!={'code_zone_cycle'}:raise RuntimeError('Unexpected cycle wrapper fields')
                exact_record(row['code_zone_cycle'],{'cycle':i,'alloc_type':i%2,'registered_assets':37,'source_end':49593,'source_residue':0,'highwater':37,'free_entry_nodes':32752,'all_children_persistent':True,'actual_find_identity':True,'all400_queries_verified':True,'cleanup_before_zone_free':True},'whole-zone cycle')
                if set(releases[i])!={'rawfile_owner_release'}:raise RuntimeError('Unexpected release wrapper fields')
                exact_record(releases[i]['rawfile_owner_release'],{'allocated':True,'baseline_restored':True,'before_zone_release':True},'whole-zone release')
                block=whole[i*38:(i+1)*38]
                if block[:36]!=raw_rows[i*36:(i+1)*36] or block[36]!=releases[i] or block[37]!=row:raise RuntimeError('Whole zone cycle record placement differs')
            for key,replacement in [('registered_assets',True),('highwater',True),('all400_queries_verified',1)]:
                bad=dict(cycle_rows[0]['code_zone_cycle']);bad[key]=replacement
                expected=dict(cycle_rows[0]['code_zone_cycle'])
                try:exact_record(bad,expected,'malformed cycle')
                except RuntimeError:pass
                else:raise RuntimeError('Malformed nested cycle scalar accepted')
            bad=dict(releases[0]['rawfile_owner_release']);bad['allocated']=1
            try:exact_record(bad,releases[0]['rawfile_owner_release'],'malformed release')
            except RuntimeError:pass
            else:raise RuntimeError('Malformed nested release scalar accepted')
            exact_record(terminals[0],{'schema':'BlackOpsARM.TASK37.code-zone.v1','pointer_bytes':oracle['pointer_bytes'],'cycles':4,'assets_per_cycle':37,'decoded_bytes':49593,'engine_startup_executed':False},'whole-zone summary')
            if whole[-1]!=terminals[0]:raise RuntimeError('Whole zone terminal placement differs')
            result.setdefault('whole_code_zone',{})[target]={'oracle':oracle,'records':whole,'malformed_records_rejected':8,'native_decode':decoded,'native_decode_sha256':digest}
'''
base=Path(__file__).with_name('run_registry_add_gate.py');program=base.read_text(encoding='utf-8-sig')
program=program.replace("base=Path(__file__).with_name('run_registry_lookup_gate.py');", "add_extra=add_extra+whole_extra\nbase=Path(__file__).with_name('run_registry_lookup_gate.py');")
program=program.replace('registry_add.exe','code_zone.exe').replace('bo1_registry_add_tests','bo1_code_zone_tests')
program=program.replace('BlackOpsARM.TASK-0036.registry-registration-gate.v1','BlackOpsARM.TASK-0037.code-zone-gate.v1')
program=program.replace("'run_registry_add_gate.py',", "'run_code_zone_gate.py', 'code_zone_oracle.py', 'build_code_zone_host.ps1', 'build_code_zone_arm64.ps1', 'run_registry_add_gate.py',")
program=program.replace('PASS actual Add/full Link new-name genuine returned-header registration','PASS all37 genuine code zone assets registered with retained prerequisites')
exec(compile(program,str(base),'exec'),globals())
