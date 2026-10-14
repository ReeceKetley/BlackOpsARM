#include <platform/android/process_guard.h>
#include <platform/android/shutdown_services.h>
#include <platform/android/server_thread.h>
#include <platform/android/thread_services.h>
#include <platform/android/console_input.h>
#include <platform/android/zone_heap.h>
#include <qcommon/common.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
std::atomic<unsigned> checks{};
std::atomic<unsigned long long> entered{},exited{};
std::size_t allocationBaseline;
bool partialProof;
std::atomic<int> callbackStage{},callbackRelease{};
int callbackMode;
void Check(bool ok,const char *why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);_exit(91);}checks.fetch_add(1);}
void Until(int stage){for(int i=0;i<5000;++i){if(callbackStage.load()>=stage)return;std::this_thread::sleep_for(std::chrono::milliseconds(1));}_exit(92);}
void ParkCallback(unsigned role){
    Check(role==THREAD_CONTEXT_SERVER,"actual installed park callback role");
    if(callbackMode==5)std::this_thread::sleep_for(std::chrono::milliseconds(15));
    Sys_InitServerEvents();Sys_ServerCompleted();callbackStage.store(1);
    if(callbackMode==1){
        Check(Sys_WaitStartServer(5000),"real pending frame admission");
        Sys_ResetServerNetworkCompletedEvent();callbackStage.store(2);
        while(!callbackRelease.load())std::this_thread::yield();
        Sys_SetServerNetworkCompletedEvent();Sys_ServerCompleted();
    }
    if(callbackMode==2){
        while(!callbackRelease.load())std::this_thread::yield();
        Sys_WaitAllowServerNetworkLoop();
        Sys_ResetServerNetworkCompletedEvent();Sys_SetServerNetworkCompletedEvent();Sys_ServerCompleted();
    }
    callbackStage.store(3);
    if(callbackMode==3)for(;;)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    (void)Sys_WaitStartServer(0xffffffffu); // Actual terminal checkpoint parks.
    _exit(93);
}
int ExecCase(const char *exe,const char *mode,const char *root){
    const pid_t child=fork();Check(child>=0,"fresh instance fork");
    if(!child){execl(exe,exe,mode,root,nullptr);_exit(94);}
    int status{};Check(waitpid(child,&status,0)==child && WIFEXITED(status),"fresh instance result");return WEXITSTATUS(status);
}
std::string Marker(const char *root){char path[512];std::snprintf(path,sizeof(path),"%s/.blackopsarm-instance",root);FILE *file=std::fopen(path,"rb");Check(file!=nullptr,"real marker read");char value[96]{};std::fgets(value,sizeof(value),file);std::fclose(file);return value;}
}
// Unexecuted whole-engine callback/error paths fail explicitly. No initialized
// engine callbacks or fake successful full-cleanup services are supplied.
void Com_Printf(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Com_Quit_f(){std::fputs("UNEXECUTED whole-engine quit callback reached\n",stderr);_exit(95);}
void Com_Error(errorParm_t,const char *,...){std::fputs("UNEXECUTED engine input-error path reached\n",stderr);_exit(96);}
extern "C" void __cyg_profile_func_enter(void*,void*){entered.fetch_add(1,std::memory_order_relaxed);}
extern "C" void __cyg_profile_func_exit(void*,void*){exited.fetch_add(1,std::memory_order_relaxed);}
extern "C" void __real__Z18Sys_ShutdownEventsv();
extern "C" void __wrap__Z18Sys_ShutdownEventsv(){
    __real__Z18Sys_ShutdownEventsv();
    if(partialProof){
        Check(!bo1::android::ConsoleInputOwnedByCaller(),"real console owner retired before queue");
        Check(bo1::android::ZoneAllocationCount()==allocationBaseline,"canonical queue freed real tagged payload");
        std::fputs("PARTIAL actual console owner and queued allocation retired\n",stderr);
    }
}
int main(int argc,char **argv){
    alarm(30);Sys_InitMainThread();
    if(argc==3){
        if(!std::strcmp(argv[1],"--contend"))return bo1::android::AcquireProcessGuard(argv[2])?97:7;
        if(!std::strcmp(argv[1],"--acquire-clean")){Check(bo1::android::AcquireProcessGuard(argv[2]),"new instance after release");bo1::android::ReleaseProcessGuardClean();return 0;}
        if(!std::strcmp(argv[1],"--early-normal")){Check(bo1::android::AcquireProcessGuard(argv[2]),"early normal guard");Sys_NormalExit();_exit(98);}
        if(!std::strcmp(argv[1],"--partial")){
            Check(bo1::android::AcquireProcessGuard(argv[2]),"partial startup guard");
            int pipeDescriptors[2];Check(pipe(pipeDescriptors)==0,"real console pipe");
            allocationBaseline=bo1::android::ZoneAllocationCount();
            bo1::android::InstallConsoleInput(pipeDescriptors[0]);close(pipeDescriptors[0]);
            Check(write(pipeDescriptors[1],"partial-command\n",16)==16,"real console input");close(pipeDescriptors[1]);
            bo1::android::PollConsoleInput();
            Check(bo1::android::ZoneAllocationCount()==allocationBaseline+1,"actual queued tagged payload");
            partialProof=true;bo1::android::RequireInitializedQuitEntry();_exit(98);
        }
        if(!std::strncmp(argv[1],"--park",6)){
            callbackMode=argv[1][6]-'0';
            if(callbackMode==4){bo1::android::ParkServerForTerminalShutdown();Check(bo1::android::ServerTerminalShutdownEstablished(),"unspawned admission actually sealed");Sys_SpawnServerThread(ParkCallback);_exit(99);}
            Check(Sys_SpawnServerThread(ParkCallback)==1,"actual parked worker spawn");if(callbackMode!=5)Until(1);
            if(callbackMode==1){Sys_WakeServer();Until(2);Check(!Sys_WaitServer(0),"pending frame really incomplete");
                std::thread([]{std::this_thread::sleep_for(std::chrono::milliseconds(30));callbackRelease.store(1);}).detach();}
            if(callbackMode==2){Sys_ResetServerAllowNetworkEvent();callbackRelease.store(1);}
            bo1::android::ParkServerForTerminalShutdown();
            Check(bo1::android::ServerTerminalShutdownEstablished(),"real parked acknowledgment established");
            Check(Sys_WaitServer(0) && Sys_WaitServer(0),"actual completed manual gate retained");
            Sys_WaitServerNetworkCompleted();Sys_ResetServerAllowNetworkEvent();Sys_SetServerAllowNetworkEvent();
            Check(g_networkOverrideThread==0,"parked worker released actual override");
            std::printf("PARK%u PASS %u bounded actual protocol checks\n",callbackMode,checks.load());std::fflush(stdout);_exit(0);
        }
        return 99;
    }
    char root[]="/data/local/tmp/bo1-shutdown-XXXXXX";Check(mkdtemp(root)!=nullptr,"real temporary user root");
    Check(bo1::android::AcquireProcessGuard(root) && bo1::android::ProcessGuardHeld(),"actual held guard");
    Check(ExecCase(argv[0],"--contend",root)==7,"independent live instance excluded");
    const pid_t inherited=fork();Check(inherited>=0,"inherited wrong-process release fork");
    if(!inherited){bo1::android::ReleaseProcessGuardClean();_exit(98);}int status{};Check(waitpid(inherited,&status,0)==inherited && WIFEXITED(status) && WEXITSTATUS(status)==2,"foreign inherited process cannot release owner guard");
    Check(ExecCase(argv[0],"--contend",root)==7,"owner exclusion survives foreign release failure");
    char path[512];std::snprintf(path,sizeof(path),"%s/.blackopsarm-instance",root);struct stat before{},after{};Check(stat(path,&before)==0,"actual guard inode");
    bo1::android::ReleaseProcessGuardClean();Check(!bo1::android::ProcessGuardHeld() && Marker(root).starts_with("clean pid="),"clean release retires guard descriptor");
    Check(stat(path,&after)==0 && before.st_ino==after.st_ino,"locked inode never unlinked/replaced");
    Check(ExecCase(argv[0],"--acquire-clean",root)==0,"fresh instance acquires original released inode");
    Check(ExecCase(argv[0],"--early-normal",root)==2 && Marker(root).starts_with("running pid="),"early public normal exit cannot publish clean");
    Check(ExecCase(argv[0],"--partial",root)==2 && Marker(root).starts_with("running pid="),"partial startup terminates with dirty marker");
    Check(ExecCase(argv[0],"--acquire-clean",root)==0,"OS releases terminal partial instance lock");
    for(int mode=0;mode<3;++mode){char option[24];std::snprintf(option,sizeof(option),"--park%d",mode);Check(ExecCase(argv[0],option,root)==0,"actual wait/frame/network gate park case");}
    Check(ExecCase(argv[0],"--park3",root)==2,"actual missing acknowledgment timeout is fatal");
    Check(ExecCase(argv[0],"--park4",root)==2,"unspawned terminal seal rejects later spawn");
    Check(ExecCase(argv[0],"--park5",root)==0,"immediate startup request waits actual callback boundary");
    unlink(path);rmdir(root);
    std::printf("PASS: %u native process guard/partial terminal/park parent checks\n",checks.load());
}
