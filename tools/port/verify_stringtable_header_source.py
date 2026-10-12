"""Prove standalone selection leaves every production expander body unchanged."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
from verify_string_pool_source import functions
ROOT = Path(__file__).resolve().parents[2]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--baseline',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args()
    baseline=json.loads(args.baseline.read_text(encoding='utf-8'))
    result={'schema':'BlackOpsARM.TASK-0018.source-preservation.v1',
            'checkpoint':baseline['BlackOpsARM']['head'],'functions':{},'declarations':{}}
    def original(name):
        data=subprocess.check_output(['git','--no-optional-locks','show',result['checkpoint']+':'+name],cwd=ROOT)
        data=data.replace(b'\n',b'\r\n')
        if hashlib.sha256(data).hexdigest().upper()!=baseline['BlackOpsARM']['files'][name]:
            raise RuntimeError('Baseline checkout differs: '+name)
        return data
    name='src/database/db_expand.cpp'
    before,after=functions(original(name)),functions((ROOT/name).read_bytes())
    if before.keys()!=after.keys():
        raise RuntimeError('Production expander function inventory differs')
    for fn,body in before.items():
        if body!=after[fn]:
            raise RuntimeError('Production expander body changed: '+fn)
        result['functions'][fn]={'unchanged':True,'sha256':hashlib.sha256(body).hexdigest()}
    for name in ['src/universal/com_stringtable.h','src/database/db_assetlist_types.h']:
        old,new=original(name),(ROOT/name).read_bytes()
        if old!=new:
            raise RuntimeError('Production declaration changed: '+name)
        result['declarations'][name]={'unchanged':True,'sha256':hashlib.sha256(new).hexdigest()}
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('PASS',len(before),'unchanged production expander bodies and2 unchanged headers')
if __name__=='__main__':
    main()
