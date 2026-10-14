// Native counterparts of the original win_main headless diagnostic owners.
// No remote userspace stack is invented: the observer reports actual proc data.
#include "headless_diagnostics.h"
#include "headless_policy.h"
#include "thread_services.h"
#include <platform/startup_services_api.h>
#include <universal/dvar.h>
#include <game_mp/g_main_mp.h>
#include <client/client.h>
#include <qcommon/common.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <climits>
#include <cstdint>

namespace {
using Clock=std::chrono::steady_clock;
std::atomic<unsigned> framePerf{},hitchThreshold{};
std::atomic<uint64_t> frameSequence{},frameStart{};
std::mutex mutex;
std::condition_variable changed;
std::thread observer;
bool configured{},returned{},stopping{};
std::atomic<bool> registered{};
pid_t ownerPid{};
Clock::time_point origin;
const dvar_t *autoQuit,*quitNotify,*dvarDump;
int previousConn=-2,previousSv=-2,previousPm=-2,previousHud=-2,hudPm=-1;
bool hudDrawn{};
uint64_t notifySeen{};
int64_t notifyAt=-1;
unsigned threadDumpDelay{};
uint64_t Now(){return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();}
[[noreturn]] void Fail(const char *what){Com_Error(ERR_FATAL,"Native diagnostics: %s",what);std::abort();}
void MainOwner(){if(!Sys_IsMainThread() || ownerPid!=getpid())Fail("main process/role owner required");}
void ReadProc(pid_t tid,const char *leaf){
    char path[128],text[2048];std::snprintf(path,sizeof(path),"/proc/self/task/%d/%s",tid,leaf);
    const int fd=open(path,O_RDONLY|O_CLOEXEC);if(fd<0){std::fprintf(stderr,"BO1_HEADLESS PROC tid %d %s unavailable errno %d\n",tid,leaf,errno);return;}
    const ssize_t count=read(fd,text,sizeof(text)-1);const int failure=errno;close(fd);
    if(count<0){std::fprintf(stderr,"BO1_HEADLESS PROC tid %d %s read unavailable errno %d\n",tid,leaf,failure);return;}
    text[count]=0;std::fprintf(stderr,"BO1_HEADLESS PROC tid %d %s %s%s",tid,leaf,text,count && text[count-1]=='\n'?"":"\n");
}
void DumpThreads(unsigned pass){
    std::fprintf(stderr,"BO1_HEADLESS THREADDUMP %u monotonic_ms %llu; remote userspace stacks unavailable (no native suspension/context owner)\n",pass,static_cast<unsigned long long>(Now()));
    DIR *directory=opendir("/proc/self/task");if(!directory){std::fprintf(stderr,"BO1_HEADLESS task enumeration unavailable errno %d\n",errno);return;}
    while(dirent *entry=readdir(directory)){char *end;const long tid=std::strtol(entry->d_name,&end,10);if(*end || tid<=0 || tid>INT_MAX)continue;ReadProc(static_cast<pid_t>(tid),"stat");ReadProc(static_cast<pid_t>(tid),"wchan");}
    closedir(directory);
}
void Watch(){
    uint64_t dumpedSequence=0;unsigned dumps=0,total=0,scheduled=0;
    const uint64_t start=Now();
    for(;;){
        {std::unique_lock lock(mutex);if(changed.wait_for(lock,std::chrono::milliseconds(10),[]{return stopping;}))return;}
        if(threadDumpDelay){const uint64_t age=Now()-start;if(scheduled<2 && age>=static_cast<uint64_t>(threadDumpDelay)+10000u*scheduled){DumpThreads(++scheduled);}if(scheduled==2)return;continue;}
        const uint64_t sequence=frameSequence.load(std::memory_order_acquire);
        if(!(sequence&1) || total>=240)continue;
        if(sequence!=dumpedSequence)dumps=0;
        const uint64_t age=Now()-frameStart.load(std::memory_order_acquire);
        if(dumps>=4 || age<static_cast<uint64_t>(hitchThreshold.load())+200u*dumps || sequence!=frameSequence.load(std::memory_order_acquire))continue;
        dumpedSequence=sequence;++dumps;++total;
        std::fprintf(stderr,"BO1_HEADLESS HITCH frame %llu age %llu ms dump %u\n",static_cast<unsigned long long>((sequence+1)/2),static_cast<unsigned long long>(age),dumps);
        DumpThreads(total);
        std::fprintf(stderr,"BO1_HEADLESS HITCH frame %llu still_inside %d\n",static_cast<unsigned long long>((sequence+1)/2),sequence==frameSequence.load(std::memory_order_acquire));
    }
}
void DumpDvar(const dvar_s *dvar,void*){Com_Printf(16,"bo1_dvardump\t%s\t%d\t0x%x\t%s\t%s\n",dvar->name,dvar->type,dvar->flags,Dvar_DisplayableResetValue(dvar),Dvar_DisplayableValue(dvar));}
}
namespace bo1::android {
void ConfigureDiagnostics(int perf,int hitch,int dump){
    if(!Sys_IsMainThread())Fail("configuration requires actual main role");
    if(perf<0 || hitch<0 || dump<0)Fail("negative diagnostic threshold");
    bool repeated;{std::lock_guard lock(mutex);repeated=configured;if(!repeated){configured=true;ownerPid=getpid();}}
    if(repeated)Fail("configuration cannot be repeated");
    framePerf.store(static_cast<unsigned>(perf));threadDumpDelay=Sys_IsHeadless()?static_cast<unsigned>(dump):0;
    hitchThreshold.store(Sys_IsHeadless() && !threadDumpDelay?static_cast<unsigned>(hitch):0);
}
void RegisterDiagnosticsDvars(){
    MainOwner();if(!configured || registered)Fail("Dvar registration lifecycle invalid");
    autoQuit=_Dvar_RegisterInt("bo1_autoquit",0,0,INT_MAX,0,"quit this many ms after actual init return (0 = off)");
    quitNotify=_Dvar_RegisterString("bo1_quitnotify","",0,"headless: quit level ms after name,count,delay level notify (empty = off)");
    dvarDump=_Dvar_RegisterBool("bo1_dvardump",false,0,"print Dvars when bo1_autoquit fires");
    registered=true;
}
void DiagnosticsInitializationReturned(){
    MainOwner();bool invalid;
    {std::lock_guard lock(mutex);invalid=!registered || returned || stopping;if(!invalid){origin=Clock::now();returned=true;}}
    if(invalid)Fail("actual initialization return publication invalid");
    if(hitchThreshold.load() || threadDumpDelay){try{observer=std::thread(Watch);}catch(...){Fail("observer thread creation failed");}}
}
void StopDiagnostics(){
    MainOwner();{std::lock_guard lock(mutex);stopping=true;}changed.notify_all();
    if(observer.joinable())observer.join();
}
void DiagnosticsFrame(){
    MainOwner();int pm,hud;int64_t at;uint64_t seen,elapsed;
    {std::lock_guard lock(mutex);if(!returned || stopping)return;elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-origin).count();pm=hudPm;hud=hudDrawn;at=notifyAt;seen=notifySeen;}
    if(!com_sv_running)Fail("server running Dvar owner absent after initialization");
    const int sv=com_sv_running->current.enabled;
    if(Sys_IsHeadless()){
        const int connection=Sys_IsHeadlessClient()?static_cast<int>(CL_GetLocalClientConnectionState(0)):-1;
        if(connection!=CA_ACTIVE){std::lock_guard lock(mutex);hudPm=-1;hudDrawn=false;pm=-1;hud=0;}
        if(connection!=previousConn || sv!=previousSv || pm!=previousPm || hud!=previousHud){previousConn=connection;previousSv=sv;previousPm=pm;previousHud=hud;Com_Printf(16,"bo1_timeline: %llu ms after init; connection state %d, sv_running %d, pm_type %d, hud %d, level.time %d\n",static_cast<unsigned long long>(elapsed),connection,sv,pm,hud,sv?level.time:0);}
    }
    if(at>=0 && sv && static_cast<int64_t>(level.time)>=at){
        {std::lock_guard lock(mutex);notifyAt=-1;}
        Com_Printf(16,"bo1_quitnotify: %llu ms after init; level.time %d, %llu '%s' notifies. Quitting.\n",static_cast<unsigned long long>(elapsed),level.time,static_cast<unsigned long long>(seen),quitNotify->current.string);Com_Quit_f();return;
    }
    if(autoQuit->current.integer>0 && elapsed>=static_cast<unsigned>(autoQuit->current.integer)){
        Com_Printf(16,"bo1_autoquit: %llu ms after init; sv_running %d, mapname '%s', level.time %d. Quitting.\n",static_cast<unsigned long long>(elapsed),sv,Dvar_GetString("mapname"),sv?level.time:0);
        if(dvarDump->current.enabled)Dvar_ForEach(DumpDvar,nullptr);Com_Quit_f();
    }
}
}
unsigned int Sys_FramePerfHitchMs(){return framePerf.load(std::memory_order_acquire);}
unsigned int Sys_HitchWatchBegin(){
    if(!hitchThreshold.load())return 0;MainOwner();
    {std::lock_guard lock(mutex);if(!returned || stopping)return 0;}
    if(frameSequence.load()&1)Fail("nested watched client frame");
    frameStart.store(Now(),std::memory_order_relaxed);const auto sequence=frameSequence.fetch_add(1,std::memory_order_release)+1;
    return static_cast<unsigned>((sequence+1)/2);
}
void Sys_HitchWatchEnd(){if(!hitchThreshold.load())return;MainOwner();if(frameSequence.load()&1)frameSequence.fetch_add(1,std::memory_order_release);}
void Sys_HeadlessNoteHud(int pm,bool drawn){std::lock_guard lock(mutex);hudPm=pm;hudDrawn=drawn;}
void Sys_HeadlessTimeline(const char *what){
    if(!Sys_IsHeadless() || !what)return;uint64_t elapsed;
    {std::lock_guard lock(mutex);if(!returned || stopping)return;elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-origin).count();}
    Com_Printf(16,"bo1_timeline: %llu ms after init; %s\n",static_cast<unsigned long long>(elapsed),what);
}
void Sys_HeadlessLevelNotify(const char *name){
    if(!Sys_IsHeadless() || !registered || !name || !quitNotify->current.string[0])return;
    char spec[128],want[64]{};std::snprintf(spec,sizeof(spec),"%s",quitNotify->current.string);
    for(char *p=spec;*p;++p)if(*p==',')*p=' ';
    int count=1,delay=0;if(std::sscanf(spec,"%63s %d %d",want,&count,&delay)<1 || std::strcmp(name,want))return;
    uint64_t seen;int64_t at;
    {std::lock_guard lock(mutex);if(stopping || notifyAt>=0)return;seen=++notifySeen;if(count>0 && seen<static_cast<unsigned>(count))return;at=static_cast<int64_t>(level.time)+(delay>0?delay:0);notifyAt=at;}
    Com_Printf(16,"bo1_quitnotify: level.time %d, level notify '%s' #%llu; quitting at level.time %lld\n",level.time,name,static_cast<unsigned long long>(seen),static_cast<long long>(at));
}
