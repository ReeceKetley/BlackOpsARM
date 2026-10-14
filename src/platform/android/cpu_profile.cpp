#include "cpu_profile.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <limits>
#include <new>
#include <time.h>
#include <unistd.h>
namespace bo1::android {
struct CpuThread { int directory; unsigned tid; std::uint64_t start; };
namespace {
struct Stat { std::uint64_t ticks{}, start{}; char name[256]{}; };
bool readStat(int directory, unsigned tid, Stat *out) {
    int fd=openat(directory,"stat",O_RDONLY|O_CLOEXEC);
    if(fd<0)return false;
    char buffer[4096]; ssize_t size;
    do { size=read(fd,buffer,sizeof(buffer)-1); } while(size<0 && errno==EINTR);
    const int saved=errno; close(fd); errno=saved;
    if(size<=0 || size==sizeof(buffer)-1)return false;
    buffer[size]=0;
    char *end=nullptr; errno=0;
    unsigned long id=strtoul(buffer,&end,10);
    if(errno || id!=tid || !end || end[0]!=' ' || end[1]!='(')return false;
    char *last=strrchr(end+2,')');
    if(!last || last[1]!=' ' || !last[2] || last[3]!=' ')return false;
    const auto length=static_cast<std::size_t>(last-(end+2));
    if(length>=sizeof(out->name))return false;
    memcpy(out->name,end+2,length); out->name[length]=0;
    // Fields 4..22 follow the state character. Parse each complete field;
    // signed fields are skipped without coercing them into unsigned counters.
    char *cursor=last+4;
    std::uint64_t user=0,system=0;
    for(int field=4;field<=22;++field) {
        while(*cursor==' ')++cursor;
        char *next=strchr(cursor,' ');
        if(!next)return false;
        if(field==14 || field==15 || field==22) {
            if(*cursor<'0' || *cursor>'9')return false;
            errno=0; auto value=strtoull(cursor,&end,10);
            if(errno || end!=next)return false;
            if(field==14)user=value; else if(field==15)system=value; else out->start=value;
        }
        cursor=next+1;
    }
    if(user>std::numeric_limits<std::uint64_t>::max()-system)return false;
    out->ticks=user+system; return true;
}
bool clockNs(clockid_t clock,std::uint64_t *out) {
    if(!out){errno=EINVAL;return false;}
    timespec time{};
    if(clock_gettime(clock,&time) || time.tv_sec<0 || time.tv_nsec<0)return false;
    const auto seconds=static_cast<std::uint64_t>(time.tv_sec);
    if(seconds>(UINT64_MAX-static_cast<unsigned long>(time.tv_nsec))/1000000000ull)return false;
    *out=seconds*1000000000ull+time.tv_nsec;return true;
}
}
CpuThread *OpenCpuThread(unsigned tid) {
    if(!tid){errno=EINVAL;return nullptr;}
    char path[64];std::snprintf(path,sizeof(path),"/proc/self/task/%u",tid);
    int directory=open(path,O_RDONLY|O_DIRECTORY|O_CLOEXEC);
    if(directory<0)return nullptr;
    Stat stat{};
    if(!readStat(directory,tid,&stat)){close(directory);return nullptr;}
    auto *thread=new(std::nothrow) CpuThread{directory,tid,stat.start};
    if(!thread)close(directory);
    return thread;
}
void CloseCpuThread(CpuThread *thread){if(thread){close(thread->directory);delete thread;}}
bool ReadCpuThread(CpuThread *thread,std::uint64_t *out) {
    if(!thread || !out){errno=EINVAL;return false;}
    Stat stat{};
    if(!readStat(thread->directory,thread->tid,&stat) || stat.start!=thread->start)return false;
    const long frequency=sysconf(_SC_CLK_TCK);
    if(frequency<=0)return false;
    const auto rate=static_cast<std::uint64_t>(frequency);
    const auto seconds=stat.ticks/rate, remainder=stat.ticks%rate;
    if(seconds>UINT64_MAX/1000000000ull || remainder>UINT64_MAX/1000000000ull)return false;
    const auto fraction=remainder*1000000000ull/rate;
    if(seconds*1000000000ull>UINT64_MAX-fraction)return false;
    *out=seconds*1000000000ull+fraction;return true;
}
bool CpuThreadName(CpuThread *thread,char *name,std::size_t capacity) {
    if(!thread || !name || !capacity){errno=EINVAL;return false;}
    Stat stat{};
    if(!readStat(thread->directory,thread->tid,&stat) || stat.start!=thread->start)return false;
    const auto length=strlen(stat.name);
    if(length>=capacity){errno=ERANGE;return false;}
    memcpy(name,stat.name,length+1);return true;
}
bool EnumerateCpuThreads(unsigned *ids,std::size_t capacity,std::size_t *count) {
    if(!ids || !count){errno=EINVAL;return false;}
    DIR *directory=opendir("/proc/self/task");if(!directory)return false;
    std::size_t n=0; bool ok=true;
    for(;;) {
        errno=0;dirent *entry=readdir(directory);
        if(!entry){if(errno)ok=false;break;}
        if(entry->d_name[0]<'0' || entry->d_name[0]>'9')continue;
        char *end=nullptr;auto id=strtoul(entry->d_name,&end,10);
        if(errno || *end || !id || id>UINT32_MAX)continue;
        if(n==capacity){errno=ENOSPC;ok=false;break;}
        ids[n++]=static_cast<unsigned>(id);
    }
    const int saved=errno;closedir(directory);errno=saved;
    if(ok)*count=n;return ok;
}
bool ProcessCpuNanoseconds(std::uint64_t *out){return clockNs(CLOCK_PROCESS_CPUTIME_ID,out);}
bool CurrentCpuNanoseconds(std::uint64_t *out){return clockNs(CLOCK_THREAD_CPUTIME_ID,out);}
}
