"""Prove the enum move and bound shared changes to fixed32 operations/includes."""
from pathlib import Path
import argparse,hashlib,json,re,subprocess
ROOT=Path(__file__).resolve().parents[2]
def functions(data):
    result={}
    pattern=rb'^(?:[\w:*]+\s+)+(?:__cdecl\s+)?([A-Za-z_]\w*)\([^;]*?\)\r?\n\{'
    for m in re.finditer(pattern,data,re.M):
        start=m.start();i=m.end();depth=1
        while depth:
            if data[i:i+1]==b'{':depth+=1
            elif data[i:i+1]==b'}':depth-=1
            i+=1
        result[m.group(1).decode()]=data[start:i]
    return result
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--baseline',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    b=json.loads(a.baseline.read_text(encoding='utf-8'));result={'schema':'BlackOpsARM.TASK-0017.source-preservation.v1','checkpoint':b['BlackOpsARM']['head'],'functions':{}}
    def original(name):
        data=subprocess.check_output(['git','show',b['BlackOpsARM']['head']+':'+name],cwd=ROOT).replace(b'\n',b'\r\n')
        if hashlib.sha256(data).hexdigest().upper()!=b['BlackOpsARM']['files'][name]:raise RuntimeError('Baseline source mismatch '+name)
        return data
    old=original('src/win32/win_common.h');start=old.index(b'enum CriticalSection : __int32');end=old.index(b'\r\n};',start)+4;enum=old[start:end]
    new=(ROOT/'src/universal/critical_sections.h').read_bytes()
    if enum not in new or (ROOT/'src/win32/win_common.h').read_bytes()!=old[:start]+b'#include <universal/critical_sections.h>'+old[end:]:raise RuntimeError('Enum move differs')
    result['critical_enum_exactly_moved']=True;result['critical_enum_sha256']=hashlib.sha256(enum).hexdigest()
    for name in ['src/clientscript/cscr_stringlist.cpp','src/clientscript/cscr_memorytree.cpp','src/clientscript/cscr_instance.cpp']:
        before=functions(original(name));after=functions((ROOT/name).read_bytes());assert before.keys()==after.keys(),name
        for fn,body in before.items():
            expected=body
            if name.endswith('cscr_stringlist.cpp'):
                for left,right in [(b'_InterlockedExchangeAdd(',b'bo1::atomic32::ExchangeAdd32('),(b'InterlockedCompareExchange(',b'bo1::atomic32::CmpExchange32('),(b'InterlockedIncrement(',b'bo1::atomic32::Increment32('),(b'_InterlockedDecrement(',b'bo1::atomic32::Decrement32(')]:expected=expected.replace(left,right)
                expected=expected.replace(b'volatile int Comperand;',b'std::uint32_t Comperand;').replace(b'volatile LONG Comperand;',b'std::uint32_t Comperand;').replace(b'Comperand = refStr->data;',b'Comperand = bo1::atomic32::AtomicLoad32(&refStr->data);').replace(b'(BYTE *)',b'(std::uint8_t *)').replace(b'(BYTE)',b'(std::uint8_t)')
            if expected!=after[fn]:raise RuntimeError('Unexpected body change '+name+':'+fn)
            result['functions'][name+':'+fn]={'unchanged':body==after[fn],'only_documented_fixed32_operations':expected==after[fn],'sha256':hashlib.sha256(after[fn]).hexdigest()}
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print('PASS exact enum move and',len(result['functions']),'bounded production bodies')
if __name__=='__main__':main()
