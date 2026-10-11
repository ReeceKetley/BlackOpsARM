"""Exercise production StartStep/CheckUseSuccess with another gun raised."""
import argparse
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--baseline', action='store_true')
args = parser.parse_args()
source = (subprocess.check_output(['git', 'show', '1fca062:src/game_sp/g_sp_testplan.cpp'], cwd=root)
          if args.baseline else (root / 'src/game_sp/g_sp_testplan.cpp').read_bytes()).decode('latin1')

def extract(signature):
    start = source.index(signature)
    pos = source.index('{', start)
    depth, end = 1, pos + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

types = '\n'.join(extract(sig) + ';' for sig in ['enum StepKind', 'struct PlanStep', 'const PlanStep s_steps[]'])
test = (root / 'tools/port/test_ammo_target.cpp').read_text()
test = test.replace('// PRODUCTION_TYPES', types)
test = test.replace('// PRODUCTION_FUNCTIONS', '\n'.join(extract(sig) for sig in
                    ['void StartStep(', 'bool CheckUseSuccess(']))
suffix = '_baseline' if args.baseline else ''
out = root / f'build/run/test_ammo_target{suffix}.cpp'
out.write_text(test.replace('\r\n', '\n'))
print(out)
