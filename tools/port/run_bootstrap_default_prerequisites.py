"""Retain actual code-zone/registration checks with bootstrap graph selection."""
from pathlib import Path
base = Path(__file__).with_name('run_code_zone_gate.py')
program = base.read_text(encoding='utf-8-sig')
program = program.replace('code_zone.exe', 'bootstrap_default.exe').replace('bo1_code_zone_tests', 'bo1_bootstrap_default_tests')
exec(compile(program, str(base), 'exec'), globals())
