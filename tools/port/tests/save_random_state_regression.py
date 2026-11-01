"""Actual pending RNG codec and RNG bodies; isolate byte IO and engine thread boundary."""
from pathlib import Path
import argparse,json,hashlib
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
def body(path,sig):
 s=(root/path).read_bytes().decode('cp1252');start=s.index(sig);b=s.index('{',start);e=b+1;depth=1
 while depth:depth+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[start:e]
actual=body('src/game_mp/g_utils_mp.cpp','int __cdecl G_rand()')+'\n'+body('src/universal/com_math.cpp','int __cdecl irand(int min, int max)')
fixture=r"""
#include <qcommon/common.h>
#include <universal/memfile.h>
#include <platform/android/engine_random.h>
#include <thread>
#include <vector>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <cstring>
unsigned int holdrand,holdrand_0;
static unsigned checks;
static thread_local bool gameThread;
static void check(bool v){if(!v)std::abort();++checks;}
void Com_Error(errorParm_t,const char*,...){throw std::runtime_error("expected diagnostic error");}
void Com_Printf(int,const char*,...){}
bool Assert_MyHandler(const char*,int,int,const char*,...){std::abort();}
void SV_CheckThread(){if(!gameThread)Com_Error(ERR_DROP,"wrong thread");}
void MemFile_WriteInt(MemoryFile*f,int n){MemFile_WriteData(f,4,(unsigned char*)&n);}
void MemFile_WriteDataInternal(MemoryFile*,unsigned int,unsigned char*){std::abort();}
void MemFile_ReadData(MemoryFile*f,int n,unsigned char*p){if(n<0 || f->bytesUsed+n>f->cacheBufferUsed)Com_Error(ERR_DROP,"bounds");std::memcpy(p,f->cacheBuffer+f->bytesUsed,n);f->bytesUsed+=n;}
ACTUAL_RNG
#include <game_sp/g_sp_save_random.inl>
int main(){
 static MemoryFile saved,again,bad;
 std::vector<unsigned int> seeds={0,1,0x7fffffff,0x80000000,0xffffffff};
 for(unsigned i=0;i<251;++i)seeds.push_back(i*2654435761u);
 for(unsigned seed:seeds){
  G_SP_ClearPendingSaveRandomState();saved={};
  int expected[32][3];
  std::thread capture([&]{gameThread=true;holdrand=seed^0xa5a5a5a5u;holdrand_0=~seed;bo1::android::CrtSrand(seed);
   G_WriteSaveRandomState(&saved,false);
   for(auto &row:expected){row[0]=G_rand();row[1]=irand(-1200,100000);row[2]=bo1::android::CrtRand();}
  });capture.join();
  holdrand=125;holdrand_0=126;bo1::android::CrtSrand(0x12345678);unsigned loader=bo1::android::CrtGetState();
  saved.bytesUsed=0;G_ReadSaveRandomState(&saved);
  check(holdrand==125 && holdrand_0==126 && bo1::android::CrtGetState()==loader);
  again={};G_WriteSaveRandomState(&again,true);
  check(again.cacheBufferUsed==saved.cacheBufferUsed && !std::memcmp(again.cacheBuffer,saved.cacheBuffer,saved.cacheBufferUsed));
  // Wrong thread cannot consume it; the genuine simulation owner still can.
  bool denied=false;try{G_SP_ApplyPendingSaveRandomState();}catch(const std::runtime_error&){denied=true;}check(denied);
  std::thread consume([&]{gameThread=true;bo1::android::CrtSrand(999);
   G_SP_ApplyPendingSaveRandomState();
   for(auto &row:expected){check(G_rand()==row[0]);check(irand(-1200,100000)==row[1]);check(bo1::android::CrtRand()==row[2]);}
   unsigned g=holdrand,m=holdrand_0,c=bo1::android::CrtGetState();G_SP_ApplyPendingSaveRandomState();
   check(g==holdrand && m==holdrand_0 && c==bo1::android::CrtGetState());
  });consume.join();check(bo1::android::CrtGetState()==loader);
  // World replacement cancels an unconsumed staged load.
  saved.bytesUsed=0;G_ReadSaveRandomState(&saved);G_SP_ClearPendingSaveRandomState();
  std::thread cancelled([&]{gameThread=true;holdrand=101;holdrand_0=102;bo1::android::CrtSrand(103);G_SP_ApplyPendingSaveRandomState();check(holdrand==101 && holdrand_0==102 && bo1::android::CrtGetState()==103);});cancelled.join();
 }
 saved.bytesUsed=0;G_ReadSaveRandomState(&saved);saved.bytesUsed=0;bool duplicate=false;
 try{G_ReadSaveRandomState(&saved);}catch(const std::runtime_error&){duplicate=true;}check(duplicate);G_SP_ClearPendingSaveRandomState();
 for(int offset : {0,4}){bad=saved;bad.bytesUsed=0;unsigned invalid=2;std::memcpy(bad.cacheBuffer+offset,&invalid,4);bool rejected=false;try{G_ReadSaveRandomState(&bad);}catch(const std::runtime_error&){rejected=true;}check(rejected);}
 bad=saved;bad.bytesUsed=0;unsigned flags=0;std::memcpy(bad.cacheBuffer+4,&flags,4);bool platform=false;try{G_ReadSaveRandomState(&bad);}catch(const std::runtime_error&){platform=true;}check(platform);
 bad=saved;bad.bytesUsed=0;bad.cacheBufferUsed=12;bool truncated=false;try{G_ReadSaveRandomState(&bad);}catch(const std::runtime_error&){truncated=true;}check(truncated);
 std::printf("save random %u checks PASS\n",checks);
}
"""
a.out.mkdir(parents=True,exist_ok=True);(a.out/'fixture.cpp').write_text(fixture.replace('ACTUAL_RNG',actual),encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps({f:hashlib.sha256((root/f).read_bytes()).hexdigest() for f in ['src/game_sp/g_sp_save_random.inl','src/platform/android/engine_random.cpp','src/platform/android/engine_random.h','src/game_mp/g_utils_mp.cpp','src/universal/com_math.cpp']},indent=2)+'\n',encoding='utf-8')
