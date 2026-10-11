"""Extract production water surface allocation/rendering and channel lookup for testing."""
from pathlib import Path
import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--baseline', action='store_true')
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (subprocess.check_output(['git', 'show', '17003a7:src/gfx_d3d/r_water_sim.cpp'], cwd=root)
          if args.baseline else (root / 'src/gfx_d3d/r_water_sim.cpp').read_bytes()).decode('latin1')


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
                        ('src/gfx_d3d/rb_tess.h', 'struct GfxModelWaterSurface '),
                        ('src/gfx_d3d/r_water_sim.h', 'struct tilemesh_t '),
                        ('src/gfx_d3d/r_water_sim.h', 'struct WaterSimulationCmdUpdate '),
                        ('src/gfx_d3d/r_water_sim.h', 'struct WaterSimulationCmdSkin '),
                        ('src/gfx_d3d/r_water_sim.h', 'struct WaterSimulationCmd //'),
                        ('tl/jobqueue/jobqueue_all.h', 'struct __declspec(align(8)) jqWorkerCmd ')]:
    parts.append(extract((root / file).read_bytes().decode('latin1'), signature) + ';')
test = (root / 'tools/port/test_water_native.cpp').read_text()
prelude, fixtures = test.split('// PRODUCTION_TYPES', 1)
swizzle = (extract(source, "static float4 WaterSwizzle(") + "\n" if not args.baseline else "")
swizzle += source[source.index("float4 g_swizzleXZYW"):source.index("unsigned __int16 tessTable_0000")].strip()
fixtures = fixtures.replace("// PRODUCTION_SWIZZLE", swizzle)
worker_source = (subprocess.check_output(['git', 'show', '17003a7:src/gfx_d3d/r_workercmds.cpp'], cwd=root)
                 if args.baseline else (root / 'src/gfx_d3d/r_workercmds.cpp').read_bytes()).decode('latin1')
worker = next(line for line in worker_source.splitlines() if line.startswith('jqWorkerCmd r_water_simWorkerCmd ='))
fixtures = fixtures.replace('// PRODUCTION_WORKER', worker)
fixtures, body = fixtures.split('// PRODUCTION_FUNCTIONS', 1)
functions = [extract(source, sig) for sig in ['void __cdecl R_RenderWaterModel(',
                                             'char *__cdecl R_SurfaceAlloc(',
                                             'float4 *__cdecl ReadValue(']]
queue_source = (root / 'src/universal/com_workercmds.cpp').read_bytes().decode('latin1')
functions += [extract(queue_source, sig) for sig in ['unsigned __int8 *__cdecl jqCommitMemory(',
                                                     'jqWorkerCmd *jqGetWorkercmdParam(',
                                                     'void *__cdecl jqLockData(']]
out = root / ('build/run/test_water_native_baseline.cpp' if args.baseline
              else 'build/run/test_water_native.cpp')
out.write_text('\n\n'.join([prelude, *parts, fixtures, *functions, body]).replace('\r\n', '\n'))
print(out)
