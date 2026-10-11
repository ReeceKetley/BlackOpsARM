"""Report actual COMWORLD water metadata recorded by weapons.ps1 -WaterCells.

This is an asset inventory, not a rendering or gameplay gate.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path
root=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--prefix',default='x64hi_water55c');args=parser.parse_args()
maps=['zombie_theater','zombie_pentagon','zombie_cosmodrome','zombie_coast','zombie_temple','zombie_moon',
      'zombie_cod5_prototype','zombie_cod5_asylum','zombie_cod5_sumpf','zombie_cod5_factory']
rows=[]
for name in [f'{args.prefix}_{map}' for map in maps]+['x64hi_water55_zombie_temple','x64hi_water55b_zombie_temple','x64hi_water55d_zombie_cod5_prototype','w32_water55_zombie_cod5_prototype']:
    run=root/'build/run'/name
    if not run.exists():continue
    log=(run/'debug.log').read_text(errors='replace')
    captured=[]
    lines=log.splitlines()
    for index,line in enumerate(lines):
        match=re.match(r'^WATER_MAP_CELLS count=(\d+)[^\r\n]*?"(maps/[^"\r\n]+)"',line)
        if not match:continue
        asset=match[2];cursor=index+1
        # CDB da prints strings in32-character lines, each separately quoted.
        # Join only immediately adjacent string/address lines, not other logs.
        while cursor<len(lines):
            continuation=re.match(r'^[0-9a-fA-F`]+\s+"([^"\r\n]*)"\s*$',lines[cursor])
            if not continuation:break
            asset+=continuation[1];cursor+=1
        captured.append(dict(count=int(match[1]),asset=asset))
    exitfile=run/'exitcode.txt';exitcode=int(exitfile.read_text().strip()) if exitfile.exists() else None
    map=next(map for map in maps if name.endswith('_'+map))
    expected='maps/'+map+'.d3dbsp'
    matched=[item for item in captured if item['asset']==expected]
    row=dict(name=name,scope='asset metadata only',captures=captured,expectedAsset=expected,exitCode=exitcode,
             exeSha256=hashlib.sha256((run/'BO1Zombies.exe').read_bytes()).hexdigest(),
             debugLogSha256=hashlib.sha256((run/'debug.log').read_bytes()).hexdigest(),
             acceptedMetadata=bool(matched) and exitcode==0 and bool(re.search(r'Exit process .*code 0\b',log)),
             noSimulationCells=bool(matched) and all(item['count']==0 for item in matched))
    rows.append(row)
output=root/'build/run/water55_map_cells_results.json';output.write_text(json.dumps(rows,indent=2)+'\n')
for row in rows:print(row['name'],row['captures'],'accepted',row['acceptedMetadata'])
print(output)
