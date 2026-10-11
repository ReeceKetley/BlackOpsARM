"""Generate a focused executable using the real production water skinner."""
from pathlib import Path
import argparse
import subprocess

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--before', action='store_true')
parser.add_argument('--baseline', action='store_true')
args = parser.parse_args()
source = (root / 'src/gfx_d3d/r_water_sim.cpp').read_bytes().decode('latin1')

def extract(text, signature):
    start = text.index(signature)
    pos = text.index('{', start)
    depth, end = 1, pos + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

parts = []
for file, signature in [('src/universal/com_pack.h', 'union PackedUnitVec '),
                        ('src/universal/com_math.h', 'union alignas(16) float4 '),
                        ('src/gfx_d3d/r_water_sim.h', 'struct GfxWaterVertex '),
                        ('src/gfx_d3d/r_water_sim.h', 'struct tilemesh_t '),
                        ('src/gfx_d3d/r_water_sim.h', 'struct WaterSimulationCmdUpdate '),
                        ('src/gfx_d3d/r_water_sim.h', 'struct WaterSimulationCmdSkin '),
                        ('src/gfx_d3d/r_water_sim.h', 'struct WaterSimulationCmd //')]:
    parts.append(extract((root / file).read_bytes().decode('latin1'), signature) + ';')
test = (root / 'tools/port/test_water_skin.cpp').read_text()
prelude, rest = test.split('// PRODUCTION_TYPES', 1)
fixtures, body = rest.split('// PRODUCTION_FUNCTIONS', 1)
functions = [extract(source, 'static float4 WaterSwizzle('),
             source[source.index('float4 g_swizzleXZYW'):source.index('unsigned __int16 tessTable_0000')].strip(),
             extract(source, 'float4 *__cdecl ReadValue(')]
if args.before or args.baseline:
    functions += ['constexpr float4 g_zero = {{0.0f, 0.0f, 0.0f, 0.0f}};\n'
                  'float4 g_255 = {{255.0f, 255.0f, 255.0f, 255.0f}};\nusing vec3r = float *;',
                  extract((root / 'src/universal/com_math.cpp').read_bytes().decode('latin1'),
                          'float __cdecl Vec3NormalizeTo('),
                  extract(source, 'void __cdecl Float4LoadFloats(')]
    if args.before:
        functions.append((root / 'build/run/water_skin_before49.cpp').read_bytes().decode('latin1'))
    else:
        baseline = subprocess.check_output(['git', 'show', '17003a7:src/gfx_d3d/r_water_sim.cpp'], cwd=root).decode('latin1')
        functions.append(extract(baseline, 'void    R_WaterSimulationSkinMesh('))
else:
    functions += [extract(source, sig) for sig in ['static unsigned char WaterSkinPackByte(',
                                                  'static void WaterSkinNormalize(',
                                                  'void R_WaterSimulationSkinMesh(']]
suffix = '_baseline' if args.baseline else '_before' if args.before else ''
out = root / f'build/run/test_water_skin{suffix}.cpp'
out.write_text('\n\n'.join([prelude, *parts, fixtures, *functions, body]).replace('\r\n', '\n'))
print(out)
