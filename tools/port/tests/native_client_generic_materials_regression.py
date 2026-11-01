"""Production CGM1 codec/publication with canonical types and isolated DB/IO."""
from pathlib import Path
import argparse,json,hashlib
root=Path(__file__).resolve().parents[3]
a=argparse.ArgumentParser();a.add_argument('--out',type=Path,required=True);args=a.parse_args()
s=(root/'src/game_sp/g_sp_native_owners_checkpoint.inl').read_text()
start=s.index('struct SPNativeCheckpointCodec');end=s.index('\n};',start)+3
codec=s[start:end]
fixture=r'''
#include <cgame_mp/cg_main_mp.h>
#include <cgame_mp/cg_local_mp.h>
#include <gfx_d3d/r_material.h>
#include <qcommon/common.h>
#include <universal/memfile.h>
#include <array>
#include <vector>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <cstdlib>
static cg_s cg;
cg_s *cgArray=&cg;
static Material a,b,wrong;
static bool badAsset,missingAsset;
static unsigned checks;
static std::vector<unsigned char> wire,input;
static void check(bool ok){if(!ok)std::abort();++checks;}
void Com_Error(errorParm_t,const char*,...){throw std::runtime_error("expected reject");}
void Com_Printf(int,const char*,...){}
bool Assert_MyHandler(const char*,int,int,const char*,...){std::abort();}
int I_stricmp(const char*x,const char*y){while(*x && std::tolower((unsigned char)*x)==std::tolower((unsigned char)*y)){++x;++y;}return std::tolower((unsigned char)*x)-std::tolower((unsigned char)*y);}
Material *Material_Register(char *name,int){if(missingAsset)return nullptr;if(badAsset)return &wrong;if(!I_stricmp(name,"fixture_a"))return &a;if(!I_stricmp(name,"fixture_b"))return &b;return nullptr;}
static void flush(MemoryFile*f){wire.insert(wire.end(),f->cacheBuffer,f->cacheBuffer+f->cacheBufferUsed);f->cacheBufferUsed=0;}
void MemFile_WriteDataInternal(MemoryFile*f,unsigned int n,unsigned char*p){flush(f);wire.insert(wire.end(),p,p+n);}
void MemFile_WriteInt(MemoryFile*f,int n){MemFile_WriteData(f,4,(unsigned char*)&n);}
void MemFile_ReadData(MemoryFile*f,int n,unsigned char*p){if(n<0 || (size_t)f->bytesUsed+n>input.size())Com_Error(ERR_DROP,"bounds");std::memcpy(p,input.data()+f->bytesUsed,n);f->bytesUsed+=n;}
char *MemFile_ReadCString(MemoryFile*f){auto start=f->bytesUsed;while((size_t)f->bytesUsed<input.size() && input[f->bytesUsed])++f->bytesUsed;if((size_t)f->bytesUsed==input.size())Com_Error(ERR_DROP,"string bounds");++f->bytesUsed;return (char*)input.data()+start;}
PRODUCTION_CODEC
#include <game_sp/g_sp_client_generic_materials_checkpoint.inl>
static void decode(){static MemoryFile reader,writer;reader={};writer={};wire.clear();SPNativeCheckpointCodec c{&reader,&writer};SP_DecodeClientGenericMaterials(c);flush(&writer);}
static bool rejected(){try{decode();return false;}catch(const std::runtime_error&){return true;}}
static std::array<Material*,64> snapshot(){std::array<Material*,64>v;std::memcpy(v.data(),cg.genericMaterialMap,sizeof(cg.genericMaterialMap));return v;}
int main(){
 a.info.name="Fixture_A";b.info.name="fixture_b";wrong.info.name="unrelated";
 for(int pattern=0;pattern<5;++pattern){
  for(int i=0;i<64;++i)cg.genericMaterialMap[i]=((i+pattern)%3==0)?&a:((i+pattern)%7==0)?&b:nullptr;
  const auto wanted=snapshot();static MemoryFile f;f={};wire.clear();SP_WriteClientGenericMaterials(&f);flush(&f);const auto saved=wire;input=saved;
  for(auto &m:cg.genericMaterialMap)m=&wrong;
  const auto before=snapshot();decode();check(wire==saved && snapshot()==before && s_clientGenericMaterialsPresent);
  SP_PublishClientGenericMaterials();check(snapshot()==wanted);
  // Repeated callbacks resolve the exact original ordinal, including holes
  // and repeated material owners; publication is stable across repeated use.
  for(int i=0;i<64;++i)check(cg.genericMaterialMap[i]==wanted[i]);
  SP_PublishClientGenericMaterials();check(snapshot()==wanted);
  for(bool missing:{false,true}){
   for(auto &m:cg.genericMaterialMap)m=&wrong;const auto old=snapshot();input=saved;
   badAsset=!missing;missingAsset=missing;check(rejected());badAsset=missingAsset=false;
   check(!s_clientGenericMaterialsPresent && snapshot()==old);SP_PublishClientGenericMaterials();check(snapshot()==old);
  }
  for(int off:{0,4,8}){
   input=saved;if(off<8){int bad=-1;std::memcpy(input.data()+off,&bad,4);}else input[off]=2;
   const auto old=snapshot();check(rejected());check(!s_clientGenericMaterialsPresent && snapshot()==old);
  }
  input=saved;input.resize(10);const auto old=snapshot();check(rejected() && snapshot()==old);
 }
 // Empty registry is valid; legacy versions skip CGM1 and must not publish a
 // stale staged registry from an earlier operation.
 for(auto&m:cg.genericMaterialMap)m=nullptr;
 static MemoryFile f;f={};wire.clear();SP_WriteClientGenericMaterials(&f);flush(&f);input=wire;decode();SP_PublishClientGenericMaterials();for(auto*m:cg.genericMaterialMap)check(!m);
 cg.genericMaterialMap[63]=&b;s_clientGenericMaterialsPresent=false;SP_PublishClientGenericMaterials();check(cg.genericMaterialMap[63]==&b);
 std::printf("generic material registry %u checks PASS\n",checks);
}
'''
args.out.mkdir(parents=True,exist_ok=False)
(args.out/'fixture.cpp').write_text(fixture.replace('PRODUCTION_CODEC',codec),encoding='utf-8')
files=['src/game_sp/g_sp_native_owners_checkpoint.inl','src/game_sp/g_sp_client_generic_materials_checkpoint.inl','src/game_sp/g_sp_client_models_checkpoint.inl','src/game_sp/g_sp_client_checkpoint.inl']
(args.out/'source-manifest.json').write_text(json.dumps({f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in files},indent=2),encoding='utf-8')
