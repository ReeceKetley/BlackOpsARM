"""Exercise actual PackCheck/InsertStep with a DEFCON reset during dependencies."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
baseline = '--baseline' in sys.argv
source = (subprocess.check_output(['git', 'show', '17003a7:src/game_sp/g_sp_testplan.cpp'], cwd=root)
          if baseline else (root / 'src/game_sp/g_sp_testplan.cpp').read_bytes()).decode('latin1')


def extract(signature):
    start = source.index(signature)
    pos = source.index('{', start)
    depth, end = 1, pos + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


test = (root / 'tools/port/test_pack_rearm.cpp').read_text()
prelude, body = test.split('// PRODUCTION_FUNCTIONS', 1)
out = root / ('build/run/test_pack_rearm_baseline.cpp' if baseline else 'build/run/test_pack_rearm.cpp')
out.write_text('\n\n'.join([prelude, extract('void InsertStep('), extract('void PackCheck('), body]).replace('\r\n', '\n'))
print(out)
