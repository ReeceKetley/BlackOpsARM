#include <win32/win_local.h>
#include <platform/critical_section_services.h>
#include <platform/android/zone_heap.h>
#include <universal/memory_services.h>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <thread>
void Sys_QueEvent(unsigned,sysEventType_t,int,int,int,void *);
sysEvent_t *Sys_GetEvent(sysEvent_t *);
void Sys_ShutdownEvents();
void NET_Sleep(unsigned);
extern unsigned eventHead,eventTail;
static unsigned overflows;
void Com_Printf(int,const char *,...){++overflows;} // Only overflow diagnostics used.
void Com_Quit_f(){std::abort();} // Unexpected terminal engine shutdown fails this probe.
static void check(bool ok,const char *name){if(!ok){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}std::printf("PASS %s\n",name);}
int main(){
    Sys_InitializeCriticalSections();sysEvent_t event;
    auto baseline=bo1::android::ZoneAllocationCount();
    for(unsigned i=0;i<270;++i){auto *payload=Z_Malloc(4,"event probe",11);*payload=i;Sys_QueEvent(100+i,SE_CONSOLE,i,0,4,payload);}
    check(overflows==14 && bo1::android::ZoneAllocationCount()==baseline+256,"overflow retires oldest owned payloads");
    for(unsigned i=14;i<270;++i){check(Sys_GetEvent(&event)==&event && event.evType==SE_CONSOLE && event.evValue==static_cast<int>(i) && *static_cast<unsigned *>(event.evPtr)==i,"queued event and payload order");Z_Free(static_cast<char *>(event.evPtr),11);}
    check(bo1::android::ZoneAllocationCount()==baseline,"popped event ownership transfers to caller");
    Sys_GetEvent(&event);check(event.evType==SE_NONE && !event.evPtr,"empty queue reports actual absence");
    eventHead=eventTail=UINT_MAX-2;
    for(int i=0;i<6;++i)Sys_QueEvent(1,SE_KEY,i,1,0,nullptr);
    for(int i=0;i<6;++i){Sys_GetEvent(&event);check(event.evType==SE_KEY && event.evValue==i,"counter wrap preserves FIFO");}
    bool seen[128]{};std::thread producers[4];
    for(int p=0;p<4;++p)producers[p]=std::thread([p]{for(int i=0;i<32;++i)Sys_QueEvent(1,SE_CHAR,p*32+i,0,0,nullptr);});
    for(auto &producer:producers)producer.join();
    for(int i=0;i<128;++i){Sys_GetEvent(&event);check(event.evType==SE_CHAR && event.evValue>=0 && event.evValue<128 && !seen[event.evValue],"concurrent producers preserve distinct events");seen[event.evValue]=true;}
    for(int i=0;i<3;++i)Sys_QueEvent(1,SE_CONSOLE,0,0,4,Z_Malloc(4,"shutdown probe",11));
    Sys_ShutdownEvents();Sys_ShutdownEvents();check(bo1::android::ZoneAllocationCount()==baseline,"shutdown drains payloads exactly once");
    auto start=std::chrono::steady_clock::now();NET_Sleep(10);
    check(std::chrono::steady_clock::now()-start>=std::chrono::milliseconds(10),"network wait uses actual OS sleep");NET_Sleep(0);
}
