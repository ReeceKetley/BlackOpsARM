"""Actual destructible codec + canonical layouts; isolate byte IO, assets and hunk provider.
This is a codec/ownership fixture, not an engine asset/gameplay restoration test.
"""
from pathlib import Path
import argparse,hashlib,json
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
source=(root/'src/physics/destructible.cpp').read_text(encoding='utf-8')
start=source.index('void __cdecl Destructible_FreeDestructible(');b=source.index('{',start);e=b+1;depth=1
while depth:
    depth+=(source[e]=='{')-(source[e]=='}');e+=1
free=source[start:e]
fixture=r"""
#include <physics/destructible.h>
#include <game_mp/g_main_mp.h>
#include <universal/mem_userhunk.h>
#include <universal/memfile.h>
#include <qcommon/common.h>
#include <clientscript/cscr_stringlist.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <set>
#include <vector>
#include <cstdarg>
gentity_s g_entities[MAX_GENTITIES_SV];
Destructible s_destructibles[64]; unsigned int s_numDestructibles;
int g_numDisabledLables,g_disabledLabels[10];
HunkUser *g_pieceArrayHunk;
alignas(16) unsigned char g_pieceArrayBuffer[73728];
static std::set<void*> allocations;
static DestructibleDef definition;
static unsigned checks;
static void check(bool yes) { if(!yes) std::abort(); ++checks; }
void Com_Error(errorParm_t,const char *fmt,...) {throw std::runtime_error(fmt);}
void Com_Printf(int,const char*,...) {}
bool Assert_MyHandler(const char*,int,int,const char*,...) {std::abort();}
HunkUser *Hunk_UserCreateFromBuffer(void*,int,HU_ALLOCATION_SCHEME,unsigned int,void*,const char*,int) {return (HunkUser*)g_pieceArrayBuffer;}
void *Hunk_UserAlloc(HunkUser*,int size,int,const char*) {void *p=std::calloc(1,size);allocations.insert(p);return p;}
void Hunk_UserFree(HunkUser*,void *p) {check(allocations.erase(p)==1);std::free(p);}
void XDoll_DestroyXDoll(int handle) {check(handle==0);}
DestructibleDef *DestructibleDefPrecache(const char *name) {return !std::strcmp(name,"fixture_def")?&definition:nullptr;}
char *SL_ConvertToString(unsigned int id,scriptInstance_t) {check(id==7);return (char*)"fixture_label";}
unsigned int SL_FindString(const char *s,scriptInstance_t) {return !std::strcmp(s,"fixture_label")?7:0;}
void MemFile_WriteInt(MemoryFile *f,int n) {MemFile_WriteData(f,4,(unsigned char*)&n);}
void MemFile_WriteDataInternal(MemoryFile*,unsigned int,unsigned char*) {std::abort();}
void MemFile_ReadData(MemoryFile *f,int n,unsigned char *p) {
 if(n<0 || f->bytesUsed+n>f->cacheBufferUsed) Com_Error(ERR_DROP,"fixture bounds");
 std::memcpy(p,f->cacheBuffer+f->bytesUsed,n);f->bytesUsed+=n;
}
char *MemFile_ReadCString(MemoryFile *f) {static char s[1024];int i=0;do {if(i==1024)std::abort();MemFile_ReadData(f,1,(unsigned char*)&s[i]);}while(s[i++]);return s;}
ACTUAL_FREE
#include <physics/destructible_save.inl>
static void reset() {
 check(allocations.empty());std::memset(g_entities,0,sizeof(g_entities));
 std::memset(s_destructibles,0,sizeof(s_destructibles));
 for(auto &d:s_destructibles)d.entNum=ENTITYNUM_NONE;
 s_numDestructibles=0;g_numDisabledLables=0;
}
static void cleanup() {
 for(auto &d:s_destructibles) if(d.entNum!=ENTITYNUM_NONE) Destructible_FreeDestructible(&d);
 check(s_numDestructibles==0 && allocations.empty());
}
static void reject(MemoryFile &f) {
 f.bytesUsed=0;bool failed=false;try{Destructible_ReadSaveState(&f);}catch(const std::runtime_error&){failed=true;}
 check(failed);cleanup();reset();
}
int main() {
 static MemoryFile saved,again;
 definition.name="fixture_def";
 for(int pieces : {1,8,32}) for(int owners : {0,1,64}) {
 reset();definition.numPieces=pieces;
 for(int i=0;i<owners;++i) {
  auto &d=s_destructibles[i];int ent=100+i;
  d.entNum=ent;d.ddef=&definition;d.pieceCount=pieces;d.flags=2|4;
  d.oldestBurnTime=125;d.bHasBeenHit=1;d.states[2].time=919;
  d.states[2].state.modelState4=0x87654321;
  d.pieceArray=(DESTRUCTIBLE_PIECE_INFO*)Hunk_UserAlloc(nullptr,pieces*sizeof(DESTRUCTIBLE_PIECE_INFO),4,nullptr);
  for(int j=0;j<pieces;++j){d.pieceArray[j].health=j%2?-1:103-j;d.pieceArray[j].burnData.burnTime=42;}
  g_entities[ent].r.inuse=1;g_entities[ent].destructible=&d;++s_numDestructibles;
 }
 g_numDisabledLables=1;g_disabledLabels[0]=7;saved={};Destructible_WriteSaveState(&saved);
 cleanup(); // Real free body must release every allocated piece owner.
 for(int i=0;i<owners;++i)g_entities[100+i].destructible=&s_destructibles[i];
 Destructible_ReadSaveState(&saved);check(s_numDestructibles==(unsigned)owners);
 again={};Destructible_WriteSaveState(&again);
 check(saved.cacheBufferUsed==again.cacheBufferUsed && !std::memcmp(saved.cacheBuffer,again.cacheBuffer,saved.cacheBufferUsed));
 for(int i=0;i<owners;++i)for(int j=0;j<pieces;++j)check(s_destructibles[i].pieceArray[j].xdollHandle==0);
 cleanup();
 }
 reset();saved={};MemFile_WriteInt(&saved,0);reject(saved); // magic
 for(int n : {-1,11}){saved={};MemFile_WriteInt(&saved,0x31545344);MemFile_WriteInt(&saved,n);reject(saved);}
 for(int n : {-2,64}){saved={};MemFile_WriteInt(&saved,0x31545344);MemFile_WriteInt(&saved,0);MemFile_WriteInt(&saved,n);reject(saved);}
 saved={};MemFile_WriteInt(&saved,0x31545344);MemFile_WriteInt(&saved,1);MemFile_WriteCString(&saved,(char*)"missing");reject(saved);
 // A valid owner prefix then wrong piece count must fail without allocating.
 definition.numPieces=1;g_entities[100].r.inuse=1;g_entities[100].destructible=&s_destructibles[0];
 saved={};MemFile_WriteInt(&saved,0x31545344);MemFile_WriteInt(&saved,0);MemFile_WriteInt(&saved,0);MemFile_WriteInt(&saved,100);
 MemFile_WriteCString(&saved,(char*)"fixture_def");MemFile_WriteInt(&saved,33);reject(saved);
 // Allocation is published before reading the tail; truncation must unwind via normal free.
 g_entities[100].r.inuse=1;g_entities[100].destructible=&s_destructibles[0];
 saved={};MemFile_WriteInt(&saved,0x31545344);MemFile_WriteInt(&saved,0);MemFile_WriteInt(&saved,0);MemFile_WriteInt(&saved,100);
 MemFile_WriteCString(&saved,(char*)"fixture_def");MemFile_WriteInt(&saved,1);reject(saved);
 // Process-specific effect/debris owners must be rejected, never copied.
 for(int field=0;field<5;++field) {
  reset();definition.numPieces=1;
  auto &d=s_destructibles[0];d.entNum=100;d.ddef=&definition;d.pieceCount=1;d.flags=2;
  d.pieceArray=(DESTRUCTIBLE_PIECE_INFO*)Hunk_UserAlloc(nullptr,sizeof(DESTRUCTIBLE_PIECE_INFO),4,nullptr);
  g_entities[100].r.inuse=1;g_entities[100].destructible=&d;s_numDestructibles=1;
  if(field==0)d.pieceArray[0].xdollHandle=91;
  if(field==1)d.pieceArray[0].fx=91;
  if(field==2)d.pieceArray[0].burnData.fx=91;
  if(field==3)d.pieceArray[0].burnData.sndId=91;
  if(field==4)d.destructiblePoseID=1;
  bool failed=false;saved={};try{Destructible_WriteSaveState(&saved);}catch(const std::runtime_error&){failed=true;}
  check(failed);
  // Fixture owns the rejected handles; clear them before normal free (which
  // deliberately aborts if asked to destroy a fabricated native handle).
  std::memset(d.pieceArray,0,sizeof(*d.pieceArray));d.destructiblePoseID=0;cleanup();
 }
 std::printf("destructible save %u checks PASS\n",checks);
}
"""
a.out.mkdir(parents=True,exist_ok=True)
(a.out/'fixture.cpp').write_text(fixture.replace('ACTUAL_FREE',free),encoding='utf-8')
(a.out/'sources.json').write_text(json.dumps({f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in ['src/physics/destructible_save.inl','src/physics/destructible.cpp','src/physics/destructible.h']},indent=2)+'\n',encoding='utf-8')
