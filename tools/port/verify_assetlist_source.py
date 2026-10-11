"""Prove the moved declarations and production helper bodies are unchanged."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
ROOT=Path(__file__).resolve().parents[2]
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--baseline',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args();baseline=json.loads(args.baseline.read_text(encoding='utf-8'))
    def original(name):
        data=subprocess.check_output(['git','--no-optional-locks','show',baseline['BlackOpsARM']['head']+':'+name],cwd=ROOT)
        # Git's normalized LF blob versus this checkout's verified CRLF bytes.
        expanded=data.replace(b'\n',b'\r\n')
        if hashlib.sha256(expanded).hexdigest().upper()!=baseline['BlackOpsARM']['files'][name]:raise RuntimeError('Baseline source differs from recorded commit CRLF checkout')
        return expanded
    old=original('src/database/db_expand.cpp');new=(ROOT/'src/database/db_expand.cpp').read_bytes()
    start=b'static int g_expandReadAccum = 0;';end=b'void DB_ExpandLoad_XAssetArray('
    old_body=old[old.index(start):old.index(end)].rstrip()
    new_body=new[new.index(start):new.index(b'#if !defined(BO1_ASSETLIST_GATE)',new.index(start))].rstrip()
    if old_body!=new_body:raise RuntimeError('Production read/expansion/helper body changed')
    registry=original('src/database/db_registry.h');types=(ROOT/'src/database/db_assetlist_types.h').read_bytes()
    result={'schema':'BlackOpsARM.TASK-0013.source-preservation.v1','git_blob_checkout_eol':'LF -> CRLF verified against baseline SHA',
            'production_helper_envelope_bytes_unchanged':True,'helper_body_sha256':hashlib.sha256(old_body).hexdigest(),'declarations':{}}
    for name in ['XAsset','ScriptStringList','XAssetList']:
        pattern=rb'^struct '+name.encode()+rb'\b[^\n]*\n\{.*?^\};'
        a=re.search(pattern,registry,re.M|re.S).group();b=re.search(pattern,types,re.M|re.S).group()
        if a!=b:raise RuntimeError('Declaration changed: '+name)
        result['declarations'][name]={'exact_bytes_unchanged':True,'sha256':hashlib.sha256(a).hexdigest()}
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('PASS unchanged production helpers/envelope and3 declarations')
if __name__=='__main__':main()
