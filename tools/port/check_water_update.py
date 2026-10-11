"""Extract production channels/config/update for exact retail-reference tests."""
import argparse
import subprocess
import re
from pathlib import Path

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--baseline', action='store_true')
parser.add_argument('--config-baseline', action='store_true')
args = parser.parse_args()
source = (subprocess.check_output(['git', 'show', '1fca062:src/gfx_d3d/r_water_sim.cpp'], cwd=root)
          if args.baseline else (root / 'src/gfx_d3d/r_water_sim.cpp').read_bytes()).decode('latin1').replace('\r\n', '\n')

def extract(text, signature):
    start = text.index(signature)
    pos = text.index('{', start)
    depth, end = 1, pos + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

types = []
for file, sig in [('src/universal/com_pack.h', 'union PackedUnitVec '),
                  ('src/universal/com_math.h', 'union alignas(16) float4 '),
                  ('src/gfx_d3d/r_water_sim.h', 'struct waterconfig_t '),
                  ('src/gfx_d3d/r_water_sim.h', 'struct WaterSimulationCmdUpdate '),
                  ('src/gfx_d3d/r_water_sim.h', 'struct WaterSimulationCmdSkin '),
                  ('src/gfx_d3d/r_water_sim.h', 'struct WaterSimulationCmd //'),
                  ('src/gfx_d3d/r_water_sim.h', 'template <typename T>')]:
    text = (root / file).read_bytes().decode('latin1').replace('\r\n', '\n')
    if sig == 'template <typename T>': sig = 'template <typename T>\nstruct __declspec(align(16)) channel_t'
    types.append(extract(text, sig) + ';')
test = (root / 'tools/port/test_water_update.cpp').read_text()
test = test.replace('// PRODUCTION_TYPES', '\n'.join(types))
current = (root / 'src/gfx_d3d/r_water_sim.cpp').read_bytes().decode('latin1')
config_source = subprocess.check_output(['git', 'show', '1fca062:src/gfx_d3d/r_water_sim.cpp'], cwd=root).decode('latin1') if args.config_baseline else current
config_function = extract(config_source, 'void R_WaterSimulationUpdateConfig(')
names = sorted(set(re.findall(r'r_watersim_\w+', config_function)))
test = test.replace('// PRODUCTION_CONFIG_DVARS', '\n'.join(
    f'ValueDvar {name}_storage; auto *{name} = &{name}_storage;' for name in names))
test = test.replace('// PRODUCTION_CONFIG', config_function)
test = test.replace('// PRODUCTION_UPDATE', extract(source, 'void R_WaterSimulationUpdateFrame('))
suffix = '_config_baseline' if args.config_baseline else '_baseline' if args.baseline else ''
out = root / f'build/run/test_water_update{suffix}.cpp'
out.write_text(test.replace('\r\n', '\n'))
print(out)
