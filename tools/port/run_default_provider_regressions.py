"""Complete MP/SP provider regressions with real default services selected."""
from pathlib import Path
base = Path(__file__).with_name('run_bootstrap_default_gate.py')
program = base.read_text(encoding='utf-8-sig')
program = program.replace('bootstrap_default.exe', 'default_resolution.exe').replace('bo1_bootstrap_default_tests', 'bo1_default_resolution_tests')
exec(compile(program, str(base), 'exec'), globals())
