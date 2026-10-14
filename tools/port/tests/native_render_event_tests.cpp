#include <platform/android/render_worker.h>
#include <platform/android/thread_services.h>
#include <universal/thread_data.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
namespace {
std::atomic<unsigned> checks{};
std::atomic<unsigned long long> entered{},exited{};
std::atomic<int> stage{};
void *control;
void Check(bool ok,const char *why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);_exit(91);}checks.fetch_add(1);}
void Until(int needed){for(int i=0;i<5000;++i){if(stage.load()>=needed)return;std::this_thread::sleep_for(std::chrono::milliseconds(1));}_exit(92);}
void Callback(unsigned role){
    Check(role==THREAD_CONTEXT_BACKEND && Sys_IsRenderThread() && !Sys_IsMainThread(),"real native backend owner");
    Check(Sys_GetValue(2)==g_com_error[role] && Sys_GetValue(4)==&g_cmd_args[1],"actual backend canonical TLS");
    stage.store(1);Sys_WaitForSingleObject(&control);
    stage.store(2);Sys_WaitD3DDeviceOKEvent();
    stage.store(3);Sys_WaitRGRegisteredEvent();
    stage.store(4);Sys_WaitForDemoStreamingEvent();
    stage.store(5);Sys_WaitForSingleObject(&control);
    // Finite callback permits actual owner retirement/join; no original renderer
    // loop, registration, device creation, frame rendering or restart claim.
}
void ExpectedUninitializedFailure(){
    const pid_t child=fork();Check(child>=0,"uninitialized event fork");
    if(!child){(void)Sys_QueryRenderEvent();_exit(93);}
    int status{};Check(waitpid(child,&status,0)==child && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT,"uninitialized handle fails instead of successful poll");
}
}
extern "C" void __cyg_profile_func_enter(void*,void*){entered.fetch_add(1,std::memory_order_relaxed);}
extern "C" void __cyg_profile_func_exit(void*,void*){exited.fetch_add(1,std::memory_order_relaxed);}
int main(){
    alarm(15);Sys_InitMainThread();ExpectedUninitializedFailure();
    Sys_InitDemoStreamingEvent();
    Check(!Sys_WaitForSingleObjectTimeout(&demoStreamingReady,0),"actual demo initially clear");
    Sys_SetDemoStreamingEvent();Sys_SetDemoStreamingEvent();Sys_WaitForDemoStreamingEvent();
    Check(!Sys_WaitForSingleObjectTimeout(&demoStreamingReady,0),"actual demo auto-reset signals coalesce/consume once");
    Sys_CreateEvent(false,false,&control);Check(Sys_SpawnRenderThread(Callback)==1,"actual native renderer thread spawn");Until(1);
    Check(Sys_QueryD3DDeviceOKEvent() && Sys_QueryD3DDeviceOKEvent(),"original manual initial device event state");
    Check(!Sys_QueryD3DShutdownEvent() && !Sys_QueryRenderEvent(),"original shutdown/render initial clear states");
    Sys_SetD3DDeviceStartEvent();Check(Sys_WaitForSingleObjectTimeout(&d3dDeviceHardStartEvent,0) && Sys_WaitForSingleObjectTimeout(&d3dDeviceHardStartEvent,0),"actual hard-start manual signal persists");
    Sys_ClearD3DDeviceStartEvent();Check(!Sys_WaitForSingleObjectTimeout(&d3dDeviceHardStartEvent,0),"actual hard-start reset");
    Sys_SetD3DShutdownEvent();Check(Sys_QueryD3DShutdownEvent() && Sys_QueryD3DShutdownEvent(),"actual shutdown manual signal persists");
    Sys_ClearD3DShutdownEvent();Check(!Sys_QueryD3DShutdownEvent(),"actual shutdown reset");
    Sys_SetEvent(&renderEvent);Check(Sys_QueryRenderEvent() && Sys_QueryRenderEvent(),"actual render manual query preserves signal");
    Sys_ResetEvent(&renderEvent);Check(!Sys_QueryRenderEvent(),"actual render reset");
    Sys_ClearD3DDeviceOKEvent();Sys_SetEvent(&control);Until(2);
    Check(!Sys_QueryD3DDeviceOKEvent() && !bo1::android::WaitForRenderThreadRetirement(2),"actual device wait remains blocked until signal");
    Sys_SetD3DDeviceOKEvent();Until(3);
    Check(!Sys_QueryRGRegisteredEvent() && !bo1::android::WaitForRenderThreadRetirement(2),"actual registration wait remains blocked until signal");
    Sys_SetRGRegisteredEvent();Until(4);
    Check(!Sys_WaitForSingleObjectTimeout(&demoStreamingReady,0) && !bo1::android::WaitForRenderThreadRetirement(2),"actual demo wait remains blocked until signal");
    Sys_SetDemoStreamingEvent();Until(5);
    Check(!Sys_WaitForSingleObjectTimeout(&demoStreamingReady,0),"callback consumed actual auto-reset demo signal");
    Sys_SetEvent(&control);Check(bo1::android::WaitForRenderThreadRetirement(5000),"actual finite callback retirement");
    bo1::android::JoinRetiredRenderThread();
    Check(stage.load()==5,"genuine join after finite callback only");
    std::printf("PASS: %u native renderer/demo event synchronization checks\n",checks.load());
}
