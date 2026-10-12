"""TASK36/37 regressions with the new TechniqueSet compile selection enabled."""
from pathlib import Path
base=Path(__file__).with_name('run_code_zone_gate.py')
program=base.read_text(encoding='utf-8-sig').replace('code_zone.exe','techset.exe').replace('bo1_code_zone_tests','bo1_techset_tests')
exec(compile(program,str(base),'exec'),globals())
