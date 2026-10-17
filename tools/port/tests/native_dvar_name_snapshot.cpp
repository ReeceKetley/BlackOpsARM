#include "universal/dvar.h"
#include "platform/fast_critical_section.h"
#include "universal/com_hash.h"
#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
extern const dvar_s *sortedDvars[4096];extern dvar_s *dvarHashTable[1024];
extern dvar_s dvarPool[4096];extern int dvarCount;extern FastCriticalSection g_dvarCritSect;
static unsigned checks;static std::vector<std::string> names;static bool mutate;
static void Check(bool ok){++checks;if(!ok)std::abort();}
bool Assert_MyHandler(const char*,int,int,const char*,...){std::abort();}
int I_stricmp(const char* a,const char* b){return strcasecmp(a,b);}
int Com_HashString(const char* name,int){unsigned h=0;while(*name)h=h*31+static_cast<unsigned char>(*name++);return int(h);}
static void Publish(unsigned i,const char* name,unsigned flags=0){
 auto& d=dvarPool[i];d.name=name;d.flags=flags;d.type=DVAR_TYPE_BOOL;d.hash=Com_HashString(name,0);
 d.hashNext=dvarHashTable[d.hash&1023];dvarHashTable[d.hash&1023]=&d;sortedDvars[i]=&d;dvarCount=int(i+1);
}
static char betaName[]="Beta";
static void Visit(const char* name){
 Check(name&&*name);names.emplace_back(name);
 Check(Dvar_FindVar("Beta")==&dvarPool[0]); // actual reentrant production lookup
 if(mutate&&names.size()==1){
  Sys_LockWrite(&g_dvarCritSect);std::memcpy(betaName,"gone",5);Publish(3,"Later");Sys_UnlockWrite(&g_dvarCritSect);
 }
}
int main(){
 Sys_LockWrite(&g_dvarCritSect);Publish(0,betaName,0x10000);Publish(1,"zeta");Sys_UnlockWrite(&g_dvarCritSect);
 std::atomic<bool> partial{},finish{};
 std::thread writer([&]{
  Sys_LockWrite(&g_dvarCritSect);
  // Exact RegisterNew publication order: slot/count become visible before
  // name initialization, all under the production writer lock.
  sortedDvars[2]=&dvarPool[2];dvarCount=3;partial=true;
  while(!finish.load())std::this_thread::yield();
  dvarPool[2].name="Alpha";dvarPool[2].flags=0x10000;
  Sys_UnlockWrite(&g_dvarCritSect);
 });
 while(!partial.load())std::this_thread::yield();
 // The previous unlocked enumeration consumes a null name at this point.
 Check(sortedDvars[2]->name==nullptr);
 std::thread reader([]{Dvar_ForEachName(Visit);});
 while(!bo1::fcs::LoadRead(&g_dvarCritSect))std::this_thread::yield();
 Check(names.empty());finish=true;writer.join();reader.join();
 Check(names==std::vector<std::string>({"Alpha","Beta","zeta"}));
 names.clear();mutate=true;Dvar_ForEachName(Visit);
 Check(names==std::vector<std::string>({"Alpha","Beta","zeta"})); // owned snapshot survives callback mutation
 mutate=false;names.clear();Dvar_ForEachName(Visit);
 Check(names==std::vector<std::string>({"Alpha","gone","Later","zeta"}));
 Sys_LockWrite(&g_dvarCritSect);Publish(4,"con_restricted_access");dvarPool[4].current.enabled=true;Sys_UnlockWrite(&g_dvarCritSect);
 names.clear();Dvar_ForEachConsoleAccessName(Visit);Check(names==std::vector<std::string>({"Alpha","gone"}));
 std::printf("native_dvar_name_snapshot=PASS checks=%u partial_publication_blocked=1 old_unlocked_null_reproduced=1 reentrant_lookup_registration=1 owned_names_sorted_exact_once=1 restricted=1\n",checks);
}
