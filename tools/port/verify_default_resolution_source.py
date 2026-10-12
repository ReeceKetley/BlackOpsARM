"""Exact production default services/type proof against verified TASK39."""
from pathlib import Path
import argparse, hashlib, json, subprocess
import verify_bootstrap_default_source as previous
R=Path(__file__).resolve().parents[2]
PARENT='76a4956'
def old(path):return subprocess.check_output(['git','show',PARENT+':'+path],cwd=R).decode('utf-8-sig').replace('\r\n','\n')
def now(path):return (R/path).read_text(encoding='utf-8-sig')
def prove():
 result={}
 corrected_setter={}
 paths=['src/database/db_load.cpp','src/database/db_expand.cpp','src/database/db_registry.cpp','src/database/db_assetnames.cpp','src/database/db_stream_load.cpp','src/gfx_d3d/r_material_shader_load.inl']
 # Frozen TASK39 source-proof names are independent of the current source edits.
 names=['Load_DWORDArray','Load_uint','Load_floatArray','Load_GfxVertexShaderLoadDef','Load_GfxPixelShaderLoadDef','Load_MaterialVertexShaderProgram','Load_MaterialPixelShaderProgram','Load_MaterialVertexShader','Load_MaterialVertexShaderPtr','Load_MaterialPixelShader','Load_MaterialPixelShaderPtr','Load_MaterialVertexDeclaration','Load_MaterialArgumentCodeConst','Load_MaterialArgumentDef','Load_MaterialShaderArgument','Load_MaterialShaderArgumentArray','Load_MaterialPass','Load_MaterialPassArray','Load_MaterialTechnique','Load_MaterialTechniquePtr','Load_MaterialTechniquePtrArray','Load_MaterialTechniqueSet','Load_MaterialTechniqueSetPtr','AllocLoad_FxElemVisStateSample','DB_ExpandLoad_MaterialTechniqueSet','DB_ExpandLoad_MaterialTechnique','DB_ExpandLoad_MaterialPassArray','DB_ExpandLoad_MaterialShaderArgumentArray','DB_ExpandLoad_MaterialVertexShader','DB_ExpandLoad_MaterialPixelShader','DB_ExpandLoad_MaterialVertexDeclaration','Load_MaterialTechniqueSetAsset','DB_AddXAsset','DB_LinkXAssetEntry','DB_CloneXAssetInternal','DB_FreeXAssetHeader_EmblemSet_','DB_SizeofXAsset_MaterialTechniqueSet_','DB_ConvertOffsetToPointer','Load_XStringCustom','Load_CreateMaterialPixelShader','Load_CreateMaterialVertexShader','Load_BuildVertexDecl','DB_FindXAssetDefaultHeaderInternal','DB_CreateDefaultEntry','DB_FreeDefaultEntries','DB_SetXAssetName','DB_DDLSetname']
 for name in names:
  matches=[]
  for path in paths:
   try:a=previous.body(old(path),name)
   except RuntimeError:continue
   matches.append((path,a))
  if len(matches)!=1:raise RuntimeError('Unique original production body missing: '+name)
  path,a=matches[0];b=previous.body(now(path),name)
  if name=='DB_DDLSetname':
   expected='''void __cdecl DB_DDLSetname(XAssetHeader *header, const char *name)
{
    // Generic assets carry their native-width name pointer in the first word.
    // Copy representations without accessing an unrelated payload structure.
    void *payload;
    ::memcpy(&payload, header, sizeof(payload));
    ::memcpy(payload, &name, sizeof(name));
}'''
   if b!=expected:raise RuntimeError('Generic native name setter differs from reviewed representation-copy implementation')
   corrected_setter={'before_sha256':hashlib.sha256(a.encode()).hexdigest(),'after_sha256':hashlib.sha256(b.encode()).hexdigest(),'native_first_word_assignment_preserved':True,'unrelated_payload_typed_access_removed':True,'unchanged_unchecked_nonnull_caller_contract':True}
   continue
  if a!=b:raise RuntimeError('Actual production body changed: '+name)
  result[name]=hashlib.sha256(a.encode()).hexdigest()
 if old('src/xanim/xmodel.h')!=now('src/xanim/xmodel.h'):raise RuntimeError('Unneeded XModel declaration change')
 for path in ['src/gfx_d3d/r_material.h','src/gfx_d3d/r_material.cpp','src/gfx_d3d/r_material_shader_load.inl','src/database/db_material_graph_types.h','src/database/db_asset_pool.h','src/database/db_stream.cpp','src/database/db_stream_load.cpp','src/universal/physicalmemory.cpp','src/universal/physicalmemory.h','src/clientscript/cscr_stringlist.cpp','src/clientscript/cscr_memorytree.cpp','src/clientscript/cscr_instance.cpp']:
  if old(path)!=now(path):raise RuntimeError('Unexpected production type/policy implementation change: '+path)
 # The new setter selection retains the complete original normal dispatch array.
 path='src/database/db_assetnames.cpp';a=old(path).index('void(__cdecl *DB_XAssetSetNameHandler');b=old(path).index('};',a);original=old(path)[old(path).index('{',a)+1:b]
 if original not in now(path):raise RuntimeError('Normal setter dispatch changed')
 for tree in ['tl','DemonWare']:subprocess.run(['git','diff','--exit-code',PARENT,'--',tree],cwd=R,check=True,capture_output=True)
 return {'schema':'BlackOpsARM.TASK40.source.v1','status':'passed','parent':PARENT,'production_body_sha256':result,'corrected_generic_name_setter':corrected_setter,'XModel_declarations_unchanged':True,'normal_setter_dispatch_unchanged':True,'SL_MT_and_PMem_policy_unchanged':True}
if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('--output',required=True);out=Path(parser.parse_args().output);out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(prove(),indent=2)+'\n',encoding='utf-8');print('TASK40 source preservation passed')
