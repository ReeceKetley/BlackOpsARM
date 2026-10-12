"""Whole-source TASK32 proof: named compile selection and three ABI corrections."""
from pathlib import Path
import argparse,hashlib,json,re,subprocess
ROOT=Path(__file__).resolve().parents[2]
def old(p):return subprocess.check_output(['git','--no-optional-locks','show','f523687:'+p],cwd=ROOT).replace(b'\r\n',b'\n').decode('utf-8')
def now(p):return (ROOT/p).read_bytes().replace(b'\r\n',b'\n').decode('utf-8')
def endbody(text,start):
 a=text.index('{',start);depth=0
 for t in re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]',re.S).finditer(text,a):
  if t.group()=='{':depth+=1
  elif t.group()=='}':
   depth-=1
   if not depth:return t.end()
 raise RuntimeError('Unclosed canonical body')
opening='#endif\n#if !defined(BO1_STRINGTABLE_OWNER_GATE) || defined(BO1_ENTRY_ALLOC_GATE)\n';closing='#endif\n#if !defined(BO1_STRINGTABLE_OWNER_GATE)\n'
def group(text,first,last=None):
 a=text.index(first);b=endbody(text,a if not last else text.index(last,a));return text[:a]+opening+text[a:b]+'\n'+closing+text[b:]
def array(text,token,rows):
 a=text.index(token);ob=text.index('{',a);end=text.index('\n};',ob);content=text[ob+1:end];wrapped=text[a:ob+1]+'\n#if defined(BO1_ENTRY_ALLOC_GATE)\n'+''.join('  '+x+',\n' for x in rows)+'#else'+content+'\n#endif'+text[end:end+3];return text[:a]+opening+wrapped+'\n'+closing+text[end+3:]
def check(p,expected,report):
 actual=now(p)
 if actual!=expected:raise RuntimeError('Undocumented source delta '+p)
 report[p]={'exact_named_edits_only':True,'sha256':hashlib.sha256(actual.encode()).hexdigest(),'bom_preserved':old(p).startswith('\ufeff')==actual.startswith('\ufeff')}
