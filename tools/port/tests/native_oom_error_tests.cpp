#include <platform/android/oom_error.h>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
// Explicit isolated immutable policy queries, not engine/UI/Monkey readiness.
static bool headless=true,monkey=false;
bool Sys_IsHeadless(){return headless;}
bool Monkey_IsRunning(){return monkey;}
static std::atomic<bool> armed{},stdioLocked{};static bool recurse{};static int marker=-1;
extern "C" void *__real_malloc(std::size_t);extern "C" void *__real_calloc(std::size_t,std::size_t);extern "C" void *__real_realloc(void *,std::size_t);extern "C" int __real_sysinfo(struct sysinfo *);
extern "C" void *__wrap_malloc(std::size_t n){if(armed)std::_Exit(91);return __real_malloc(n);}
extern "C" void *__wrap_calloc(std::size_t n,std::size_t s){if(armed)std::_Exit(91);return __real_calloc(n,s);}
extern "C" void *__wrap_realloc(void *p,std::size_t n){if(armed)std::_Exit(91);return __real_realloc(p,n);}
extern "C" int __wrap_sysinfo(struct sysinfo *p){if(armed && recurse){recurse=false;Sys_OutOfMemErrorInternal("injected genuine recursive handler entry",2);}return __real_sysinfo(p);}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL OOM fixture line%d errno%d\n",__LINE__,errno);std::abort();}}while(0)
static void AtExit(){constexpr char clean[]="clean-from-atexit\n";pwrite(marker,clean,sizeof(clean)-1,0);}
static void *HoldStdio(void *){flockfile(stderr);stdioLocked=true;for(;;)pause();return nullptr;}
static long long Millis(){timespec t{};CHECK(!clock_gettime(CLOCK_MONOTONIC,&t));return t.tv_sec*1000LL+t.tv_nsec/1000000;}
static void Case(const char *directory,const char *name,bool render,bool monkeyMode,bool recursive,bool full,bool broken){
    int output[2];CHECK(!pipe(output));
    if(full){const int flags=fcntl(output[1],F_GETFL);CHECK(flags>=0 && !fcntl(output[1],F_SETFL,flags|O_NONBLOCK));char fill[4096]{};while(write(output[1],fill,sizeof(fill))>0){}CHECK(errno==EAGAIN || errno==EWOULDBLOCK);}
    char path[1024];CHECK(snprintf(path,sizeof(path),"%s/.oom-fixture-%s",directory,name)>0);
    const auto child=fork();CHECK(child>=0);
    if(!child){
        close(output[0]);if(broken)close(output[1]);
        if(broken){int closed[2];if(pipe(closed))std::_Exit(92);close(closed[0]);if(dup2(closed[1],STDERR_FILENO)<0)std::_Exit(92);close(closed[1]);}
        else {if(dup2(output[1],STDERR_FILENO)<0)std::_Exit(92);close(output[1]);}
        headless=!render;monkey=monkeyMode;recurse=recursive;
        marker=open(path,O_RDWR|O_CREAT|O_TRUNC|O_CLOEXEC,0600);if(marker<0 || flock(marker,LOCK_EX|LOCK_NB))std::_Exit(92);
        constexpr char running[]="running-before-OOM\n";if(write(marker,running,sizeof(running)-1)!=sizeof(running)-1)std::_Exit(92);if(atexit(AtExit))std::_Exit(92);
        // A different real OS thread holds stderr's FILE lock. The reporter
        // cannot use stdio or wait for that owner to retire.
        pthread_t owner;if(pthread_create(&owner,nullptr,HoldStdio,nullptr))std::_Exit(92);while(!stdioLocked)usleep(1000);
        rlimit limit{};if(getrlimit(RLIMIT_AS,&limit))std::_Exit(92);limit.rlim_cur=1024*1024;if(setrlimit(RLIMIT_AS,&limit))std::_Exit(92);
        errno=0;void *failed=__real_malloc(32*1024*1024);if(failed || errno!=ENOMEM)std::_Exit(93);
        armed=true;char filename[900];memset(filename,'x',sizeof(filename)-1);filename[sizeof(filename)-1]=0;
        bo1::android::ReportAllocationFailure(filename,-2147483647-1,"actual failed malloc under RLIMIT_AS","fixture kernel exhaustion",32*1024*1024,0,false);std::_Exit(94);
    }
    close(output[1]);if(broken){close(output[0]);output[0]=-1;}
    const auto deadline=Millis()+2000;int status{};for(;;){const auto done=waitpid(child,&status,WNOHANG);CHECK(done>=0);if(done==child)break;if(Millis()>deadline){kill(child,SIGKILL);waitpid(child,&status,0);CHECK(false);}usleep(1000);}
    CHECK(WIFEXITED(status) && WEXITSTATUS(status)==(render||monkeyMode?255:3));
    std::string log;if(output[0]>=0){char buffer[4096];for(;;){const auto n=read(output[0],buffer,sizeof(buffer));if(n<=0)break;log.append(buffer,n);}close(output[0]);}
    if(!full && !broken){CHECK(log.find("dirty")!=std::string::npos);if(recursive)CHECK(log.find("recursive/concurrent")!=std::string::npos);else{CHECK(log.find("line=-2147483648")!=std::string::npos && log.find("...")!=std::string::npos);CHECK(log.find("OS physical total bytes=0\n")==std::string::npos);CHECK(log.find("requestedBytes=33554432 deficitBytes=unavailable")!=std::string::npos);CHECK(log.find("OS MemAvailable KiB=")!=std::string::npos);CHECK(log.find("Process VmRSS KiB=")!=std::string::npos);}}
    const int file=open(path,O_RDWR|O_CLOEXEC);CHECK(file>=0 && !flock(file,LOCK_EX|LOCK_NB));char retained[32]{};CHECK(read(file,retained,sizeof(retained))>0);CHECK(!strcmp(retained,"running-before-OOM\n"));close(file);CHECK(!unlink(path));
    printf("PASS actual RLIMIT_AS/ENOMEM terminal%s exit%d/heap tripwire/other-thread stdio lock/non-clean marker/kernel lock release%s\n",name,WEXITSTATUS(status),recursive?"/recursive entry":(full?"/full nonblocking pipe":(broken?"/broken pipe":"/actual OS metrics")));
}
int main(int argc,char **argv){CHECK(argc==2);Case(argv[1],"headless",false,false,false,false,false);Case(argv[1],"rendered",true,false,false,false,false);Case(argv[1],"monkey",false,true,false,false,false);Case(argv[1],"recursive",false,false,true,false,false);Case(argv[1],"full-pipe",false,false,false,true,false);Case(argv[1],"broken-pipe",false,false,false,false,true);}
