#include <platform/startup_services_api.h>
#include <platform/android/render_worker.h>
#include <platform/android/thread_services.h>
#include <platform/android/engine_paths.h>
#include <platform/android/headless_policy.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
extern "C" void __cyg_profile_func_enter(void *, void *) {}
extern "C" void __cyg_profile_func_exit(void *, void *) {}
extern const dvar_t *r_logFile;
void RB_OpenLogFile();void RB_CloseLogFile();void RB_LogPrint(const char *);
bool RB_IsLogging();const char *R_ErrorDescription(int);void Sys_DirectXFatalError();
std::atomic<int> payload{};std::atomic<bool> identity{};
void check(bool good,const char *message) {
    if(!good){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}
    std::printf("PASS %s\n",message);
}
void callback(unsigned role) {
    identity=role==1 && Sys_IsRenderThread() && !std::strcmp(Sys_GetCurrentThreadName(),"Backend");
    Sys_WaitResourcesQueuedEvent(5000);
    if(!Sys_WaitForSingleObjectTimeout(&resourcesQueuedEvent,0))std::abort();
    Sys_ClearResourcesQueuedEvent();payload.fetch_add(4);Sys_SetResourcesFlushedEvent();
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    std::setvbuf(stdout,nullptr,_IONBF,0);
    Sys_InitializeHeadlessPolicy("+set bo1_headless 1 +set dedicated 1");
    Sys_InitializeCriticalSections();Sys_InitMainThread();Dvar_Init();
    bo1::android::InstallEngineUserRoot(argv[1]);
    check(Sys_SpawnRenderThread(callback)==1,"actual backend thread creation and TLS publication");
    check(!Sys_QueryRGRegisteredEvent(),"graphics registration remains unsignaled");
    check(!Sys_WaitForSingleObjectTimeout(&resourcesFlushedEvent,0),"original flushed event starts unsignaled");
    check(Sys_WaitForSingleObjectTimeout(&renderCompletedEvent,0),"original idle-completed event starts signaled");
    payload=73;Sys_SetResourcesQueuedEvent();Sys_WaitResourcesFlushedEvent();
    check(identity && payload==77,"real backend callback consumes queue signal and publishes completion");
    check(!Sys_WaitForSingleObjectTimeout(&resourcesQueuedEvent,0),"original queued-event reset");
    check(Sys_WaitForSingleObjectTimeout(&resourcesFlushedEvent,0),"flushed completion is manual-reset state");
    Sys_ClearResourcesFlushedEvent();
    check(!Sys_WaitForSingleObjectTimeout(&resourcesFlushedEvent,0),"original flushed-event reset");
    check(bo1::android::WaitForRenderThreadRetirement(2000),"actual callback retirement");
    bo1::android::JoinRetiredRenderThread();
    check(!Sys_QueryRGRegisteredEvent(),"thread completion does not fabricate graphics registration");
    r_logFile=_Dvar_RegisterInt("r_logFile",1,0,10,0,"test logging configuration");
    RB_OpenLogFile();check(RB_IsLogging(),"real renderer log opens in writable user root");
    RB_LogPrint("native renderer log\n");RB_CloseLogFile();check(!RB_IsLogging(),"real renderer log closes");
    const auto path=std::string(argv[1])+"/dx.log";FILE *file=fopen(path.c_str(),"rb");
    char bytes[256]{};const auto count=file?fread(bytes,1,sizeof(bytes)-1,file):0;
    if(file)fclose(file);
    check(count && std::strstr(bytes,"native renderer log\r\n"),"native log preserves physical Windows text newlines");
    check(!std::strcmp(R_ErrorDescription(static_cast<int>(0x8007000eu)),"Direct3D HRESULT 0x8007000e"),"real HRESULT bits reported without Windows DXERR dependency");
    const auto child=fork();if(child==0)Sys_DirectXFatalError();int status=0;
    check(child>0 && waitpid(child,&status,0)==child && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT,"native renderer fatal error terminates rather than returning success");
    check(unlink(path.c_str())==0,"test-owned log cleanup");Dvar_Shutdown();
}