def prove():
 report={};p='src/database/db_registry.h';s=old(p);a=s.index('enum FF_DIR :');b=s.index('enum DBCloneMethod',a);enum=s[a:b];c=s.index('struct __declspec(align(4)) XZoneName');d=s.index('#include "db_asset_pool.h"',c);zone=s[c:d];s=s[:c]+s[d:];s=s[:a]+'#include "db_zone_name_types.h"\n\n'+s[b:];s=s.replace('DB_PrintAssetName(XAssetHeader header, int *data)','DB_PrintAssetName(XAssetHeader header, void *data)');check(p,s,report)
 light=now('src/database/db_zone_name_types.h');expected='#pragma once\n#include <cstddef>\n\n'+enum+zone+'static_assert(sizeof(FF_DIR)==4 && sizeof(XZoneName)==80);\nstatic_assert(offsetof(XZoneName,flags)==64 && offsetof(XZoneName,loaded)==76);\nextern XZoneName g_zoneNames[33];\n';assert light==expected
 p='src/database/db_asset_pool.h';check(p,old(p).replace('DB_FreeXAssetHeader_EmblemSet_(XAssetPoolEntry<EmblemSet> **pool','DB_FreeXAssetHeader_EmblemSet_(void *pool'),report)
 p='src/database/db_registry.cpp';s=old(p).replace('#include "db_asset_entry_types.h"','#include "db_asset_entry_types.h"\n#if defined(BO1_ENTRY_ALLOC_GATE)\n#include "db_entry_alloc_api.h"\n#endif',1)
 for token,rows in [('static int g_poolSize[43]',['0']*36+['POOLSIZE_RAWFILE','POOLSIZE_STRINGTABLE']+['0']*5),('XAssetHeader(__cdecl *DB_AllocXAssetHeaderHandler[43])',['NULL']*36+['DB_AllocXAsset<RawFile>','DB_AllocXAsset<StringTable>']+['NULL']*5),('void *DB_XAssetPool[43]',['NULL']*36+['&g_RawFilePool','&g_StringTablePool']+['NULL']*5),('void(__cdecl *DB_FreeXAssetHeaderHandler[43])',['NULL']*36+['DB_FreeXAssetHeader_EmblemSet_']*2+['NULL']*5)]:s=array(s,token,rows)
 s=s.replace('(void(*)(void*, XAssetHeader))DB_FreeXAssetHeader_EmblemSet_','DB_FreeXAssetHeader_EmblemSet_').replace('DB_FreeXAssetHeader_EmblemSet_(XAssetPoolEntry<EmblemSet> **pool','DB_FreeXAssetHeader_EmblemSet_(void *pool')
 for line in ['XZoneName g_zoneNames[33];','int XAssetEntryHighWaterMark;\nint XAssetEntryHighCount;']:assert s.count(line)==1;s=s.replace(line,opening+line+'\n'+closing,1)
 s=group(s,'void __cdecl DB_PrintXAssetsForType_FastFile(', 'void __cdecl DB_PrintXAssetsForType(');s=group(s,'XAssetHeader __cdecl DB_AllocXAssetHeader(', 'void __cdecl DB_FreeXAssetHeader(');s=group(s,'XAssetEntryPoolEntry *__cdecl DB_AllocXAssetEntry(');s=group(s,'void __cdecl DB_FreeXAssetEntry(')
 s=s.replace('#if !defined(BO1_ASSET_ENTRY_GATE)\nvoid __cdecl DB_CloneXAssetInternal','#if !defined(BO1_ASSET_ENTRY_GATE) || defined(BO1_ENTRY_ALLOC_GATE)\nvoid __cdecl DB_CloneXAssetInternal').replace('DB_PrintAssetName(XAssetHeader header, int *data)','DB_PrintAssetName(XAssetHeader header, void *data)').replace('DB_GetXAssetHeaderName(*data, &header)','DB_GetXAssetHeaderName(*static_cast<int *>(data), &header)').replace('(void (__cdecl *)(XAssetHeader, void *))DB_PrintAssetName','DB_PrintAssetName');check(p,s,report)
 p='src/database/db_assetnames.cpp';s=old(p).replace('#include "db_asset_pool.h"','#include "db_asset_pool.h"\n#include <cstring>\n#if defined(BO1_ENTRY_ALLOC_GATE)\n#include "db_entry_alloc_api.h"\n#endif',1);s=array(s,'const char *(__cdecl *DB_XAssetGetNameHandler[43])',['NULL']*36+['DB_DDLGetName']*2+['NULL']*5);a=s.index('const char *g_assetNames[43]');ob=s.index('{',a);z=s.index('\n};',ob)+3;s=s[:a]+opening+s[a:z]+'\n'+closing+s[z:];s=group(s,'const char *__cdecl DB_DDLGetName(');s=group(s,'const char *__cdecl DB_GetXAssetHeaderName(');s=s.replace('    return header->xmodelPieces->name;','    void *payload;\n    const char *name;\n    ::memcpy(&payload, header, sizeof(payload));\n    ::memcpy(&name, payload, sizeof(name));\n    return name;',1);check(p,s,report)
 for p in ['src/database/db_asset_entry_types.h','src/database/db_load.cpp','src/database/db_expand.cpp','src/database/db_stream.cpp','src/database/db_stream_load.cpp','src/platform/android/script_string_services.cpp','src/platform/android/script_pool_bridge.cpp']:
  check(p,old(p),report);report[p]['whole_source_unchanged']=True
 return {'schema':'BlackOpsARM.TASK32.source.v1','parent':'f523687','status':'passed','files':report,'canonical_allocation_and_entry_free_bodies_preserved':True,'generic_free_four_copy_body_preserved':True,'typed_callback_and_void_pool_signatures':True,'name_getter_nonnull_pointer_representation':True,'game_assets_copied':False}
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--output',required=True);args=ap.parse_args();p=Path(args.output);p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(prove(),indent=2));print('TASK32 source proof passed')
