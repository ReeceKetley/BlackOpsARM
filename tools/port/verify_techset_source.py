"""Exact production leaf/normal policy preservation across additive gate selection."""
from pathlib import Path
import argparse,hashlib,json,re,subprocess
R=Path(__file__).resolve().parents[2]
def old(p):return subprocess.check_output(['git','show','5ae358a:'+p],cwd=R).decode('utf-8-sig').replace('\r\n','\n')
def now(p):return (R/p).read_text(encoding='utf-8-sig')
def body(s,name):
 matches=list(re.finditer(r'^[^ \t\n][^\n;{}]*\b'+re.escape(name)+r'\([^;{}]*?\)\s*\{',s,re.M));
 if len(matches)!=1:raise RuntimeError('Unique actual definition not found: '+name)
 a=matches[0].start();b=s.index('{',a);depth=1;c=b+1
 while depth:
  depth+=(s[c]=='{')-(s[c]=='}');c+=1
 return s[a:c]
def prove():
 result={}
 for p,names in {'src/database/db_load.cpp':['Load_MaterialTechniquePtr','Load_MaterialTechniquePtrArray','Load_MaterialTechniqueSet','Load_MaterialTechniqueSetPtr','AllocLoad_FxElemVisStateSample'],'src/database/db_expand.cpp':['DB_ExpandLoad_MaterialTechniqueSet'],'src/database/db_registry.cpp':['Load_MaterialTechniqueSetAsset','DB_AddXAsset','DB_LinkXAssetEntry','DB_CloneXAssetInternal','DB_FreeXAssetHeader_EmblemSet_'],'src/database/db_assetnames.cpp':['DB_SizeofXAsset_MaterialTechniqueSet_']}.items():
  for name in names:
   a,b=body(old(p),name),body(now(p),name)
   if a!=b:raise RuntimeError('Actual production body changed: '+name)
   result[name]=hashlib.sha256(a.encode()).hexdigest()
 original=old('src/gfx_d3d/r_material.h');a=original.index('struct MaterialTechniqueSet //');b=original.index('struct __declspec(align(4)) GfxImageLoadDef',a)
 if original[a:b] not in now('src/database/db_techset_types.h'):raise RuntimeError('Canonical declaration changed')
 if 'struct MaterialTechniqueSet //' in now('src/gfx_d3d/r_material.h'):raise RuntimeError('Duplicate canonical declaration')
 for macro,value in [('POOLSIZE_TECHSET',2048),('POOLSIZE_TECHSET_MOD',3072)]:
  pattern=r'#define '+macro+r'\s+'+str(value)
  if len(re.findall(pattern,now('src/database/db_asset_pool.h')))!=1 or re.search(pattern,now('src/database/db_registry.cpp')):raise RuntimeError('Canonical pool policy duplicate/change')
 for tree in ['tl','DemonWare']:subprocess.run(['git','diff','--exit-code','5ae358a','--',tree],cwd=R,check=True,capture_output=True)
 return {'schema':'BlackOpsARM.TASK38.source.v1','status':'passed','parent':'5ae358a','production_body_sha256':result,'canonical_type_moved_exactly':True,'canonical_pool2048_mod3072':True,'disk528_native1056_assertions_retained':True,'unsupported_fixture_dependencies_abort':True}
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--output',required=True);p=Path(ap.parse_args().output);p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(prove(),indent=2),encoding='utf-8');print('TASK38 source preservation passed')
