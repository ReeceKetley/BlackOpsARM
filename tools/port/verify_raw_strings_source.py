"""Verify production stream-loading bodies against the recorded checkpoint."""
from pathlib import Path
import argparse,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
def body(data,name):
    start=data.index(('void __cdecl '+name+'(').encode());opening=data.index(b'{',start);depth=1;end=opening+1
    while depth:
        if data[end:end+1]==b'{':depth+=1
        elif data[end:end+1]==b'}':depth-=1
        end+=1
    return data[start:end]
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--baseline',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
    baseline=json.loads(args.baseline.read_text(encoding='utf-8'));name='src/database/db_stream_load.cpp'
    old=subprocess.check_output(['git','--no-optional-locks','show',baseline['BlackOpsARM']['head']+':'+name],cwd=ROOT).replace(b'\n',b'\r\n')
    if hashlib.sha256(old).hexdigest().upper()!=baseline['BlackOpsARM']['files'][name]:raise RuntimeError('Recorded source differs from checkpoint checkout')
    new=(ROOT/name).read_bytes();result={'schema':'BlackOpsARM.TASK-0015.source-preservation.v1','checkpoint':baseline['BlackOpsARM']['head'],'bodies':{}}
    for fn in ['Load_Stream','Load_DelayStream','DB_ConvertOffsetToPointer','DB_ConvertOffsetToAlias','Load_XStringCustom','Load_TempStringCustom']:
        before,after=body(old,fn),body(new,fn)
        if before!=after:raise RuntimeError('Production body changed: '+fn)
        result['bodies'][fn]={'unchanged':True,'sha256':hashlib.sha256(after).hexdigest()}
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print('PASS6 unchanged production stream-loading bodies')
if __name__=='__main__':main()
