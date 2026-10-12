"""Restore exact defaults and verify unique production ownership leaves."""
from pathlib import Path
import argparse,hashlib,json,subprocess,sys
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
def digest(data):return hashlib.sha256(data).hexdigest().upper()
def once(data,old,new):
 assert data.count(old)==1,(old[:100],data.count(old));return data.replace(old,new,1)
def body(data,signature):
 marker=signature+b'\r\n{';assert data.count(marker)==1,signature;start=data.index(marker);pos=start+len(marker);depth=1
 while depth:
  if data[pos:pos+1]==b'{':depth+=1
  elif data[pos:pos+1]==b'}':depth-=1
  pos+=1
 return data[start:pos]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--baseline',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 b=json.loads(a.baseline.read_text(encoding='utf-8'));parent=b['BlackOpsARM']['head'];out=a.output.resolve()
 assert out.is_relative_to(ROOT/'build-android-audit') and not out.exists()
 result={'schema':'BlackOpsARM.TASK-0026.source-preservation.v1','parent':parent,'files':{},'leaves':{},'verifier_sha256':digest(Path(__file__).read_bytes())}
 def original(n):
  raw=subprocess.check_output(['git','--no-optional-locks','show',parent+':'+n],cwd=ROOT);lf=raw.replace(b'\r\n',b'\n');choices={raw,lf,lf.replace(b'\n',b'\r\n')}
  good=[data for data in choices if digest(data)==b['BlackOpsARM']['files'][n]];assert len(good)==1,n;return good[0]
 def prove(n,data):
  assert digest(data)==b['BlackOpsARM']['files'][n],n;result['files'][n]={'default_restored_exact_working_bytes':True,'current_sha256':digest((ROOT/n).read_bytes())}
 old=original('src/database/db_registry.cpp');work=(ROOT/'src/database/db_registry.cpp').read_bytes();pool=(ROOT/'src/database/db_asset_pool.h').read_bytes()
 alloc=b'template <typename T>\r\n'+body(old,b'XAssetHeader DB_AllocXAsset(void *pool)');init=b'template <typename T>\r\n'+body(old,b'void __cdecl DB_InitPool(void *arg, int32_t size)')
 policy=b'#define POOLSIZE_STRINGTABLE      576\r\n#define POOLSIZE_STRINGTABLE_MOD 768 // mod: capacity for bo1_mod_zones; in use only with it on the command line (DB_InitXAssets)\r\n'
 for name,chunk in [('Alloc',alloc),('Init',init),('policy',policy)]:assert pool.count(chunk)==1 and not work.count(chunk),name;result['leaves'][name]={'canonical_move_byte_exact':True,'sha256':digest(chunk)}
 def unselect(data,head,segments):
  bom=b'\xef\xbb\xbf' if data.startswith(b'\xef\xbb\xbf') else b'';data=data[len(bom):]
  data=once(data,head,b'');data=once(data,b'#endif\r\n#if !defined(BO1_STRINGTABLE_OWNER_GATE)\r\n',b'')
  for chunk in segments:data=once(data,b'\r\n#endif\r\n'+chunk+b'\r\n#if !defined(BO1_STRINGTABLE_OWNER_GATE)\r\n',chunk)
  footer=b'\r\n#endif // !BO1_STRINGTABLE_OWNER_GATE\r\n';assert data.endswith(footer);return bom+data[:-len(footer)]
 freeSig=b'void __cdecl DB_FreeXAssetHeader_EmblemSet_(XAssetPoolEntry<EmblemSet> **pool, XAssetHeader header)';cloneSig=b'void __cdecl DB_CloneXAssetInternal(const XAsset *from, XAsset *to)'
 free=body(work,freeSig);clone=body(work,cloneSig);assert clone==body(old,cloneSig)
 line=next(line for line in work.splitlines(keepends=True) if b'g_StringTablePool;' in line)
 head=b'#if defined(BO1_STRINGTABLE_OWNER_GATE)\r\n#include "db_asset_pool.h"\r\n#include <universal/assertive.h>\r\n#include <cstring>\r\n#else\r\n'
 default=unselect(work,head,[line,free,clone])
 expected=once(old,policy,b'');expected=once(expected,alloc+b'\r\n',b'');expected=once(expected,init+b'\r\n',b'');expected=once(expected,freeSig+b';\r\n',b'');expected=once(expected,body(old,freeSig),free)
 assert default==expected,'db_registry default beyond documented moves/free change'
 result['files']['src/database/db_registry.cpp']={'all_default_bytes_exact_except_canonical_moves_and_generic_free':True,'current_sha256':digest(work)}
 result['leaves']['Clone']={'body_byte_unchanged':True,'sha256':digest(clone)}
 assert free.count(b'::memcpy(')==4 and b'header.xmodelPieces->name' not in free
 result['leaves']['Free']={'only_intentional_changed_body':True,'old_sha256':digest(body(old,freeSig)),'new_sha256':digest(free),'four_native_pointer_representation_transfers':True,'no_child_release':True}
 n='src/database/db_registry.h';oldH=original(n);start=oldH.index(b'template <typename T>\r\nunion XAssetPoolEntry');end=oldH.index(b'struct Material;',start);types=oldH[start:end];assert pool.count(types)==1
 expected=oldH[:start]+b'#include "db_asset_pool.h"\r\n\r\n'+oldH[end:];expected=once(expected,cloneSig+b';\r\n',b'');assert (ROOT/n).read_bytes()==expected
 result['files'][n]={'only_canonical_pool_types_include_and_clone_prototype_move':True};result['leaves']['PoolTypes']={'byte_unchanged':True,'sha256':digest(types)}
 n='src/database/db_assetnames.h';oldH=original(n);expected=once(oldH,b'int __cdecl DB_GetXAssetTypeSize(int type);\r\n',b'');expected=once(expected,b'#pragma once\r\n',b'#pragma once\r\n#include "db_asset_pool.h"\r\n');assert (ROOT/n).read_bytes()==expected
 result['files'][n]={'only_canonical_size_prototype_include_move':True}
 n='src/database/db_assetnames.cpp';old=original(n);work=(ROOT/n).read_bytes();assert work.startswith(b'\xef\xbb\xbf') and work.count(b'\xef\xbb\xbf')==1
 getterSig=b'int __cdecl DB_GetXAssetTypeSize(int type)';sizeSig=b'int __cdecl DB_SizeofXAsset_StringTable_()'
 sizeStart=work.index(b'#if !BO1_NATIVE_64BIT\r\nstatic_assert(sizeof(StringTable) == 20);');sizeBody=body(work,sizeSig);size=work[sizeStart:work.index(sizeBody)+len(sizeBody)];getter=body(work,getterSig)
 tableStart=work.index(b'int(__cdecl *DB_GetXAssetSizeHandler[43])() =');tableEnd=work.index(b'\r\n};',tableStart)+len(b'\r\n};');table=work[tableStart:tableEnd]
 default=unselect(work,b'#if defined(BO1_STRINGTABLE_OWNER_GATE)\r\n#include "db_asset_pool.h"\r\n#include <universal/assertive.h>\r\n#else\r\n',[size,table,getter])
 restored=once(table,b'#if defined(BO1_STRINGTABLE_OWNER_GATE)\r\n'+b'  NULL,\r\n'*37+b'#else\r\n',b'')
 restored=once(restored,b'#endif\r\n  &DB_SizeofXAsset_StringTable_,\r\n',b'  &DB_SizeofXAsset_StringTable_,\r\n')
 tail=b'  &DB_SizeofXAsset_PackIndex_,\r\n  &DB_SizeofXAsset_XGlobals_,\r\n  &DB_SizeofXAsset_ddlRoot_t_,\r\n  &DB_SizeofXAsset_Glasses_,\r\n  &DB_SizeofXAsset_EmblemSet_\r\n'
 restored=once(restored,b'#if !defined(BO1_STRINGTABLE_OWNER_GATE)\r\n'+tail+b'#endif\r\n',tail)
 default=once(default,table,restored);assert default==old,n
 result['files'][n]={'whole_default_bytes_table_slot_order_and_bom_exact':True,'current_sha256':digest(work)}
 for name,signature in [('TypeSize',getterSig),('StringTableSize',sizeSig)]:assert body(work,signature)==body(old,signature);result['leaves'][name]={'body_byte_unchanged':True,'sha256':digest(body(work,signature))}
 for n in ['src/database/db_expand.cpp','src/database/db_stream_load.cpp','src/database/db_load.cpp','src/universal/com_stringtable.cpp','src/universal/com_stringtable_obj.cpp','src/universal/q_shared.cpp','src/universal/com_hash.cpp','src/universal/com_stringtable_sort.cpp','src/platform/win32_compat/sort16.cpp']:
  prove(n,(ROOT/n).read_bytes())
 n='android/cmake/arm64-audit/CMakeLists.txt';data=(ROOT/n).read_bytes();start=data.index(b'        add_executable(bo1_stringtable_owner_tests ');end=data.index(b'        add_custom_command(',start);prove(n,data[:start]+data[end:])
 assert pool.count(b'extern XAssetPool<StringTable, POOLSIZE_STRINGTABLE_MOD> g_StringTablePool;')==1
 result.update(status='passed',registry_execution_certified=False,only_behavior_change='defined native pointer representation transfers in existing genericfree',original_init_alloc_clone_size_policy_and_all_prior_leaves_retained=True)
 out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes((json.dumps(result,indent=2)+'\n').encode());print('PASS default source/unique ownership leaves/policy/BOM preservation; isolated four-copy free correction')
if __name__=='__main__':main()
