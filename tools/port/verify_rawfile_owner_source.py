"""Exact TASK30 source selection proof against the verified TASK29 checkpoint."""
from pathlib import Path
import argparse, hashlib, json, subprocess
ROOT=Path(__file__).resolve().parents[2]
def norm(b): return b.replace(b'\r\n',b'\n')
def sha(b): return hashlib.sha256(b).hexdigest()
def old(p): return norm(subprocess.check_output(['git','--no-optional-locks','show','0a9dbfc:'+p],cwd=ROOT))
def once(b,a,c):
 if b.count(a)!=1: raise RuntimeError('Expected one exact source edit: '+repr(a[:80]))
 return b.replace(a,c,1)
def prove():
 report={'schema':'BlackOpsARM.TASK-0030.rawfile-owner-source.v1','parent':'0a9dbfc','files':{}}
 policy=b'#define POOLSIZE_RAWFILE          1024\n#define POOLSIZE_RAWFILE_MOD 2048 // mod: capacity for bo1_mod_zones; in use only with it on the command line (DB_InitXAssets)\n'
 for p in ['src/database/db_asset_pool.h','src/database/db_registry.cpp','src/database/db_assetnames.cpp']:
  b=old(p);expected=b
  if p.endswith('db_asset_pool.h'):
   expected=once(expected,b'#include <universal/com_stringtable.h>',b'#include <universal/com_stringtable.h>\n#include "db_rawfile_types.h"')
   expected=once(expected,b'template <typename T>\nunion',policy+b'\ntemplate <typename T>\nunion')
   expected+=b'\nextern XAssetPool<RawFile, POOLSIZE_RAWFILE_MOD> g_RawFilePool;\nint __cdecl DB_SizeofXAsset_RawFile_();\n'
  elif p.endswith('db_registry.cpp'):
   expected=once(expected,policy,b'')
   pool=b'XAssetPool<RawFile, POOLSIZE_RAWFILE_MOD>                   g_RawFilePool;\n'
   expected=once(expected,pool+b'\n#endif',b'\n#endif\n#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_RAWFILE_OWNER_GATE)\n'+pool+b'#endif')
  else:
   start=expected.index(b'#if !BO1_NATIVE_64BIT\nstatic_assert(sizeof(RawFile)');end=expected.index(b'\nint __cdecl DB_SizeofXAsset_LocalizeEntry_',start)
   body=expected[start:end]
   # Keep actual getter body and default definition ordering unchanged.
   expected=expected[:start]+b'#endif\n#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_RAWFILE_OWNER_GATE)\n'+body.rstrip(b'\n')+b'\n\n#endif\n#if !defined(BO1_STRINGTABLE_OWNER_GATE)\n\n'+expected[end:]
   a=expected.index(b'#if defined(BO1_STRINGTABLE_OWNER_GATE)\n  NULL,',expected.index(b'DB_GetXAssetSizeHandler'));z=expected.index(b'#else',a)
   slots=expected[a:z];assert slots.count(b'  NULL,')==37
   expected=expected[:a]+slots.rsplit(b'  NULL,',1)[0]+b'#if defined(BO1_RAWFILE_OWNER_GATE)\n  &DB_SizeofXAsset_RawFile_,\n#else\n  NULL,\n#endif\n'+expected[z:]
  actual=norm((ROOT/p).read_bytes())
  if actual!=expected:
   import difflib
   raise RuntimeError(p+' undocumented delta:\n'+''.join(difflib.unified_diff(expected.decode().splitlines(True),actual.decode().splitlines(True))))
  report['files'][p]={'exact_named_edits_only':True,'original_sha256':sha(b),'current_sha256':sha(actual),'bom_preserved':b.startswith(b'\xef\xbb\xbf')==actual.startswith(b'\xef\xbb\xbf')}
 for p in ['src/database/db_rawfile_types.h','src/database/db_load.cpp','src/database/db_stream.cpp','src/database/db_expand.cpp','src/database/db_stream_load.cpp','src/universal/com_stringtable.cpp','src/universal/com_stringtable_sort.cpp','src/universal/com_hash.cpp']:
  b=old(p);a=norm((ROOT/p).read_bytes());assert b==a,p
  report['files'][p]={'whole_source_unchanged':True,'sha256':sha(a)}
 report['status']='passed';return report
if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('--output',required=True);args=parser.parse_args();p=Path(args.output);p.parent.mkdir(parents=True,exist_ok=True);result=prove();p.write_text(json.dumps(result,indent=2));print(result['status'])
