"""Extract production tracking/teleport/expiry and compare local retail state."""
import argparse
import json
import struct
import subprocess
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--baseline',action='store_true');parser.add_argument('--distance-baseline',action='store_true');args=parser.parse_args()
subprocess.check_call([sys.executable,str(root/'tools/port/check_water_scheduler.py')])
source=(subprocess.check_output(['git','show','1fca062:src/gfx_d3d/r_water_sim.cpp'],cwd=root)
        if args.baseline else (root/'src/gfx_d3d/r_water_sim.cpp').read_bytes()).decode('latin1')
if args.distance_baseline:
    start=source.index('        // Retail sums X/Z/Y')
    end=source.index('        /* clamp to [0, 1] */',start)
    source=source[:start]+'        const float d = Vec3Distance(eyePos, data.oldEyePos);\n        t = d / config.gridScale;\n\n'+source[end:]
header=(root/'src/gfx_d3d/r_water_sim.h').read_bytes().decode('latin1')
def extract(text,sig):
    start=text.index(sig);pos=text.index('{',start);depth,end=1,pos+1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
prelude=(root/'build/run/test_water_scheduler.cpp').read_text().split('void __cdecl ExpireMesh(',1)[0]
prelude=prelude.replace('void R_WaterSimulationTrackView(const float *) { calls.push_back(2); }','')
prelude+='#include <cmath>\n'+extract(header,'struct waterconfig_t ')+';\nwaterconfig_t config;\n'
math=(root/'src/universal/com_math.cpp').read_bytes().decode('latin1')
prelude+=extract(math,'float __cdecl Vec3Length(').replace('const vec3r v','const float *v')+'\n'
prelude+=extract(math,'float __cdecl Vec3Distance(')+'\n'
functions='\n'.join(extract(source,sig) for sig in ['void __cdecl ExpireMesh(',
    'void __cdecl ExpireAllMeshes(', 'void __cdecl R_WaterSimulationTeleport(',
    'void R_WaterSimulationTrackView(const float *eyePos)'])
test=(root/'tools/port/test_water_track.cpp').read_text()
test=test.replace('// PRODUCTION_PRELUDE',prelude).replace('// PRODUCTION_FUNCTIONS',functions)
suffix='_distance_baseline' if args.distance_baseline else '_baseline' if args.baseline else ''
(root/f'build/run/test_water_track{suffix}.cpp').write_text(test.replace('\r\n','\n'))
oracle=json.loads((root/'build/run/water_track54.json').read_text())
assert oracle['exe_sha256']=='2ba4a04e81de2b60582da0e4489efde21e6cb07904a223bae633b6ccaebbf759'
with (root/'build/run/water_track54.bin').open('wb') as out:
    out.write(struct.pack('<8sI',b'BO1TRCK1',len(oracle['cases'])))
    def words(values): out.write(struct.pack('<'+'I'*len(values),*(int(v)&0xffffffff for v in values)))
    def floats(values):out.write(struct.pack('<'+'f'*len(values),*values))
    for case in oracle['cases']:
        inp,expected=case['input'],case['output'];out.write(inp['name'].encode().ljust(32,b'\0'))
        words(inp['grid']+inp['center']+[inp['rings']]);floats(inp['eye']+inp['old']+[inp['height'],inp['local'],inp['speed'],inp['scale']])
        words([expected['frame']]+expected['grid']+expected['center']+expected['local_bits']+expected['old_bits']+
              [expected['base_calls'],expected['expired_head'],expected['expired_tail'],len(expected['fills'])])
        words(expected['expired'])
        for fill in expected['fills']:words(fill)
        for tile in expected['tiles']:words(tile)
print(root/f'build/run/test_water_track{suffix}.cpp')
