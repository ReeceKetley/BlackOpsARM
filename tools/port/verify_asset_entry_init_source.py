"""Exact TASK31 canonical declaration/init extraction and native DBHASH proof."""
from pathlib import Path
import argparse,hashlib,json,subprocess
from verify_temp_cursor_source import function,lf
ROOT=Path(__file__).resolve().parents[2]
def old(p):return lf(subprocess.check_output(['git','--no-optional-locks','show','63c2fa4:'+p],cwd=ROOT))
def now(p):return lf((ROOT/p).read_bytes())
def once(b,a,c):
 if b.count(a)!=1:raise RuntimeError('Expected exactly one edit '+repr(a[:80]))
 return b.replace(a,c,1)
def same(p,b,report):
 if now(p)!=b:
  import difflib
  raise RuntimeError(p+' undocumented source change\n'+''.join(difflib.unified_diff(b.decode().splitlines(True),now(p).decode().splitlines(True))))
 report[p]={'exact_named_edits_only':True,'sha256':hashlib.sha256(now(p)).hexdigest()}
def prove():
 files={};p='src/database/db_registry.h';b=old(p);a=b.index(b'struct XAssetEntry //');z=b.index(b'struct DBReorderAssetEntry',a);entry=b[a:z];a2=b.index(b'union XAssetEntryPoolEntry //');z2=b.index(b'struct XZone //',a2);pool=b[a2:z2];expected=b[:a2]+b[z2:];expected=expected[:a]+b'#include "db_asset_entry_types.h"\n\n'+expected[z:];same(p,expected,files)
 light=now('src/database/db_asset_entry_types.h');prefix=b'#pragma once\n#include "db_assetlist_types.h"\n#include <cstddef>\n#include <cstdint>\n\n'+entry+pool
 if not light.startswith(prefix) or light.count(entry)!=1 or light.count(pool)!=1:raise RuntimeError('Canonical declarations differ/duplicated')
 # Assert the moved definitions each occur only once across the database headers.
 for token in [b'struct XAssetEntry //',b'union XAssetEntryPoolEntry //']:
  if sum(lf(h.read_bytes()).count(token) for h in (ROOT/'src/database').glob('*.h'))!=1:raise RuntimeError('Nonunique canonical definition')
 files['src/database/db_asset_entry_types.h']={'canonical_definition_bytes_unchanged':True,'sha256':hashlib.sha256(light).hexdigest()}
 p='src/database/db_registry.cpp';b=old(p);expected=once(b,b'#include "db_asset_pool.h"',b'#include "db_asset_pool.h"\n#include "db_asset_entry_types.h"')
 guard=b'#endif\n#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_ASSET_ENTRY_GATE)\n';closing=b'#endif\n#if !defined(BO1_STRINGTABLE_OWNER_GATE)\n'
 globals=b'unsigned __int16 db_hashTable[0x8000/*32768*/];\nXAssetEntryPoolEntry g_assetEntryPool[65536]; // mod: 32768 in the exe; the upper half is chained only with bo1_mod_zones (DB_InitXAssets)\n'
 expected=once(expected,globals,guard+globals+closing);head=b'XAssetEntryPoolEntry *g_freeAssetEntryHead;\n';expected=once(expected,head,guard+head+closing)
 clone=function(b,'DB_CloneXAssetInternal')+b'\n';expected=once(expected,clone,b'#if !defined(BO1_ASSET_ENTRY_GATE)\n'+clone+b'#endif\n')
 init=function(b,'DB_Init');start=init.index(b'    g_freeAssetEntryHead = g_assetEntryPool + 16;');tail=init[start:-2]
 # Keep all actual assignments/loop bounds/comments; only supply the existing mode decision.
 helper=tail.replace(b'I_stristr(GetCommandLineA(), "bo1_mod_zones") ? 0xFFFF : 0x7FFF',b'modZones ? 0xFFFF : 0x7FFF')
 changed=init[:start]+b'    DB_InitAssetEntryPool(I_stristr(GetCommandLineA(), "bo1_mod_zones") != NULL);\n}'
 changed=once(changed,b'    unsigned int i; // [esp+4h] [ebp-4h]\n\n',b'')
 expected=once(expected,init,changed)
 anchor=b'\nvoid __cdecl DB_InitPoolHeader(XAssetType type)';new=b'\n'+guard+b'void DB_InitAssetEntryPool(bool modZones)\n{\n    unsigned int i; // existing DB_Init freelist loop\n'+helper+b'\n}\n'+closing
 expected=once(expected,anchor,new+anchor);same(p,expected,files)
 p='src/platform/android/script_string_services.cpp';expected=old(p).replace(b'    fail("invalid lock",',b'    if (lock == bo1::script::PoolLock::Database) return 2;\n    fail("invalid lock",').replace(b'sections[2]',b'sections[3]');same(p,expected,files)
 p='src/platform/android/script_string_services.h';same(p,once(old(p),b'PoolLock { Strings, MemoryTree }',b'PoolLock { Strings, MemoryTree, Database }'),files)
 p='src/platform/android/script_pool_bridge.cpp';expected=old(p).replace(b'// Bounded real SL/MT platform bridge: only the two production pool sections',b'// Bounded real SL/MT/DBHASH platform bridge: only three production sections').replace(b'#include <platform/script_pool_services.h>',b'#include <platform/critical_section_services.h>').replace(b'defined(BO1_STRING_POOL_GATE))',b'(defined(BO1_STRING_POOL_GATE) || defined(BO1_ASSET_ENTRY_GATE)))').replace(b'CRITSECT_MEMORY_TREE == 28);',b'CRITSECT_MEMORY_TREE == 28 && CRITSECT_DBHASH == 74);').replace(b'    case CRITSECT_MEMORY_TREE: return bo1::script::PoolLock::MemoryTree;',b'    case CRITSECT_MEMORY_TREE: return bo1::script::PoolLock::MemoryTree;\n    case CRITSECT_DBHASH: return bo1::script::PoolLock::Database;');same(p,expected,files)
 p='src/platform/script_pool_services.h';expected=old(p);a=expected.index(b'// Native pool boundary');z=expected.index(b'bool __cdecl Sys_IsServerThread();',a);expected=expected[:a]+b'// Native sections: script string, memory tree and DBHASH.\n#include <platform/critical_section_services.h>\n'+expected[z:];same(p,expected,files)
 for p in ['src/database/db_asset_pool.h','src/database/db_assetlist_types.h','src/database/database.h','src/database/db_assetnames.cpp','src/database/db_load.cpp','src/database/db_expand.cpp','src/database/db_stream.cpp','src/universal/critical_sections.h','src/win32/win_common.cpp','src/win32/win_common.h']:
  same(p,old(p),files);files[p]['whole_source_unchanged']=True
 return {'schema':'BlackOpsARM.TASK31.source.v1','status':'passed','parent':'63c2fa4','files':files,'canonical_init_assignments_and_loop_preserved':True,'native_existing_lock_operations_preserved':True,'full_db_init_execution_certified':False}
if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('--output',required=True);args=parser.parse_args();p=Path(args.output);p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(prove(),indent=2));print('source proof passed')
