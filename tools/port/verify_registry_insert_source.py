"""TASK35 exact production extraction proof against the lookup checkpoint."""
from pathlib import Path
import argparse,hashlib,json,subprocess
from verify_entry_alloc_source import endbody
ROOT=Path(__file__).resolve().parents[2]
PARENT='54dd902'
def old(p):return subprocess.check_output(['git','show',PARENT+':'+p],cwd=ROOT).replace(b'\r\n',b'\n').decode('utf-8-sig')
def now(p):return (ROOT/p).read_text(encoding='utf-8-sig')
def prove():
 records={};p='src/database/db_registry.cpp';s=old(p)
 a=s.index('// Win32 packed XAssetEntry was 16 bytes');guard=s.rindex('#if !defined(BO1_STRINGTABLE_OWNER_GATE)',0,a)
 s=s[:guard]+s[guard:].replace('#if !defined(BO1_STRINGTABLE_OWNER_GATE)','#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_REGISTRY_INSERT_GATE)',1)
 a=s.index('static inline unsigned short DB_AssetEntryIndex(');end=endbody(s,a)
 helper='''

// Caller holds DBHASH and has completed the new-name/duplicate decision. The
// entry belongs to g_assetEntryPool and owns its live header/zone. This is only
// primary-chain insertion; default, override and mod policy stays with Link.
void __cdecl DB_InsertNewXAssetEntry(XAssetEntry *newEntry, unsigned int hash)
{
    newEntry->nextHash = db_hashTable[hash % 0x8000];
    db_hashTable[hash % 0x8000] = DB_AssetEntryIndex(newEntry);
}'''
 s=s[:end]+helper+s[end:]
 original='        newEntry->nextHash = db_hashTable[hash % 0x8000];\n        db_hashTable[hash % 0x8000] = DB_AssetEntryIndex(newEntry);'
 assert s.count(original)==1;s=s.replace(original,'        DB_InsertNewXAssetEntry(newEntry, hash);',1)
 expected={p:s,'src/database/db_registry.h':old('src/database/db_registry.h').replace('unsigned int __cdecl DB_HashForName(const char *name, XAssetType type);','unsigned int __cdecl DB_HashForName(const char *name, XAssetType type);\nvoid __cdecl DB_InsertNewXAssetEntry(XAssetEntry *newEntry, unsigned int hash);',1)+'\n','src/database/db_registry_lookup_api.h':old('src/database/db_registry_lookup_api.h').replace('unsigned int __cdecl DB_HashForName(const char *,XAssetType);','unsigned int __cdecl DB_HashForName(const char *,XAssetType);\n// DBHASH-held, pool-owned entry, live header/zone, new-name decision complete.\nvoid __cdecl DB_InsertNewXAssetEntry(XAssetEntry *,unsigned int);',1)}
 for p,s in expected.items():
  if now(p)!=s:raise RuntimeError('Undocumented production delta '+p)
  records[p]=hashlib.sha256((ROOT/p).read_bytes()).hexdigest()
 changed=set(subprocess.check_output(['git','diff','--name-only',PARENT,'--','src','tl','DemonWare'],cwd=ROOT).decode().splitlines())
 if changed!=set(expected):raise RuntimeError('Other production tree changes '+str(changed))
 if subprocess.check_output(['git','ls-files','--others','--exclude-standard','--','src','tl','DemonWare'],cwd=ROOT).strip():raise RuntimeError('Unexpected new production source')
 if not now('android/cmake/arm64-audit/CMakeLists.txt').startswith(old('android/cmake/arm64-audit/CMakeLists.txt')):raise RuntimeError('Existing CMake targets changed')
 return {'schema':'BlackOpsARM.TASK35.source.v1','status':'passed','parent':PARENT,'files':records,'original_two_assignments_preserved':True,'native_index_body_preserved':True,'normal_link_call_position_preserved':True,'other_link_policy_and_lookup_bodies_preserved':True,'existing_targets_preserved':True,'full_link_or_add_executed':False,'fixture_removal_explicit':True}
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--output',required=True);args=ap.parse_args();p=Path(args.output);p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(prove(),indent=2));print('TASK35 exact source extraction proof passed')
