#include <platform/android/headless_diagnostics.h>
#include <platform/android/headless_policy.h>
#include <game_mp/g_main_mp.h>
#include <client/client.h>
#include <platform/android/udp_socket.h>
#include <platform/android/thread_services.h>
#include <platform/critical_section_services.h>
#include <clientscript/cscr_stringlist.h>
#include <qcommon/msg_mp.h>
#include <qcommon/common.h>
#include <stringed/stringed_hooks.h>
#include <universal/dvar.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <csignal>

namespace {
unsigned checks;std::atomic<unsigned long long> entered{},exited{};
std::atomic<unsigned> quits{};bool reenterFatal{};
void Check(bool ok,const char *why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);_exit(91);}++checks;}

}
extern "C" void __cyg_profile_func_enter(void*,void*){entered.fetch_add(1,std::memory_order_relaxed);}
extern "C" void __cyg_profile_func_exit(void*,void*){exited.fetch_add(1,std::memory_order_relaxed);}
void Com_Error(errorParm_t,const char *format,...){if(reenterFatal)Sys_HeadlessNoteHud(1,false);va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::fputc('\n',stderr);}
void Com_Printf(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Com_PrintWarning(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Com_PrintError(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Com_DPrintf(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
// Explicit isolated providers for unexecuted logging/localization/script failure paths.
const dvar_t *com_logfile,*com_sv_running;
level_locals_t level{};
clientUIActive_t clientUIActives[1]{};
void Com_Quit_f(){++quits;} // Isolated terminal sink; whole-engine cleanup is deliberately not claimed.
void Com_PrintMessage(int,char *message,int){std::fputs(message,stderr);}
bool Com_LogFileOpen(){return false;}
[[noreturn]] static void Unexecuted(const char *name){std::fprintf(stderr,"UNEXECUTED: %s\n",name);std::abort();}
bool Assert_MyHandler(const char *file,int line,int,const char *format,...){std::fprintf(stderr,"assert %s:%d ",file,line);va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::abort();}
char *SEH_LocalizeTextMessage(const char*,const char*,msgLocErrType_t){Unexecuted("localization");}
void Scr_DumpScriptThreads(scriptInstance_t){Unexecuted("script dump");}
void Scr_DumpScriptVariablesDefault(scriptInstance_t){Unexecuted("script variable dump");}
void Scr_TerminalError(const char*,scriptInstance_t){Unexecuted("script terminal error");}

int main(int argc,char **argv){
    alarm(15);Sys_InitMainThread();Sys_InitializeCriticalSections();Sys_InitializeHeadlessPolicy("+set bo1_headless 1");SL_Init(SCRIPTINSTANCE_SERVER);SL_Init(SCRIPTINSTANCE_CLIENT);Dvar_Init();
    const bool hitch=argc>1 && !std::strcmp(argv[1],"hitch");
    const bool dump=argc>1 && !std::strcmp(argv[1],"dump");
    bo1::android::ConfigureDiagnostics(hitch?12:0,hitch?20:0,dump?10:0);
    bo1::android::RegisterDiagnosticsDvars();
    for(int fault=0;fault<2;++fault){const pid_t child=fork();Check(child>=0,"actual failure child");if(!child){reenterFatal=true;bo1::android::ConfigureDiagnostics(fault?-1:0,0,0);_exit(93);}int status{};Check(waitpid(child,&status,0)==child && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT,"invalid/repeated configure fails after unlocked reentrant returning reporter");}

    com_sv_running=_Dvar_RegisterBool("sv_running",false,0,"isolated actual server state Dvar");
    Check(Sys_FramePerfHitchMs()==(hitch?12u:0u),"actual configured frame threshold");
    Check(Sys_HitchWatchBegin()==0,"no watched frame before actual-return hook");
    Check(!Dvar_GetInt("bo1_autoquit") && !*Dvar_GetString("bo1_quitnotify") && !Dvar_GetBool("bo1_dvardump"),"actual canonical offdefault Dvars");
    Dvar_SetStringByName("bo1_quitnotify","round,2,50");
    level.time=100;Sys_HeadlessLevelNotify("different");Sys_HeadlessLevelNotify("round");
    level.time=200;Sys_HeadlessLevelNotify("round");
    // Actual level storage is isolated fixture state, not a running server.
    bo1::android::DiagnosticsFrame();Check(quits.load()==0,"no terminal action before return publication");
    bo1::android::DiagnosticsInitializationReturned();
    Sys_HeadlessTimeline("fixture actual-return observation");Sys_HeadlessNoteHud(7,true);
    if(hitch){Check(Sys_HitchWatchBegin()==1,"first real watched frame sequence");std::this_thread::sleep_for(std::chrono::milliseconds(65));Sys_HitchWatchEnd();Check(Sys_HitchWatchBegin()==2,"next real watched frame sequence");Sys_HitchWatchEnd();}
    if(dump)std::this_thread::sleep_for(std::chrono::milliseconds(40));
    level.time=250;bo1::android::DiagnosticsFrame();Check(quits.load()==0,"level quit requires actual running Dvar");
    Dvar_SetBoolByName("sv_running",true);level.time=249;bo1::android::DiagnosticsFrame();Check(quits.load()==0,"level deadline not premature");
    level.time=250;bo1::android::DiagnosticsFrame();Check(quits.load()==1,"real notify count retained across pre-return boundary and level deadline");
    Dvar_SetIntByName("bo1_autoquit",1);std::this_thread::sleep_for(std::chrono::milliseconds(3));bo1::android::DiagnosticsFrame();Check(quits.load()==2,"actual elapsed-time autoquit deadline");
    bo1::android::StopDiagnostics();const unsigned previous=quits;bo1::android::DiagnosticsFrame();Check(quits.load()==previous,"retired diagnostics cannot issue terminal action");
    std::printf("PASS: %u actual diagnostic lifecycle/state checks (%s)\n",checks,argc>1?argv[1]:"off");
}
