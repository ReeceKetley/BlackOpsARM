"""TASK30 adapter retaining the full strict TASK29 collector contract.
The additional ownership records are checked before the unchanged prerequisite
records are passed to the original validator. Raw output remains unfiltered.
"""
from pathlib import Path

base = Path(__file__).with_name('run_rawfile_payload_gate.py')
source = base.read_text(encoding='utf-8-sig')
source = source.replace('rawfile_payload.exe', 'rawfile_owner.exe').replace('bo1_rawfile_payload_tests', 'bo1_rawfile_owner_tests')
needle = "            fixtures, cycles, summaries, measurement_records, directory_records, header_records"
position = source.index(needle)
additional = '''            owner_rows = [row for row in rows if isinstance(row, dict) and any(key.startswith('rawfile_owner_') for key in row)]
            owner_fixture = [row['rawfile_owner_fixtures'] for row in owner_rows if set(row) == {'rawfile_owner_fixtures'}]
            owner_measurement = [row['rawfile_owner_measurements'] for row in owner_rows if set(row) == {'rawfile_owner_measurements'}]
            owner_release = [row['rawfile_owner_release'] for row in owner_rows if set(row) == {'rawfile_owner_release'}]
            for i, row in enumerate(rows):
                if 'rawfile_owner_fixtures' in row and (i == 0 or set(rows[i-1]) != {'owner_fixtures'} or set(rows[i+1]) != {'rawfile_fixtures'}):
                    raise RuntimeError('TASK30 pool fixture order differs')
                if 'rawfile_owner_measurements' in row and set(rows[i+1]) != {'rawfile_measurements'}:
                    raise RuntimeError('TASK30 genuine ownership measurement order differs')
                if 'rawfile_owner_release' in row and 'asset_index' not in rows[i+1]:
                    raise RuntimeError('TASK30 release must precede completed cycle')
            pb = 4 if target == 'windows_x86' else 8
            hb = 12 if pb == 4 else 24
            if len(owner_rows) != 14 or len(owner_fixture) != 1 or len(owner_measurement) != 1 or len(owner_release) != 12:
                raise RuntimeError('TASK30 missing/extra ownership evidence')
            exact_record(owner_fixture[0], {'active_sizes': [1, 2, 1024, 2048], 'allocated_nodes': 3075,
                'exhaustions': 4, 'invalid_caller_preconditions': 20, 'header_bytes': hb,
                'pool_bytes': pb + 2048 * hb, 'baseline_restored': True, 'lifo_verified': True,
                'full_registry_executed': False}, 'RawFile pool fixtures')
            exact_record(owner_measurement[0], {'header_bytes': hb, 'name_bytes': 17, 'buffer_bytes': 141,
                'temp_overwritten_bytes': hb, 'persistent_header_unchanged': True, 'children_unchanged': True,
                'first_table_queries': 400, 'full_registry_executed': False}, 'Genuine RawFile ownership')
            for i, record in enumerate(owner_release):
                exact_record(record, {'allocated': i % 2 == 0, 'baseline_restored': True,
                    'before_zone_release': True}, 'RawFile release cycle')
            result.setdefault('rawfile_ownership', {})[target] = {'fixtures': owner_fixture[0],
                'genuine': owner_measurement[0], 'releases': owner_release}
            rows = [row for row in rows if row not in owner_rows]
            if rows[-1].get('schema') != 'BlackOpsARM.TASK-0030.rawfile-owner.v1':
                raise RuntimeError('TASK30 terminal schema required')
            rows[-1]['schema'] = 'BlackOpsARM.TASK-0029.rawfile-payload.v1'
'''
source = source[:position] + additional + source[position:]
# The prerequisite result schema is retained as nested contract by the adapter.
source = source.replace("result = {'schema': 'BlackOpsARM.TASK-0029.rawfile-payload-gate.v1'", "result = {'schema': 'BlackOpsARM.TASK-0030.rawfile-owner-gate.v1'")
source = source.replace("'run_rawfile_payload_gate.py', 'rawfile_header_oracle.py'", "'run_rawfile_owner_gate.py', 'verify_rawfile_owner_source.py', 'build_rawfile_owner_host.ps1', 'build_rawfile_owner_arm64.ps1', 'run_rawfile_payload_gate.py', 'rawfile_header_oracle.py'")
source = source.replace('Completed StringTable ownership and actual next code_pre_gfx RawFile name/guarded len+1 buffer;', 'Actual RawFile pool1024/2048 init/alloc/clone/free; complete genuine name17/buffer141 lifetime through TEMP overwrite; preserved StringTable queries;')
source = source.replace('PASS genuine next RawFile name/len+1 buffer, deferred TechniqueSet boundary and preserved StringTable ownership/queries', 'PASS actual RawFile pool/clone/free and genuine child lifetime across TEMP overwrite with all prerequisite validations')
exec(compile(source, str(base), 'exec'), globals())
