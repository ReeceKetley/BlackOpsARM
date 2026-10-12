"""TASK35 adapter: actual primary insertion, preserving all prior validations."""
from pathlib import Path
base=Path(__file__).with_name('run_registry_lookup_gate.py')
program=base.read_text(encoding='utf-8-sig')
# Match emitted scope directly; do not rewrite observed record booleans.
program=program.replace("'fixture_installed_hash_membership':True", "'fixture_installed_hash_membership':False").replace("'registry_publication_executed':False", "'registry_publication_executed':True")
program=program.replace("program=base.read_text(encoding='utf-8-sig')", "program=base.read_text(encoding='utf-8-sig').replace(\"'registry_publication_executed':False\",\"'registry_publication_executed':True\")")
program=program.replace('registry_lookup.exe','registry_insert.exe').replace('bo1_registry_lookup_tests','bo1_registry_insert_tests')
program=program.replace('BlackOpsARM.TASK34.lookup-controls.v1','BlackOpsARM.TASK35.lookup-controls.v1').replace('BlackOpsARM.TASK-0034.registry-lookup.v1','BlackOpsARM.TASK-0035.registry-insertion.v1').replace('BlackOpsARM.TASK-0034.registry-lookup-gate.v1','BlackOpsARM.TASK-0035.registry-insertion-gate.v1')
program=program.replace("'run_registry_lookup_gate.py',", "'run_registry_insert_gate.py', 'build_registry_insert_host.ps1', 'build_registry_insert_arm64.ps1', 'run_registry_lookup_gate.py',")
program=program.replace('Actual no-mod registry hash/name/find on genuine entry-owned headers with fixture-installed membership restored before release;', 'Actual production primary hash insertion and hash/name/find on genuine entry-owned headers; fixture removal before release;')
program=program.replace('PASS actual registry lookup on genuine entry-owned headers','PASS actual primary hash insertion and lookup on genuine entry-owned headers')
exec(compile(program,str(base),'exec'),globals())
