"""TASK34 whole production-source proof: lookup bodies remain canonical."""
from pathlib import Path
import argparse, hashlib, json, subprocess
from verify_entry_alloc_source import endbody
ROOT=Path(__file__).resolve().parents[2]
PARENT='5d4d641'
def old(p):return subprocess.check_output(['git','show',PARENT+':'+p],cwd=ROOT).replace(b'\r\n',b'\n').decode('utf-8-sig')
def now(p):return (ROOT/p).read_text(encoding='utf-8-sig')
opening='#endif\n#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_REGISTRY_LOOKUP_GATE)\n'
closing='#endif\n#if !defined(BO1_STRINGTABLE_OWNER_GATE)\n'
def expose(s,token):
 a=s.index(token);b=endbody(s,a)
 return s[:a]+opening+s[a:b]+'\n'+closing+s[b:]
def prove():
 records={}
 for p,functions in [('src/database/db_registry.cpp',['unsigned int __cdecl DB_HashForName(','XAssetEntryPoolEntry *__cdecl DB_FindXAssetEntry(']),('src/database/db_assetnames.cpp',['const char *__cdecl DB_GetXAssetName('])]:
  s=old(p).replace('#include "db_entry_alloc_api.h"','#include "db_entry_alloc_api.h"\n#if defined(BO1_REGISTRY_LOOKUP_GATE)\n#include "db_registry_lookup_api.h"'+ ('\n#include <cctype>' if 'db_registry.cpp' in p else '')+'\n#endif',1)
  for token in functions:s=expose(s,token)
  if 'db_registry.cpp' in p:
   line='static int g_modDeferredCount; // slots MOD_DEFERRED_UNLINKED'
   s=s.replace(line,opening+'#if defined(BO1_REGISTRY_LOOKUP_GATE)\nint g_modDeferredCount; // slots MOD_DEFERRED_UNLINKED\n#else\n'+line+'\n#endif\n'+closing,1)
  if s!=now(p):raise RuntimeError('Undocumented production delta '+p)
  records[p]={'exact_selection_only':True,'sha256':hashlib.sha256((ROOT/p).read_bytes()).hexdigest(),'canonical_functions':functions}
 changed=subprocess.check_output(['git','diff','--name-only',PARENT,'--','src','tl','DemonWare'],cwd=ROOT).decode().splitlines()
 header='src/database/db_registry_lookup_api.h'
 if set(changed)-{header}!=set(records):raise RuntimeError('Unexpected production changes '+str(changed))
 new=subprocess.check_output(['git','ls-files','--others','--exclude-standard','--','src','tl','DemonWare'],cwd=ROOT).decode().splitlines()
 if set(new)-{header}:raise RuntimeError('Unexpected new production sources '+str(new))
 if not now('android/cmake/arm64-audit/CMakeLists.txt').startswith(old('android/cmake/arm64-audit/CMakeLists.txt')):raise RuntimeError('Existing CMake target changed')
 return {'schema':'BlackOpsARM.TASK34.source.v1','status':'passed','parent':PARENT,'production_files':records,'normal_mod_deferred_static_linkage_preserved':True,'default_mod_helper_body_preserved':True,'existing_targets_preserved':True,'new_header_sha256':hashlib.sha256((ROOT/header).read_bytes()).hexdigest(),'registry_publication_executed':False}
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--output',required=True);args=ap.parse_args();p=Path(args.output);p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(prove(),indent=2));print('TASK34 source proof passed')
