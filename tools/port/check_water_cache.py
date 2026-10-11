"""Exercise the complete production channel/cache template on both pointer widths."""
import argparse
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--baseline', action='store_true')
parser.add_argument('--halo-baseline', action='store_true')
args = parser.parse_args()
source = (subprocess.check_output(['git', 'show', '1fca062:src/gfx_d3d/r_water_sim.h'], cwd=root)
          if args.baseline else (root / 'src/gfx_d3d/r_water_sim.h').read_bytes()).decode('latin1').replace('\r\n', '\n')
if args.halo_baseline:
    # Reproduce batch50's remaining halo placement defect with fixed alignment.
    source = source.replace('T pad0[16 / sizeof(T)];', 'T pad0;').replace(
        'c->pad0[16 / sizeof(T) - 1]', 'c->pad0')
start = source.index('template <typename T>\nstruct __declspec(align(16)) channel_t')
pos = source.index('{', start)
depth, end = 1, pos + 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
test = (root / 'tools/port/test_water_cache.cpp').read_text()
test = test.replace('// PRODUCTION_CHANNEL', source[start:end] + ';')
suffix = '_halo_baseline' if args.halo_baseline else '_baseline' if args.baseline else ''
out = root / f'build/run/test_water_cache{suffix}.cpp'
out.write_text(test.replace('\r\n', '\n'))
print(out)
