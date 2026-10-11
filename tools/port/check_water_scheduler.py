"""Extract production native water state, FIFO and complete render scheduler."""
import argparse
import json
import struct
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--baseline', action='store_true')
parser.add_argument('--fixture', type=Path, default=root / 'build/run/water_scheduler52.json')
args = parser.parse_args()
source = (subprocess.check_output(['git', 'show', '1fca062:src/gfx_d3d/r_water_sim.cpp'], cwd=root)
          if args.baseline else (root / 'src/gfx_d3d/r_water_sim.cpp').read_bytes()).decode('latin1')

def extract(text, signature):
    start = text.index(signature)
    pos = text.index('{', start)
    depth, end = 1, pos + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

header = (root / 'src/gfx_d3d/r_water_sim.h').read_bytes().decode('latin1').replace('\r\n', '\n')
types = []
for file, signature in [('src/gfx_d3d/r_gfx.h', 'union GfxColor '),
                        ('src/universal/com_pack.h', 'union PackedUnitVec '),
                        ('src/universal/com_math.h', 'union alignas(16) float4 ')]:
    types.append(extract((root / file).read_bytes().decode('latin1'), signature) + ';')
for signature in ['struct meshExpire_t ', 'struct WaterSimulationCmdUpdate ', 'struct GfxWaterVertex ',
                  'struct tilemesh_t ', 'struct WaterSimulationCmdSkin ', 'struct WaterSimulationCmd //',
                  'template <typename T, int SIZE>', 'struct tile_t ', 'struct debugpoint_t ',
                  'template <typename T>\nstruct __declspec(align(16)) channel_t',
                  'struct WaterSimInfo ', 'struct waterdata_t ']:
    types.append(extract(header, signature) + ';')
functions = '\n'.join(extract(source, signature) for signature in
    ['void __cdecl ExpireMesh(', 'unsigned int __cdecl AllocMeshVerts(',
     'unsigned int __cdecl NumMeshVertsFree(', 'void __cdecl FreeMeshVerts(',
     'void __cdecl R_WaterSimulationSwitchOff(',
     'void R_WaterSimulationRender(const float *eyePos, int time, unsigned int viewIndex)'])
test = (root / 'tools/port/test_water_scheduler.cpp').read_text()
test = test.replace('// PRODUCTION_TYPES', '\n'.join(types)).replace('// PRODUCTION_FUNCTIONS', functions)
suffix = '_baseline' if args.baseline else ''
(root / f'build/run/test_water_scheduler{suffix}.cpp').write_text(test.replace('\r\n', '\n'))

oracle = json.loads(args.fixture.read_text())
assert oracle['exe_sha256'] == '2ba4a04e81de2b60582da0e4489efde21e6cb07904a223bae633b6ccaebbf759'
with (root / 'build/run/water_scheduler53.bin').open('wb') as out:
    out.write(struct.pack('<8sI', b'BO1SCHD1', len(oracle['cases'])))
    def words(values):
        out.write(struct.pack('<' + 'I' * len(values), *(int(v) & 0xffffffff for v in values)))
    for case in oracle['cases']:
        inp, expected = case['input'], case['output']
        out.write(inp['name'].encode().ljust(32, b'\0'))
        words([inp[key] for key in ['enabled', 'cells', 'allocated', 'frame', 'delta', 'time',
               'previous', 'draw', 'sparse', 'rings', 'free_count', 'expiry']])
        words([expected[key] for key in ['frame', 'enabled', 'delta', 'free_count', 'expired_head', 'expired_tail']]
              + [len(expected['calls']), len(expected['commands'])])
        words(expected['free']); words(expected['expired'])
        words([{'config':1, 'track_view':2, 'render_model':3, 'worker_enqueue':4}[c] for c in expected['calls']])
        for cmd in expected['commands']:
            words(cmd['update'] + cmd['skin'] + [v for pair in cmd['meshes'] for v in pair])
        for tile in expected['tiles']:
            words([tile['last_update'], tile['head'], tile['tail']] + [v for mesh in tile['meshes'] for v in mesh])
print(root / f'build/run/test_water_scheduler{suffix}.cpp')
