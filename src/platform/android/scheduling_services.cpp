#include "scheduling_services.h"
#include "thread_services.h"
#include <array>
#include <atomic>
#include <mutex>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>
#if !defined(__ANDROID__) || !defined(__aarch64__)
#error Native scheduling owner requires Android ARM64
#endif
void Sys_Error(char *,...);
namespace {
struct Slot {
    pid_t tid{};
    cpu_set_t baseline{},lastWanted{};
    int startNice{},lastNice{},lastError{};
    bool attempted{};
};
std::mutex mutex;
std::array<Slot,THREAD_CONTEXT_COUNT> slots{};
std::array<std::atomic<signed char>,CPU_SETSIZE> classes{};
cpu_set_t initialAllowed{},fastest{};
std::atomic<bool> classified{};
std::atomic<pid_t> processOwner{};
bool topologyInitialized{},heterogeneous{};
int priorityMode{},pcoreMode{};
unsigned lockDepth{};
[[noreturn]] void fail(const char *why,int error=0) {
    Sys_Error(const_cast<char *>("Native scheduling: %s (%d)"),why,error);
    std::abort();
}
void requireMain(){if(!Sys_IsMainThread() || processOwner.load(std::memory_order_acquire)!=getpid())fail("controls require actual main/process owner");}
bool equal(const cpu_set_t &a,const cpu_set_t &b){return CPU_EQUAL(&a,&b);}
int niceValue(pid_t tid,int *value){errno=0;const int result=getpriority(PRIO_PROCESS,tid);if(errno)return errno;*value=result;return 0;}
bool capacity(int cpu,unsigned long *value) {
    char path[128];std::snprintf(path,sizeof(path),"/sys/devices/system/cpu/cpu%d/cpu_capacity",cpu);
    const int fd=open(path,O_RDONLY|O_CLOEXEC);if(fd<0)return false;
    char text[64]{};const ssize_t count=read(fd,text,sizeof(text)-1);const int error=errno;close(fd);
    if(count<=0 || count==sizeof(text)-1){errno=error;return false;}
    char *end;errno=0;const auto parsed=std::strtoul(text,&end,10);
    if(errno || end==text || !parsed)return false;
    while(*end==' ' || *end=='\n' || *end=='\t')++end;
    if(*end)return false;*value=parsed;return true;
}
void topology(const cpu_set_t &allowed) {
    if(topologyInitialized)return;topologyInitialized=true;initialAllowed=allowed;CPU_ZERO(&fastest);
    for(auto &entry:classes)entry.store(-1,std::memory_order_relaxed);
    std::array<unsigned long,CPU_SETSIZE> values{};
    unsigned long low=ULONG_MAX,high=0;
    for(int cpu=0;cpu<CPU_SETSIZE;++cpu)if(CPU_ISSET(cpu,&allowed)){
        if(!capacity(cpu,&values[cpu])){classified.store(true,std::memory_order_release);return;}
        if(values[cpu]<low)low=values[cpu];if(values[cpu]>high)high=values[cpu];
    }
    heterogeneous=high && low<high;
    if(heterogeneous)for(int cpu=0;cpu<CPU_SETSIZE;++cpu)if(CPU_ISSET(cpu,&allowed)){
        const bool fast=values[cpu]==high;classes[cpu].store(fast?1:0,std::memory_order_relaxed);
        if(fast)CPU_SET(cpu,&fastest);
    }
    classified.store(true,std::memory_order_release);
}
bool pcoreRole(int role){return heterogeneous && ((pcoreMode>=1 && role==THREAD_CONTEXT_SERVER)
    || (pcoreMode>=2 && (role==THREAD_CONTEXT_MAIN || role==THREAD_CONTEXT_BACKEND || role==THREAD_CONTEXT_WORKER0 || role==THREAD_CONTEXT_WORKER1)));}
int lockedCpu(int role) {
    const int count=CPU_COUNT(&initialAllowed);if(!lockDepth || count<2 || count>4)return -1;
    int allowed[4],n=0;for(int cpu=0;cpu<CPU_SETSIZE;++cpu)if(CPU_ISSET(cpu,&initialAllowed))allowed[n++]=cpu;
    if(role==THREAD_CONTEXT_MAIN)return allowed[0];
    if(role==THREAD_CONTEXT_BACKEND)return allowed[count-1];
    if(role==THREAD_CONTEXT_DATABASE)return allowed[1];
    if(role==THREAD_CONTEXT_WORKER0 && count>=3)return allowed[1];
    if(role==THREAD_CONTEXT_WORKER1 && count>=4)return allowed[2];
    return -1; // Original >4-CPU policy uses unrestricted inherited defaults.
}
int desired(int role,const Slot &slot,cpu_set_t *affinity,int *nice) {
    *affinity=slot.baseline;
    const int pinned=lockedCpu(role);
    if(pinned>=0){CPU_ZERO(affinity);if(CPU_ISSET(pinned,&slot.baseline))CPU_SET(pinned,affinity);}
    if(pcoreRole(role))for(int cpu=0;cpu<CPU_SETSIZE;++cpu)if(!CPU_ISSET(cpu,&fastest))CPU_CLR(cpu,affinity);
    const int boost=(priorityMode==2?10:priorityMode==1?5:0)+(pcoreRole(role) && role==THREAD_CONTEXT_SERVER?5:0);
    *nice=slot.startNice-boost;if(*nice<-20)*nice=-20;
    return CPU_COUNT(affinity)?0:EINVAL;
}
// sched_getaffinity reports the requested mask ANDed with currently active
// CPUs. Qualcomm core_ctl pauses big cores under light load (e.g. during
// loading), so that value is transient. Observe the thread's actual requested
// mask instead; restoring or narrowing from a transient value would exclude
// paused cores permanently.
int affinityValue(pid_t tid,cpu_set_t *value) {
    char path[64];std::snprintf(path,sizeof(path),"/proc/self/task/%d/status",int(tid));
    const int fd=open(path,O_RDONLY|O_CLOEXEC);if(fd<0)return errno;
    char text[4096];std::size_t used=0;ssize_t count;
    while(used<sizeof(text)-1 && (count=read(fd,text+used,sizeof(text)-1-used))>0)used+=std::size_t(count);
    const int error=count<0?errno:0;close(fd);if(error)return error;text[used]=0;
    const char *line=std::strstr(text,"\nCpus_allowed_list:");if(!line)return ENODATA;
    const char *cursor=line+std::strlen("\nCpus_allowed_list:");
    cpu_set_t result;CPU_ZERO(&result);
    while(*cursor==' ' || *cursor=='\t')++cursor;
    for(;;) {
        char *end;errno=0;const unsigned long first=std::strtoul(cursor,&end,10);
        if(errno || end==cursor || first>=CPU_SETSIZE)return EINVAL;
        unsigned long last=first;cursor=end;
        if(*cursor=='-') {++cursor;errno=0;last=std::strtoul(cursor,&end,10);if(errno || end==cursor || last<first || last>=CPU_SETSIZE)return EINVAL;cursor=end;}
        for(unsigned long cpu=first;cpu<=last;++cpu)CPU_SET(cpu,&result);
        if(*cursor!=',')break;++cursor;
    }
    if(*cursor!='\n' && *cursor!=0)return EINVAL;
    if(!CPU_COUNT(&result))return EINVAL;
    *value=result;return 0;
}
int applyAffinity(pid_t tid,const cpu_set_t &wanted) {
    cpu_set_t before{},after{};int error=affinityValue(tid,&before);if(error)return error;
    if(equal(before,wanted))return 0;
    if(sched_setaffinity(tid,sizeof(wanted),&wanted))return errno;
    error=affinityValue(tid,&after);if(!error && equal(after,wanted))return 0;
    // Kernel cpuset/hotplug can silently narrow a request; never certify it.
    if(sched_setaffinity(tid,sizeof(before),&before))return errno;
    return error?error:EAGAIN;
}
int applyNice(pid_t tid,int wanted) {
    int observed;int error=niceValue(tid,&observed);if(error)return error;
    if(observed==wanted)return 0;
    if(setpriority(PRIO_PROCESS,tid,wanted))return errno;
    error=niceValue(tid,&observed);return error?error:observed==wanted?0:EAGAIN;
}
int refresh(int role) {
    auto &slot=slots[role];if(!slot.tid)return 0;
    cpu_set_t wanted{};int nice;const int invalid=desired(role,slot,&wanted,&nice);
    if(slot.attempted && equal(wanted,slot.lastWanted) && nice==slot.lastNice)return 0;
    slot.attempted=true;slot.lastWanted=wanted;slot.lastNice=nice;
    const int affinityError=invalid?invalid:applyAffinity(slot.tid,wanted);
    const int niceError=applyNice(slot.tid,nice);
    slot.lastError=affinityError?affinityError:niceError;
    return slot.lastError;
}
void refreshAll(const char *operation) {
    int firstError=0,role=-1;
    {std::lock_guard<std::mutex> lock(mutex);for(int i=0;i<THREAD_CONTEXT_COUNT;++i){const int error=refresh(i);if(error && !firstError){firstError=error;role=i;}}}
    if(firstError)std::fprintf(stderr,"Native scheduling %s unavailable for role%d: %s (%d); actual state retained, no applied-success claim\n",operation,role,std::strerror(firstError),firstError);
}
}
void bo1::android::RegisterSchedulingThread(ThreadContext_t role) {
    if(role<0 || role>=THREAD_CONTEXT_COUNT || Sys_GetThreadContext()!=role)fail("registration requires actual TLS owner");
    const auto process=processOwner.load(std::memory_order_acquire);
    if((process && process!=getpid()) || (!process && role!=THREAD_CONTEXT_MAIN))fail("register main process owner before scheduling workers");
    int error=0,applyError=0;const pid_t tid=gettid();
    {std::lock_guard<std::mutex> lock(mutex);auto &slot=slots[role];
        if(slot.tid){if(slot.tid!=tid)error=EEXIST;}
        else {
            cpu_set_t inherited{};int inheritedNice{};
            error=affinityValue(tid,&inherited);if(!error)error=niceValue(tid,&inheritedNice);
            if(!error && !CPU_COUNT(&inherited))error=EINVAL;
            if(!error){slot.tid=tid;slot.baseline=inherited;slot.startNice=inheritedNice;if(role==THREAD_CONTEXT_MAIN){topology(inherited);processOwner.store(getpid(),std::memory_order_release);}applyError=refresh(role);}
        }
    }
    if(error)fail("capture inherited thread scheduling",error);
    if(applyError)std::fprintf(stderr,"Native scheduling unavailable for newly registered role%d: %s (%d); no applied-success claim\n",role,std::strerror(applyError),applyError);
}
void bo1::android::RetireSchedulingThread(ThreadContext_t role) {
    if(role<=THREAD_CONTEXT_MAIN || role>=THREAD_CONTEXT_COUNT || Sys_GetThreadContext()!=role)fail("retirement requires actual non-main owner");
    int error=0;
    {std::lock_guard<std::mutex> lock(mutex);auto &slot=slots[role];
        if(slot.tid!=gettid())error=ESRCH;
        else {error=applyAffinity(slot.tid,slot.baseline);const int niceError=applyNice(slot.tid,slot.startNice);if(!error)error=niceError;slot={};}
    }
    if(error)fail("restore/retire actual scheduling owner",error);
}
bool bo1::android::ReadSchedulingState(ThreadContext_t role,SchedulingState *output) {
    requireMain();if(role<0 || role>=THREAD_CONTEXT_COUNT || !output)fail("invalid scheduling observation");
    SchedulingState result{};int error=0;
    {std::lock_guard<std::mutex> lock(mutex);const auto &slot=slots[role];if(!slot.tid)return false;
        result.tid=slot.tid;result.inheritedAffinity=slot.baseline;result.inheritedNice=slot.startNice;result.lastError=slot.lastError;
        error=affinityValue(slot.tid,&result.currentAffinity);if(!error)error=niceValue(slot.tid,&result.currentNice);
        result.affinityChanged=!equal(result.currentAffinity,result.inheritedAffinity);result.niceChanged=result.currentNice!=result.inheritedNice;
    }
    if(error)fail("observe live scheduling owner",error);*output=result;return true;
}
void Sys_ModApplyPriority(int mode) {
    requireMain();if(mode<0 || mode>2)fail("priority mode outside0..2");
    {std::lock_guard<std::mutex> lock(mutex);priorityMode=mode;}
    refreshAll("engine-thread nice priority");
}
void Sys_ModApplyPCore(int mode) {
    requireMain();if(mode<0 || mode>2)fail("pcore mode outside0..2");
    bool unavailable;
    {std::lock_guard<std::mutex> lock(mutex);unavailable=mode && !heterogeneous && mode!=pcoreMode;pcoreMode=mode;}
    if(unavailable)std::fputs("Native pcore topology unavailable/uniform; no heterogeneous placement applied\n",stderr);
    refreshAll("fastest allowed kernel capacity affinity");
}
int Sys_ModOnPCore() {
    if(processOwner.load(std::memory_order_acquire)!=getpid() || !classified.load(std::memory_order_acquire))return -1;
    const int cpu=sched_getcpu();return cpu>=0 && cpu<CPU_SETSIZE?classes[cpu].load(std::memory_order_relaxed):-1;
}
void Win_LockThreadAffinity() {
    requireMain();bool invalid;
    {std::lock_guard<std::mutex> lock(mutex);invalid=lockDepth==UINT_MAX;if(!invalid)++lockDepth;}
    if(invalid)fail("affinity lock nesting overflow");
    refreshAll("original profiling affinity lock");
}
void Win_UnlockThreadAffinity() {
    requireMain();bool invalid;
    {std::lock_guard<std::mutex> lock(mutex);invalid=lockDepth==0;if(!invalid)--lockDepth;}
    if(invalid)fail("unbalanced affinity unlock");refreshAll("inherited affinity restoration");
}
