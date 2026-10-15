"""Summarize saved sampled GPU pass/copy intervals without adding overlapping spans."""
import argparse
import json
from pathlib import Path
import re
import statistics
from collections import defaultdict

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('capture',type=Path,help='Observer JSON with adjacent -logcat.txt')
p.add_argument('--min-room-draws',type=int)
a=p.parse_args()
data=json.loads(a.capture.read_text(encoding='utf-8'))
threshold=a.min_room_draws if a.min_room_draws is not None else data.get('min_room_draws',900)
frames={int(row['frame']) for row in data['samples'] if row.get('draws',0)>=threshold}
groups=defaultdict(list)
for line in a.capture.with_name(a.capture.stem+'-logcat.txt').read_text(encoding='utf-8',errors='replace').splitlines():
    if 'native_pass_profile ' not in line: continue
    row=dict(re.findall(r'(\w+)=([^\s]*)',line))
    if int(row['frame']) not in frames: continue
    key=tuple(row[k] for k in ['pass','target','width','height','color','depth'])
    groups[key].append(row)
rows=[]
for key,values in groups.items():
    item=dict(zip(['pass','target','width','height','color','depth'],key))
    item['kind']={'-1':'colour copy','-2':'depth/stencil copy'}.get(item['target'],'render pass')
    for field in ['gpu_ms','draws']:
        series=[float(v[field]) for v in values]
        item[field]={'mean':statistics.mean(series),'median':statistics.median(series),'min':min(series),'max':max(series)}
    item['samples']=len(values);rows.append(item)
rows.sort(key=lambda r:r['gpu_ms']['mean'],reverse=True)
output=a.capture.with_name(a.capture.stem+'-gpu-summary.json')
assert not output.exists(),'Preserve prior analysis'
output.write_text(json.dumps({'source':str(a.capture),'min_room_draws':threshold,'frames':len(frames),
    'scope':'TOP/BOTTOM GPU intervals include dependency latency and may overlap; do not sum as independent work.',
    'passes':rows},indent=2)+'\n',encoding='utf-8')
for row in rows:
    print(f"{row['pass']:>2} {row['kind']:<20} {row['gpu_ms']['mean']:7.3f}ms {row['draws']['mean']:6.1f}draws {row['color']} -> {row['depth']} ({row['samples']} samples)")
