"""Extract actual production samplers and native state for original comparison."""
import argparse
import subprocess
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--baseline',action='store_true');args=parser.parse_args()
subprocess.check_call([sys.executable,str(root/'tools/port/check_water_scheduler.py')])
def extract(text,sig):
    start=text.index(sig);pos=text.index('{',start);end=pos+1;depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
source=(subprocess.check_output(['git','show','1fca062:src/gfx_d3d/r_water_sim.cpp'],cwd=root)
        if args.baseline else (root/'src/gfx_d3d/r_water_sim.cpp').read_bytes()).decode('latin1')
header=(root/'src/gfx_d3d/r_water_sim.h').read_bytes().decode('latin1')
bsp=(root/'src/qcommon/com_bsp.h').read_bytes().decode('latin1')
prelude=(root/'build/run/test_water_scheduler.cpp').read_text().split('void __cdecl ExpireMesh(',1)[0]
types='struct ComPrimaryLight; struct ComBurnableCell;\n'+'\n'.join(extract(bsp,sig)+';' for sig in ['struct ComWaterHeader ', 'struct ComWaterCell ', 'struct ComBurnableHeader ', 'struct ComWorld '])
prelude=prelude.replace('struct { unsigned numWaterCells; } comWorld;',types+'\nComWorld comWorld;')
prelude+='#include <cmath>\n#define LOBYTE(x) ((unsigned char)(x))\n'+extract(header,'struct waterconfig_t ')+';\nwaterconfig_t config;\n'
prelude+='char R_WaterFindCells(int,int,ComWaterCell**); double R_WaterSimulationGetHeightGlobal(const float*);\n'
functions='\n'.join(extract(source,sig) for sig in ['char __cdecl R_WaterFindCells(', 'bool __cdecl R_WaterSimulationFillTile(',
    'char __cdecl ConvertWorldToGrid(', 'double __cdecl R_WaterSimulationGetHeightGlobal(', 'double __cdecl R_WaterSimulationGetBaseHeight('])
test=(root/'tools/port/test_water_sampling.cpp').read_text().replace('// PRODUCTION_PRELUDE',prelude).replace('// PRODUCTION_FUNCTIONS',functions)
suffix='_baseline' if args.baseline else ''
(root/f'build/run/test_water_sampling{suffix}.cpp').write_text(test)
print(root/f'build/run/test_water_sampling{suffix}.cpp')
