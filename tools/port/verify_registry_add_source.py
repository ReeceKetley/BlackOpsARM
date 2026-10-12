"""TASK36 named production selections and canonical clone-type move."""
from pathlib import Path
import argparse,hashlib,json,subprocess
from verify_entry_alloc_source import endbody
ROOT=Path(__file__).resolve().parents[2];PARENT='71eee49'
def old(p):return subprocess.check_output(['git','show',PARENT+':'+p],cwd=ROOT).replace(b'\r\n',b'\n').decode('utf-8')
opening='#endif\n#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_REGISTRY_ADD_GATE)\n'
closing='#endif\n#if !defined(BO1_STRINGTABLE_OWNER_GATE)\n'
def group(s,token,last=None):
 a=s.index(token);b=endbody(s,a if not last else s.index(last,a));return s[:a]+opening+s[a:b]+'\n'+closing+s[b:]
def line(s,token):
 assert s.count(token)==1;return s.replace(token,opening+token+'\n'+closing,1)
def expected():
 out={};p='src/database/db_registry.h';s=old(p);a=s.index('enum DBCloneMethod :');b=s.index('#include "db_rawfile_types.h"',a);out['src/database/db_clone_types.h']='#pragma once\n\n'+s[a:b];out[p]=s[:a]+'#include "db_clone_types.h"\n\n'+s[b:]
 p='src/database/db_registry.cpp';s=old(p).replace('#include "db_registry_lookup_api.h"','#include "db_registry_lookup_api.h"\n#if defined(BO1_REGISTRY_ADD_GATE)\n#include "db_registry_add_api.h"\nstatic bool DB_IsLevelSingletonType(XAssetType);\n#endif',1)
 # Original canonical array contents and globals, selecting only cheap state.
 a=s.index('const char *g_defaultAssetName[43]');b=s.index('\n};',a)+3;s=s[:a]+opening+s[a:b]+'\n'+closing+s[b:]
 for token in ['bool g_mayRecoverLostAssets;','int g_defaultAssetCount;','bool g_isRecoveringLostDevice;','unsigned int g_zoneIndex;','const dvar_t *db_xassetdebug;\nconst dvar_t *db_xassetdebugtype;'] :s=line(s,token)
 # A single canonical capacity avoids requiring unrelated Material/XModel
 # definitions merely to compile the full Link body in the finite gate.
 s=s.replace('static ModDeferredAsset g_modDeferred[8192];', 'static ModDeferredAsset g_modDeferred[DB_MOD_DEFERRED_CAPACITY];')
 count='static int g_modDeferredUsed; // slots handed out since the level\'s zones started loading (never reused before that)'
 s=s.replace(count,opening+'#if defined(BO1_REGISTRY_ADD_GATE)\nint g_modDeferredUsed; // standalone no-mod input\n#else\n'+count+'\n#endif\n'+closing,1)
 a=s.index('static ModDeferredAsset g_modDeferred[');s=s[:a]+'#endif\n#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_REGISTRY_ADD_GATE)\nstatic constexpr int DB_MOD_DEFERRED_CAPACITY = 8192;\n#endif\n#if !defined(BO1_STRINGTABLE_OWNER_GATE)\n'+s[a:]
 s=line(s,'static bool g_modDeferredLinking;\nstatic XAssetEntryPoolEntry g_modUnlinked; // returned by DB_LinkXAssetEntry for an asset of a mod zone it leaves unlinked\nbool g_modLazyModels = true; // L62: bo1_mod_lazymodels (Com_LoadLevelFastFiles)')
 s=group(s,'static bool DB_IsModDeferringLoad()')
 s=group(s,'XAssetHeader __cdecl DB_AddXAsset(','void DB_SyncLostDevice()')
 s=line(s,'static char g_modExtraZones[256];\nint g_dbTopAssetType = -1; // mod (L60): the top-level asset type Load_XAssetArrayCustom is loading, -1 between zones')
 s=group(s,'static bool DB_IsModExtraZone(', 'static bool DB_IsLevelSingletonType(XAssetType type)\n{')
 s=group(s,'XAssetEntryPoolEntry *__cdecl DB_LinkXAssetEntry(')
 s=s.replace('g_modDeferredUsed < ARRAY_COUNT(g_modDeferred)','g_modDeferredUsed < DB_MOD_DEFERRED_CAPACITY',1);out[p]=s
 p='src/database/db_assetnames.cpp';s=old(p).replace('#include "db_registry_lookup_api.h"','#include "db_registry_lookup_api.h"\n#if defined(BO1_REGISTRY_ADD_GATE)\n#include "db_registry_add_api.h"\n#endif',1);out[p]=group(s,'const char *__cdecl DB_GetXAssetTypeName(')
 p='src/database/db_load.cpp';s=old(p).replace('#include "db_scalar_load.h"','#include "db_scalar_load.h"\n#if defined(BO1_REGISTRY_ADD_GATE)\n#include "db_asset_entry_types.h"\n#endif',1).replace('XAsset *varXAsset;','#endif\n#if !defined(BO1_STRINGTABLE_INDEX_GATE) || defined(BO1_REGISTRY_ADD_GATE)\nXAsset *varXAsset;\n#endif\n#if !defined(BO1_STRINGTABLE_INDEX_GATE)',1);out[p]=s
 p='src/universal/q_shared.cpp';s=old(p).replace('#include <platform/bo1_native_width.h>','#include <platform/bo1_native_width.h>\n#if defined(BO1_REGISTRY_ADD_GATE)\n#include <ctype.h>\n#endif',1);a=s.index('const char *__cdecl I_stristr(');b=endbody(s,a);out[p]=s[:a]+'#endif\n#if !defined(BO1_STRINGTABLE_LOOKUP_GATE) || defined(BO1_REGISTRY_ADD_GATE)\n'+s[a:b]+'\n#endif\n#if !defined(BO1_STRINGTABLE_LOOKUP_GATE)\n'+s[b:]
 return out
def prove():
 exp=expected();records={}
 for p,s in exp.items():
  if (ROOT/p).read_text(encoding='utf-8')!=s:raise RuntimeError('Undocumented production delta '+p)
  records[p]=hashlib.sha256((ROOT/p).read_bytes()).hexdigest()
 allowed=set(exp)|{'src/database/db_registry_add_api.h'}
 changed=set(subprocess.check_output(['git','diff','--name-only',PARENT,'--','src','tl','DemonWare'],cwd=ROOT).decode().splitlines())|set(subprocess.check_output(['git','ls-files','--others','--exclude-standard','--','src','tl','DemonWare'],cwd=ROOT).decode().splitlines())
 if changed!=allowed:raise RuntimeError('Unexpected production tree changes '+str(changed))
 return {'schema':'BlackOpsARM.TASK36.source.v1','status':'passed','parent':PARENT,'files':records,'canonical_clone_enum_moved_once':True,'actual_add_and_full_link_bodies_preserved_except_named_capacity':True,'capacity_still_8192':True,'actual_no_mod_policy_and_sync_bodies_preserved':True,'fixture_endpoints_not_production':True}
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--output');ap.add_argument('--apply',action='store_true');args=ap.parse_args()
 if args.apply:
  for p,s in expected().items():(ROOT/p).write_text(s,encoding='utf-8',newline='\n')
 else:
  assert args.output;p=Path(args.output);p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(prove(),indent=2),encoding='utf-8');print('TASK36 named source proof passed')
